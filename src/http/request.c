/*
 *  request.c -- Request class to handle individual HTTP requests.
 *
 *  The Request class is the real work-horse in managing HTTP requests. An instance is created per HTTP request. During
 *  keep-alive it is preserved to process further requests. Requests run in a single thread and do not need multi-thread
 *  locking except for the timeout code which may run on another thread.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/***************************** Forward Declarations ***************************/

static void addMatchEtag(MaConn *conn, char *etag);
static int  destroyRequest(MaRequest *req);
static bool getChunkSize(MaConn *conn, MprBuf *buf, int *boundaryLen, int *size);
static char *getToken(MaConn *conn, cchar *delim);
static bool matchEtag(MaConn *conn, char *requestedEtag);
static bool matchModified(MaConn *conn, MprTime time);
static bool parseFirstLine(MaConn *conn, MaPacket *packet);
static bool parseHeaders(MaConn *conn, MaPacket *packet);
static bool parseRange(MaConn *conn, char *value);
static bool parseRequest(MaConn *conn, MaPacket *packet);
static void processChunk(MaConn *conn, MaPacket *packet);
static void processContent(MaConn *conn, MaPacket *packet);
static bool processCompletion(MaConn *conn);
static void setIfModifiedDate(MaConn *conn, MprTime when, bool ifMod);

#if BLD_DEBUG
static void traceContent(MaConn *conn, MaPacket *packet);
#endif

/*********************************** Code *************************************/

MaRequest *maCreateRequest(MaConn *conn)
{
    MaRequest   *req;
    MprHeap     *arena;

    /*
     *  Create a request memory arena. From this arena, are all allocations made for this entire request.
     *  Arenas are scalable, thread-safe virtual memory blocks that are freed in one chunk.
     */
    arena  = mprAllocArena(conn->arena, "request", MA_REQ_MEM, 0, NULL);
    if (arena == 0) {
        return 0;
    }

    req = mprAllocObjWithDestructorZeroed(arena, MaRequest, destroyRequest);
    if (req == 0) {
        return 0;
    }
    req->conn = conn;
    req->arena = arena;
    req->length = -1;
    req->ifMatch = 1;
    req->ifModified = 1;
    req->host = conn->host;
    req->remainingContent = 0;
    req->method = MA_REQ_GET;

    req->headers = mprCreateHash(req, MA_VAR_HASH_SIZE);

    return req;
}


int destroyRequest(MaRequest *req)
{
    maResetConn(req->conn);
    return 0;
}


/*
 *  Process a write event. These occur when a request could not be completed when it was first received.
 */
void maProcessWriteEvent(MaConn *conn)
{
    mprLog(conn, 6, "maProcessWriteEvent, state %d", conn->state);

    if (unlikely(conn->expire <= conn->time)) {
        /*
         *  Ignore the event if we have expired. TODO - who cleans up?
         */
        return;
    }
    if (conn->response) {
        /*
         *  Enable the queue upstream from the connector
         */
        maEnableQueue(conn->response->queue[MA_QUEUE_SEND].prevQ);
        maServiceQueues(conn);
    }
}


/*
 *  Process incoming requests. This will process as much of the request as possible before returning. All socket I/O is 
 *  non-blocking, and this routine must not block. 
 */
void maProcessReadEvent(MaConn *conn, MaPacket *packet)
{
    mprLog(conn, 6, "maProcessReadEvent, state %d", conn->state);
    conn->canProceed = 1;
    
    while (conn->canProceed) {

        switch (conn->state) {
        case MPR_HTTP_STATE_BEGIN:
            conn->canProceed = parseRequest(conn, packet);
            break;

        case MPR_HTTP_STATE_CONTENT:
            processContent(conn, packet);
            break;

        case MPR_HTTP_STATE_CHUNK:
            processChunk(conn, packet);
            break;

        case MPR_HTTP_STATE_PROCESSING:
            conn->canProceed = maServiceQueues(conn);
            break;

        case MPR_HTTP_STATE_COMPLETE:
            conn->canProceed = processCompletion(conn);
            break;

        default:
            conn->keepAliveCount = 0;
            mprAssert(0);
            return;
        }
    }
}


/*
 *  Parse a new request. Return true to keep going with this or subsequent request, zero means insufficient data to proceed.
 */
static bool parseRequest(MaConn *conn, MaPacket *packet) 
{
    MaRequest   *req;
    char        *start, *end;
    int         len;

    /*
     *  Must wait until we have the complete set of headers.
     */
    if ((len = mprGetBufLength(packet->content)) == 0) {
        return 0;
    }
    if (len >= conn->host->limits->maxHeader) {
        maFailRequest(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, "Header too big");
        return 0;
    }
    start = mprGetBufStart(packet->content);
    if ((end = mprStrnstr(start, "\r\n\r\n", len)) == 0) {
        return 0;
    }

    *end = '\0';
    mprLog(conn, 3, "\n@@@ Request =>\n%s\n", start);
    *end = '\r';
        
    if (!parseFirstLine(conn, packet) || !parseHeaders(conn, packet)) {
        conn->keepAliveCount = 0;
        return 0;
    }  
    
    /*
     *  This request now owns the input packet. Must preserve the headers.
     */    
    req = conn->request;
    conn->input = 0;
    mprStealBlock(req, packet);

    /*
     *  Have read the headers. Create the request pipeline.
     */
    maCreatePipeline(conn);

    if (req->remainingContent > 0) {
        conn->state = (req->flags & MA_REQ_CHUNKED) ? MPR_HTTP_STATE_CHUNK : MPR_HTTP_STATE_CONTENT;
    } else {
        /*
         *  Can run the request now if there is no incoming data.
         */
        conn->state = MPR_HTTP_STATE_PROCESSING;
        maRunPipeline(conn);
    }
    return !conn->abandonConnection;
}


/*
 *  Parse the first line of a http request. Return true if the first line parsed. This is only called once all the headers
 *  have been read and buffered.
 */
static bool parseFirstLine(MaConn *conn, MaPacket *packet)
{
    MaRequest   *req;
    MaResponse  *resp;
    char        *uri;
    int         method;

    req = conn->request = maCreateRequest(conn);
    resp = conn->response = maCreateResponse(conn);

    mprLog(req, 4, "New request from %s:%d to %s:%d", conn->remoteIpAddr, conn->remotePort, conn->sock->ipAddr, 
        conn->sock->port);

    req->methodName = getToken(conn, " ");
    if (*req->methodName == '\0') {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad request method name");
        return 0;
    }

    method = 0;
    switch (req->methodName[0]) {
    case 'D':
        if (strcmp(req->methodName, "DELETE") == 0) {
            method = MA_REQ_DELETE;
        }
        break;

    case 'G':
        if (strcmp(req->methodName, "GET") == 0) {
            method = MA_REQ_GET;
        }
        break;

    case 'P':
        if (strcmp(req->methodName, "POST") == 0) {
            method = MA_REQ_POST;

        } else if (strcmp(req->methodName, "PUT") == 0) {
            method = MA_REQ_PUT;
        }
        break;

    case 'H':
        if (strcmp(req->methodName, "HEAD") == 0) {
            method = MA_REQ_HEAD;
            resp->flags |= MA_RESP_NO_BODY;
        }
        break;

    case 'O':
        if (strcmp(req->methodName, "OPTIONS") == 0) {
            method = MA_REQ_OPTIONS;
            resp->flags |= MA_RESP_NO_BODY;
        }
        break;

    case 'T':
        if (strcmp(req->methodName, "TRACE") == 0) {
            method = MA_REQ_TRACE;
            resp->flags |= MA_RESP_NO_BODY;
        }
        break;
    }

    if (method == 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_METHOD, "Bad method");
        return 0;
    }

    req->method = method;

    uri = getToken(conn, " ");
    if (*uri == '\0') {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad HTTP request. Bad URI.");
        return 0;
    }

    if ((int) strlen(uri) >= conn->http->limits.maxUrl) {
        maFailConnection(conn, MPR_HTTP_CODE_REQUEST_URL_TOO_LARGE, "Bad request. URI too long.");
        return 0;
    }

    req->httpProtocol = getToken(conn, "\r\n");

    if (strcmp(req->httpProtocol, "HTTP/1.1") == 0) {
        conn->protocol = 1;

    } else if (strcmp(req->httpProtocol, "HTTP/1.0") == 0) {
        conn->keepAliveCount = 0;
        conn->protocol = 0;
        if (method == MA_REQ_POST || method == MA_REQ_PUT) {
            req->remainingContent = MAXINT;
        }

    } else {
        maFailConnection(conn, MPR_HTTP_CODE_NOT_ACCEPTABLE, "Unsupported HTTP protocol");
        return 0;
    }

    mprLog(conn, 2, "%s %s %s", req->methodName, uri, req->httpProtocol);

    if (maSetRequestUri(conn, uri) < 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad URL format");
        return 0;
    }
    return 1;
}


/*
 *  Parse the request headers. Return true if the header parsed.
 */
static bool parseHeaders(MaConn *conn, MaPacket *packet)
{
    MaHostAddress   *address;
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host, *hp;
    MaLimits        *limits;
    MprBuf          *content;
    char            keyBuf[MPR_MAX_STRING];
    char            *key, *value, *cp, *tok;
    int             count;

    req = conn->request;
    resp = conn->response;
    host = req->host;
    content = packet->content;
    conn->request->headerPacket = packet;
    limits = &conn->http->limits;

    strcpy(keyBuf, "HTTP_");

    //  TODO - BUG can fall off the end. What if start[0] == '\0'
    for (count = 0; content->start[0] != '\r'; count++) {

        if (count >= limits->maxNumHeaders) {
            maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Too many headers");
            return 0;
        }

        if ((key = getToken(conn, ":")) == 0) {
            maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad header format");
            return 0;
        }

        value = getToken(conn, "\r\n");
        while (isspace((int) *value)) {
            value++;
        }
        if (conn->requestFailed) {
            continue;
        }

        //  TODO - should support header continuations

        mprStrUpper(key);
        //  TODO - why do this??? No needed for Ejs anymore
        for (cp = key; *cp; cp++) {
            if (*cp == '-') {
                *cp = '_';
            }
        }

        mprLog(req, 8, "Key %s, value %s", key, value);

        if (strspn(key, "%<>/\\") > 0) {
            maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad header key value");
            continue;
        }

        /*
         *  Define the header with a "HTTP_" prefix
         */
        mprStrcpy(&keyBuf[5], sizeof(keyBuf) - 5, key);
        mprAddDuplicateHash(req->headers, keyBuf, value);

        switch (key[0]) {
        case 'A':
            if (strcmp(key, "AUTHORIZATION") == 0) {
                req->authType = mprStrTok(value, " \t", &tok);
                req->authDetails = tok;

            } else if (strcmp(key, "ACCEPT_CHARSET") == 0) {
                req->acceptCharset = value;

            } else if (strcmp(key, "ACCEPT") == 0) {
                req->accept = value;

            } else if (strcmp(key, "ACCEPT_ENCODING") == 0) {
                req->acceptEncoding = value;
            }
            break;

        case 'C':
            if (strcmp(key, "CONTENT_LENGTH") == 0) {
                if (req->length >= 0) {
                    maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Mulitple content length headers");
                    continue;
                }
                req->length = atoi(value);
                if (req->length < 0) {
                    maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad content length");
                    continue;
                }
                if (req->length >= host->limits->maxBody) {
                    maFailConnection(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, 
                        "Request content length %d is too big. Limit %d", req->length, host->limits->maxBody);
                    continue;
                }
                mprAssert(req->length >= 0);
                req->remainingContent = req->length;
                req->contentLengthStr = value;

            } else if (strcmp(key, "CONTENT_RANGE") == 0) {
                /*
                 *  This headers specifies the range of any posted body data
                 *  Format is:  Content-Range: bytes n1-n2/length
                 *  Where n1 is first byte pos and n2 is last byte pos
                 */
                char    *sp;
                int     start, end, size;

                start = end = size = -1;

                sp = value;
                while (*sp && !isdigit((int) *sp)) {
                    sp++;
                }
                if (*sp) {
                    start = mprAtoi(sp, 10);

                    if ((sp = strchr(sp, '-')) != 0) {
                        end = mprAtoi(++sp, 10);
                    }
                    if ((sp = strchr(sp, '/')) != 0) {
                        /*
                         *  Note this is not the content length transmitted, but the original size of the input of which 
                         *  the client is transmitting only a portion.
                         */
                        size = mprAtoi(++sp, 10);
                    }
                }
                if (start < 0 || end < 0 || size < 0 || end <= start) {
                    maFailRequest(conn, MPR_HTTP_CODE_RANGE_NOT_SATISFIABLE, "Bad content range");
                    continue;
                }
                req->inputRange = maCreateRange(conn, start, end);

            } else if (strcmp(key, "CONTENT_TYPE") == 0) {
                req->mimeType = value;

            } else if (strcmp(key, "COOKIE") == 0) {
                if (req->cookie && *req->cookie) {
                    mprAllocStrcat(req, &req->cookie, -1, 0, req->cookie, "; ", value, 0);
                } else {
                    req->cookie = value;
                }

            } else if (strcmp(key, "CONNECTION") == 0) {
                req->connection = value;
                if (mprStrcmpAnyCase(value, "KEEP_ALIVE") == 0) {
                    /* Nothing to do */
                    ;

                } else if (mprStrcmpAnyCase(value, "CLOSE") == 0) {
                    conn->keepAliveCount = 0;
                }
                if (!host->keepAlive) {
                    conn->keepAliveCount = 0;
                }
            }
            break;

        case 'F':
            req->forwarded = value;
            break;

        case 'H':
            if (strcmp(key, "HOST") == 0) {
                req->hostName = value;
                address = conn->address;
                if (maIsNamedVirtualHostAddress(address)) {
                    hp = maLookupVirtualHost(address, value);
                    if (hp == 0) {
                        maFailRequest(conn, 404, "No host to serve request. Searching for %s", value);
                        continue;
                    }
                    req->host = hp;
#if UNUSED && TODO
                    /*
                     *  Reassign this request to a new host
                     */
                    host->removeRequest(this);
                    host = hp;
                    host->insertRequest(this);
#endif
                }
            }
            break;

        case 'I':
            if ((strcmp(key, "IF_MODIFIED_SINCE") == 0) || (strcmp(key, "IF_UNMODIFIED_SINCE") == 0)) {
                MprTime     newDate = 0;
                char        *cp;
                bool        ifModified = (key[3] == 'M');

                if ((cp = strchr(value, ';')) != 0) {
                    *cp = '\0';
                }
                if (mprParseTime(conn, &newDate, value) < 0) {
                    mprAssert(0);
                    break;
                }
                if (newDate) {
                    setIfModifiedDate(conn, newDate, ifModified);
                    req->flags |= MA_REQ_IF_MODIFIED;
                }

            } else if ((strcmp(key, "IF_MATCH") == 0) || (strcmp(key, "IF_NONE_MATCH") == 0)) {
                char    *word, *tok;
                bool    ifMatch = key[3] == 'M';

                if ((tok = strchr(value, ';')) != 0) {
                    *tok = '\0';
                }

                req->ifMatch = ifMatch;
                req->flags |= MA_REQ_IF_MODIFIED;

                value = mprStrdup(conn, value);
                word = mprStrTok(value, " ,", &tok);
                while (word) {
                    addMatchEtag(conn, word);
                    word = mprStrTok(0, " ,", &tok);
                }
                mprFree(value);

            } else if (strcmp(key, "IF_RANGE") == 0) {
                char    *word, *tok;

                if ((tok = strchr(value, ';')) != 0) {
                    *tok = '\0';
                }

                req->ifMatch = 1;
                req->flags |= MA_REQ_IF_MODIFIED;

                value = mprStrdup(conn, value);
                word = mprStrTok(value, " ,", &tok);
                while (word) {
                    addMatchEtag(conn, word);
                    word = mprStrTok(0, " ,", &tok);
                }
                mprFree(value);
            }
            break;

        case 'P':
            if (strcmp(key, "PRAGMA") == 0) {
                req->pragma = value;
            }
            break;

        case 'R':
            if (strcmp(key, "RANGE") == 0) {
                if (!parseRange(conn, value)) {
                    maFailRequest(conn, MPR_HTTP_CODE_RANGE_NOT_SATISFIABLE, "Bad range");
                }
            } else if (strcmp(key, "REFERER") == 0) {
                req->referer = value;
            }
            break;

        case 'T':
            if (strcmp(key, "TRANSFER_ENCODING") == 0) {
                mprStrLower(value);
                if (strcmp(value, "chunked") == 0) {
                    req->flags |= MA_REQ_CHUNKED;
                }
            }
            break;
        
#if BLD_DEBUG
        case 'X':
            if (strcmp(key, "X_APPWEB_CHUNK_SIZE") == 0) {
                mprStrUpper(value);
                resp->chunkSize = atoi(value);
                if (resp->chunkSize <= 0) {
                    resp->chunkSize = 0;
                } else if (resp->chunkSize > conn->http->limits.maxChunkSize) {
                    resp->chunkSize = conn->http->limits.maxChunkSize;
                }
            }
            break;
#endif

        case 'U':
            if (strcmp(key, "USER_AGENT") == 0) {
                req->userAgent = value;
            }
            break;
        }
    }
    mprAdjustBufStart(content, 2);

    maMatchHandler(conn);
    
    return 1;
}


/*
 *  Process post or put content data. Packet will be null if the client closed the connection to signify end of data.
 */
static void processContent(MaConn *conn, MaPacket *packet)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaQueue         *q;
    MprBuf          *content;
    int             nbytes;


    req = conn->request;
    resp = conn->response;
    q = &resp->queue[MA_QUEUE_RECEIVE];

    if (conn->requestFailed) {
        conn->canProceed = 0;
        mprFree(packet);
        return;
    }

    /*
     *  Transfer ownership of the packet. If it contains header data for the next pipelined request, it will be split below.
     */
    mprStealBlock(resp, packet);
    conn->input = 0;

    /*
     *  Packet will be null if the client closed the connection to signify end of data
     */
    if (packet) {
        content = packet->content;
        nbytes = min(req->remainingContent, mprGetBufLength(content));
#if BLD_DEBUG
        if (mprGetLogLevel(conn) >= 5) {
            traceContent(conn, packet);
        }
#endif
        if (nbytes > 0) {
            mprAssert(packet->count > 0);
            req->remainingContent -= nbytes;
            req->receivedContent += nbytes;

            if (req->receivedContent >= conn->host->limits->maxBody) {
                conn->keepAliveCount = 0;
                maFailConnection(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, 
                    "Request content body is too big %d vs limit %d", 
                    req->receivedContent, conn->host->limits->maxBody);
                return;
            } 

            if (req->remainingContent == 0 && mprGetBufLength(packet->content) > nbytes) {
                /*
                 *  Looks like this packet contains the header of the next request. Split the packet and put back
                 *  the next request header onto the connection input queue.
                 */
                conn->input = maSplitPacket(conn, packet, mprGetBufLength(content) - nbytes);
            }
            if ((q->count + packet->count) > q->max) {
                conn->keepAliveCount = 0;
                maFailConnection(q->conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, "Too much body data");
                return;
            }
            if (!conn->requestFailed) {
                packet->count = mprGetBufLength(packet->content);
                maPutNext(q, packet);
            }
        }
    }

    if (packet == 0 || req->remainingContent == 0) {
        /*
         *  End of input. Send a zero packet EOF signal and enable the handler send queue.
         */
        if (req->remainingContent > 0 && conn->protocol > 0) {
            maFailConnection(conn, MPR_HTTP_CODE_COMMS_ERROR, "Insufficient content data sent with request");

        } else {
            maPutNext(q, maCreateEndPacket(conn));
            conn->state = MPR_HTTP_STATE_PROCESSING;
            maRunPipeline(conn);
        }

    } else {
        conn->canProceed = 0;
        maServiceQueues(conn);
    }
}


/*
 *  Complete the request and return true if there is a pipelined request following
 */
static bool processCompletion(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaPacket    *packet;
    bool        more;

    req = conn->request;
    resp = conn->response;

    maLogRequest(conn);
    maCloseStage(conn);

#if BLD_DEBUG
    mprAssert((conn->arena->allocBytes / 1024) < 20000);
    mprLog(req, 7, "Request complete used %,d K, conn usage %,d K, mpr usage %,d K, page usage %,d K", 
        req->arena->allocBytes / 1024, conn->arena->allocBytes / 1024, mprGetMpr(conn)->heap.allocBytes / 1024, 
        mprGetMpr(conn)->pageHeap.allocBytes / 1024);
//  mprPrintAllocReport(mprGetMpr(conn), "Before completing request");
#endif

    packet = conn->input;
    more = packet && (mprGetBufLength(packet->content) > 0);
    mprAssert(!more || mprGetParent(packet) == conn);

    /*
     *  This will free the request and response and cause maResetConn to run which will reset the state and cleanse the conn
     *  The connection will not be closed unless keepAliveCount is zero when returning to ioEvent in conn.c 
     */
    mprFree(req->arena);

    return more;
}


static void processChunk(MaConn *conn, MaPacket *packet)
{
    MaRequest       *req;
    MprBuf          *content;
    int             nbytes, boundaryLen;

    req = conn->request;

    if (packet == 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Connection closed before request fully read");
        return;
    }
    content = packet->content;
    nbytes = min(req->remainingContent, mprGetBufLength(content));

    if (!getChunkSize(conn, content, &boundaryLen, &req->remainingChunk)) {
        return;
    }

    mprAdjustBufStart(content, boundaryLen);

    if (req->remainingChunk == 0) {
        /*
         *  Received all the data. Discard last "\r\n" 
         */
        mprAdjustBufEnd(content, 2);
        conn->state = MPR_HTTP_STATE_PROCESSING;
        maRunPipeline(conn);
        
    } else {
        conn->state = MPR_HTTP_STATE_CONTENT;
    }
}


#if BLD_DEBUG
static void traceContent(MaConn *conn, MaPacket *packet)
{
    MprBuf      *content;
    char        *data, *buf;
    int         len;

    content = packet->content;

    len = mprGetBufLength(content);
    buf = mprGetBufStart(content);

    data = mprAlloc(conn, len + 1);
    memcpy(data, buf, len);
    data[len] = '\0';
    mprRawLog(conn, 5, "@@@ Content =>\n%s\n", data);
    mprFree(data);
}
#endif


static bool getChunkSize(MaConn *conn, MprBuf *buf, int *boundaryLen, int *size)
{
    char    *start, *nextContent;
    int     available;

    /*
     *  Must at least have "\r\nDIGIT\r\n"
     */
    start = mprGetBufStart(buf), 
    available = (int) (mprGetBufEnd(buf) - start);
    mprAssert(available >= 0);

    if (available < 5) {
        conn->canProceed = 0;
        return 0;
    }

    /*
     *  Chunk delimiter is: "\r\nHEX_COUNT; chunk length DECIMAL_COUNT\r\n". The "; chunk length DECIMAL_COUNT is optional.
     *  Step over the existing content and get the next chunk count.
     */
    if (start[0] != '\r' || start[1] != '\n') {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad chunk specification");
        return 0;
    }
    nextContent = mprStrnstr(start + 2, "\r\n", available);
    if (nextContent == 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad chunk specification");
        return 0;
    }
    nextContent += 2;

    *boundaryLen = (int) (nextContent - start);
    if (*boundaryLen < 0 || *boundaryLen > 256) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad boundary length");
        return 0;
    }

    *size = mprAtoi(start + 2, 16);
    if (*size < 0) {
        maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad chunk size");
        return 0;
    }
    return 1;
}


void maFailRequest(MaConn *conn, int code, cchar *fmt, ...)
{
    MaResponse  *resp;
    MaRequest   *req;
    va_list     args;
    cchar       *url, *status;
    char        *msg, *filename;

    mprAssert(fmt);
    
    if (conn->requestFailed) {
        return;
    }

    req = conn->request;
    resp = conn->response;
    resp->code = code;

    msg = 0;
    va_start(args, fmt);
    mprAllocVsprintf(resp, &msg, MA_BUFSIZE, fmt, args);
    va_end(args);

    //  TODO - should we only do one error per request?
  
    if (resp == 0 || req == 0) {
        mprLog(resp, 2, "\"%s\", code %d: %s.", mprGetHttpCodeString(conn, code), code, msg);

    } else {
        filename = resp->filename ? resp->filename : 0;

        if (code != 711) {
            mprLog(resp, 2, "Error: \"%s\", code %d for URI \"%s\", file \"%s\": %s.", 
                mprGetHttpCodeString(conn, code), code, req->url ? req->url : "", filename ? filename : "", msg);
        }

        /*
         *  Use an error document rather than standard error boilerplate.
         */
        if (req->location) {
            url = maLookupErrorDocument(req->location, code);
            if (url && *url) {
                maRedirect(conn, 302, url);
                return;
            }
        }

        /*
         *  If the headers have already been filled, this alternate response body will be ignored.
         */
        if (resp->altBody == 0) {
            status = mprGetHttpCodeString(conn, code);
            /*
             *  For security, do not emit the "msg" value. 
             */
            mprAllocSprintf(conn->response, &resp->altBody, -1, 
                "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
                "<html><head><title>Document Error: %s</title></head>\r\n"
                "<body><h2>Access Error: %d -- %s</h2></body>\r\n</html>\r\n",
                status, code, status);
        }
    }
    mprFree(msg);

    resp->flags |= MA_RESP_NO_BODY;
    conn->requestFailed = 1;
}


/*
 *  Stop all requests on the current connection. Fail the current request and the processing pipeline. Force a connection 
 *  closure as in most cases, the client is seriously messed up.
 */
void maFailConnection(MaConn *conn, int code, cchar *fmt, ...)
{
    va_list     args;
    char        *msg;

    mprAssert(fmt);

    conn->state = MPR_HTTP_STATE_COMPLETE;
    conn->canProceed = 0;

    if (conn->abandonConnection) {
        return;
    }
    conn->abandonConnection = 1;

    va_start(args, fmt);
    msg = 0;
    mprAllocVsprintf(conn->response, &msg, MA_BUFSIZE, fmt, args);
    va_end(args);

    maFailRequest(conn, code, "%s", msg);
    mprFree(msg);

    conn->keepAliveCount = 0;
    maDiscardPipeData(conn);
}


//  TODO - reorder the routines to the end of the file
int maSetRequestUri(MaConn *conn, cchar *uri)
{
    MaRequest   *req;
    int         len;

    req = conn->request;

    /*
     *  We parse (tokenize) the request uri first. Then we decode and lastly we validate the URI path portion.
     *  This allows URLs to have '?' in the URL. We descape and validate insitu.
     */
    req->parsedUri = mprParseUri(req, uri);
    if (req->parsedUri == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    conn->response->extension = req->parsedUri->ext;

    /*
     *  Decode the URI path portion
     *  TODO - is this right? What about decoding the other parts of the URI
     */
    req->url = mprStrdup(req, req->parsedUri->url);
    len = (int) strlen(req->url);
    if (mprUrlDecode(req->url, len + 1, req->url) == 0) {
        return MPR_ERR_BAD_ARGS;
    }

    //  TODO - need to do case insensitive testing on ext and maybe other places See MprFileInfo.caseMatters

    if (mprValidateUrl(req->url) == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    return 0;
}


/*
 *  Format is:  Range: bytes=n1-n2,n3-n4,...
 *  Where n1 is first byte pos and n2 is last byte pos
 *
 *  Examples:
 *      Range: 0-49             first 50 bytes
 *      Range: 50-99,200-249    Two 50 byte ranges from 50 and 200
 *      Range: -50              Last 50 bytes
 *      Range: 1-               Skip first byte then emit the rest
 *
 *  Return 1 if more ranges, 0 if end of ranges, -1 if bad range.
 */
static bool parseRange(MaConn *conn, char *value)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaRange     *range, *last, *next;
    char        *tok, *ep;

    req = conn->request;
    resp = conn->response;

    value = mprStrdup(conn, value);
    if (value == 0) {
        return 0;
    }

    /*
     *  Step over the "bytes="
     */
    tok = mprStrTok(value, "=", &value);

    for (last = 0; value && *value; ) {
        range = mprAllocObjZeroed(req, MaRange);
        if (range == 0) {
            return 0;
        }

        /*
         *  A range "-7" will set the start to -1 and end to 8
         */
        tok = mprStrTok(value, ",", &value);
        if (*tok != '-') {
            range->start = mprAtoi(tok, 10);
        } else {
            range->start = -1;
        }
        range->end = -1;

        if ((ep = strchr(tok, '-')) != 0) {
            if (*++ep != '\0') {
                /*
                 *  End is one beyond the range. Makes the math easier.
                 */
                range->end = mprAtoi(ep, 10) + 1;
            }
        }
        if (range->start >= 0 && range->end >= 0) {
            range->len = range->end - range->start;
        }
        if (last == 0) {
            req->ranges = range;
        } else {
            last->next = range;
        }
        last = range;
    }

    /*
     *  Validate ranges
     */
    for (range = req->ranges; range; range = range->next) {
        if (range->end != -1 && range->start >= range->end) {
            return 0;
        }
        if (range->start < 0 && range->end < 0) {
            return 0;
        }
        next = range->next;
        if (range->start < 0 && next) {
            /* This range goes to the end, so can't have another range afterwards */
            return 0;
        }
        if (next) {
            if (next->start >= 0 && range->end > next->start) {
                return 0;
            }
        }
    }
    resp->currentRange = req->ranges;

    return (last) ? 1: 0;
}


/*
 *  Called by connectors to complete a request
 */
void maCompleteRequest(MaConn *conn)
{
    conn->state = MPR_HTTP_STATE_COMPLETE;
}


/*
 *  Connector is write blocked and can't proceed
 */
void maRequestWriteBlocked(MaConn *conn)
{
    conn->canProceed = 0;
}


void maSetNoKeepAlive(MaConn *conn)
{
    conn->keepAliveCount = 0;
}


//  TODO - move and 
bool maContentNotModified(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    bool        same;

    req = conn->request;
    resp = conn->response;

    if (req->flags & MA_REQ_IF_MODIFIED) {
        /*
         *  If both checks, the last modification time and etag, claim that the request doesn't need to be
         *  performed, skip the transfer.
         */
        //  TODO - need to check if fileInfo is actually set.
        same = matchModified(conn, resp->fileInfo.mtime) && matchEtag(conn, resp->etag);

        if (req->ranges && !same) {
            /*
             *  Need to transfer the entire resource
             */
            mprFree(req->ranges);
            req->ranges = 0;
        }
        return same;
    }
    return 0;
}


MaRange *maCreateRange(MaConn *conn, int start, int end)
{
    MaRange     *range;

    range = mprAllocObjZeroed(conn->request, MaRange);
    if (range == 0) {
        return 0;
    }
    range->start = start;
    range->end = end;
    range->len = end - start;

    return range;
}


static void addMatchEtag(MaConn *conn, char *etag)
{
    MaRequest   *req;

    req = conn->request;

    if (req->etags == 0) {
        req->etags = mprCreateList(req);
    }
    mprAddItem(req->etags, etag);
}


/*
 *  Return TRUE if the client's cached copy matches an entity's etag.
 */
//  TODO was RequestMatch::matches

static bool matchEtag(MaConn *conn, char *requestedEtag)
{
    MaRequest   *req;
    char        *tag;
    int         next;

    req = conn->request;

    if (req->etags == 0) {
        return 1;
    }
    if (requestedEtag == 0) {
        return 0;
    }

    for (next = 0; (tag = mprGetNextItem(req->etags, &next)) != 0; ) {
        if (strcmp(tag, requestedEtag) == 0) {
            return (req->ifMatch) ? 0 : 1;
        }
    }
    return (req->ifMatch) ? 1 : 0;
}


/*
 *  If an IF-MODIFIED-SINCE was specified, then return true if the resource has not been modified. If using
 *  IF-UNMODIFIED, then return true if the resource was modified.
 */
static bool matchModified(MaConn *conn, MprTime time)
{
    MaRequest   *req;

    req = conn->request;

    if (req->since == 0) {
        /*  If-Modified or UnModified not supplied. */
        return 1;
    }

    if (req->ifModified) {
        /*
         *  Return true if the file has not been modified.
         */
        return !(time > req->since);

    } else {
        /*
         *  Return true if the file has been modified.
         */
        return (time > req->since);
    }
}


static void setIfModifiedDate(MaConn *conn, MprTime when, bool ifMod)
{
    MaRequest   *req;

    req = conn->request;
    req->since = when;
    req->ifModified = ifMod;
}


/*
 *  Get the next input token. The content buffer is advanced to the next token. This routine always returns a non-zero token. 
 *  The empty string means the delimiter was not found.
 */
static char *getToken(MaConn *conn, cchar *delim)
{
    MprBuf  *buf;
    char    *token, *nextToken;
    int     len;

    buf = conn->input->content;
    len = mprGetBufLength(buf);
    if (len == 0) {
        return "";
    }

    token = mprGetBufStart(buf);
    nextToken = mprStrnstr(mprGetBufStart(buf), delim, len);
    if (nextToken) {
        *nextToken = '\0';
        len = (int) strlen(delim);
        nextToken += len;
        buf->start = (uchar*) nextToken;

    } else {
        buf->start = (uchar*) mprGetBufEnd(buf);
    }
    return token;
}


cchar *maGetCookies(MaConn *conn)
{
    return conn->request->cookie;
}


//  TODO - move
void maSetRequestUser(MaConn *conn, cchar *user)
{
    MaRequest   *req;

    req = conn->request;

    mprFree(req->user);
    req->user = mprStrdup(conn->request, user);
}


//  TODO - move
void maSetRequestGroup(MaConn *conn, cchar *group)
{
    MaRequest   *req;

    req = conn->request;

    mprFree(req->group);
    req->group = mprStrdup(conn->request, group);
}



cchar *maGetQueryString(MaConn *conn)
{
    MaRequest   *req;

    req = conn->request;

    return conn->request->parsedUri->query;
}


/*
 *  @copy   default
 *
 *  Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
 *  Copyright (c) Michael O'Brien, 1993-2009. All Rights Reserved.
 *
 *  This software is distributed under commercial and open source licenses.
 *  You may use the GPL open source license described below or you may acquire
 *  a commercial license from Embedthis Software. You agree to be fully bound
 *  by the terms of either license. Consult the LICENSE.TXT distributed with
 *  this software for full details.
 *
 *  This software is open source; you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by the
 *  Free Software Foundation; either version 2 of the License, or (at your
 *  option) any later version. See the GNU General Public License for more
 *  details at: http://www.embedthis.com/downloads/gplLicense.html
 *
 *  This program is distributed WITHOUT ANY WARRANTY; without even the
 *  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  This GPL license does NOT permit incorporating this software into
 *  proprietary programs. If you are unable to comply with the GPL, you must
 *  acquire a commercial license to use this software. Commercial licenses
 *  for this software and support services are available from Embedthis
 *  Software at http://www.embedthis.com
 *
 *  @end
 */

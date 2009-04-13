/*
 *  response.c
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/***************************** Forward Declarations ***************************/

static int destroyResponse(MaResponse *resp);
static void putFormattedHeader(MaPacket *packet, cchar *key, cchar *fmt, ...);
static void putHeader(MaPacket *packet, cchar *key, cchar *value);

/*********************************** Code *************************************/

MaResponse *maCreateResponse(MaConn *conn)
{
    MaResponse  *resp;
    MaHttp      *http;

    http = conn->http;

    resp = mprAllocObjWithDestructorZeroed(conn->request->arena, MaResponse, destroyResponse);
    if (resp == 0) {
        return 0;
    }

    resp->conn = conn;
    resp->code = MPR_HTTP_CODE_OK;
    resp->mimeType = "text/html";
    resp->handler = http->passHandler;
    resp->length = -1;
    resp->entityLength = -1;
    resp->chunkSize = -1;

    resp->headers = mprCreateHash(resp, MA_HEADER_HASH_SIZE);

    maInitQueue(http, &resp->queue[MA_QUEUE_SEND], "responseSendHead");
    maInitQueue(http, &resp->queue[MA_QUEUE_RECEIVE], "responseReceiveHead");

    return resp;
}


static int destroyResponse(MaResponse *resp)
{
    MaConn      *conn;

    conn = resp->conn;
    mprLog(conn, 5, "destroyResponse");
    maCloseStage(conn);

    return 0;
}


/*
 *  TODO OPT - how to speed this whole routine up. Can some of this be pre-computed?
 */
void maFillHeaders(MaConn *conn, MaPacket *packet)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host;
    MaRange         *range;
    MprHash         *hp;
    MprBuf          *buf;

    mprAssert(packet->flags == MA_PACKET_HEADER);

    req = conn->request;
    resp = conn->response;
    host = req->host;
    buf = packet->content;

    if (resp->flags & MA_RESP_HEADERS_CREATED) {
        return;
    }    
    if (req->method ==  MA_REQ_TRACE || req->method == MA_REQ_OPTIONS) {
        maTraceOptions(conn);
    }

    mprPutStringToBuf(buf, req->httpProtocol);
    mprPutCharToBuf(buf, ' ');
    mprPutIntToBuf(buf, resp->code);
    mprPutCharToBuf(buf, ' ');
    mprPutStringToBuf(buf, mprGetHttpCodeString(resp, resp->code));
    mprPutStringToBuf(buf, "\r\n");

    mprLog(conn, 2, "    => %s %d %s", req->httpProtocol, resp->code, mprGetHttpCodeString(resp, resp->code));

    putHeader(packet, "Date", req->host->currentDate);
    putHeader(packet, "Server", MA_SERVER_NAME);

    if (resp->flags & MA_RESP_DONT_CACHE) {
        putHeader(packet, "Cache-Control", "no-cache");
    }

    if (resp->etag) {
        putFormattedHeader(packet, "ETag", "%s", resp->etag);
    }

    if (resp->altBody) {
        resp->length = (int) strlen(resp->altBody);
    }

    if (resp->chunkSize > 0) {
        if (!(req->method & MA_REQ_HEAD)) {
            maSetHeader(conn, 0, "Transfer-Encoding", "chunked");
        }

    } else if (resp->length > 0) {
        putFormattedHeader(packet, "Content-Length", "%d", resp->length);
    }

    if (req->ranges) {
        if (req->ranges->next == 0) {
            range = req->ranges;
            if (resp->entityLength > 0) {
                putFormattedHeader(packet, "Content-Range", "bytes %d-%d/%d", range->start, range->end, resp->entityLength);
            } else {
                putFormattedHeader(packet, "Content-Range", "bytes %d-%d/*", range->start, range->end);
            }
        } else {
            putFormattedHeader(packet, "Content-Type", "multipart/byteranges; boundary=%s", resp->rangeBoundary);
        }
        putHeader(packet, "Accept-Ranges", "bytes");

        //  TODO - does not look right
    } else if (resp->code != MPR_HTTP_CODE_MOVED_TEMPORARILY) {
        putHeader(packet, "Content-Type", (resp->mimeType) ? resp->mimeType : "text/html");
    }

    if (conn->keepAliveCount-- > 0) {
        putHeader(packet, "Connection", "keep-alive");
        putFormattedHeader(packet, "Keep-Alive", "timeout=%d, max=%d", host->keepAliveTimeout / 1000, conn->keepAliveCount);
    } else {
        putHeader(packet, "Connection", "close");
    }

    /*
     *  Output any remaining custom headers
     */
    hp = mprGetFirstHash(resp->headers);
    while (hp) {
        putHeader(packet, hp->key, hp->data);
        hp = mprGetNextHash(resp->headers, hp);
    }

    if (resp->chunkSize <= 0 || resp->altBody) {
        mprPutStringToBuf(buf, "\r\n");
    }
    if (resp->altBody) {
        mprPutStringToBuf(buf, resp->altBody);
        maDiscardData(resp->queue[MA_QUEUE_SEND].nextQ, 0);
    }
    packet->count = mprGetBufLength(buf);
    resp->headerSize = packet->count;

    resp->flags |= MA_RESP_HEADERS_CREATED;

    mprLog(conn, 3, "\n@@@ Response => \n%s", mprGetBufStart(buf));
}


void maTraceOptions(MaConn *conn)
{
    MaResponse  *resp;
    MaRequest   *req;
    int         flags;

    if (conn->requestFailed) {
        return;
    }
    resp = conn->response;
    req = conn->request;

    if (req->method & MA_REQ_TRACE) {
        if (req->host->flags & MA_HOST_NO_TRACE) {
            resp->code = MPR_HTTP_CODE_NOT_ACCEPTABLE;
            maFormatBody(conn, "Trace Request Denied", "<p>The TRACE method is disabled on this server.</p>");
        } else {
            mprAllocSprintf(resp, &resp->altBody, -1, "%s %s %s\r\n", req->methodName, req->parsedUri->originalUri, 
                req->httpProtocol);
        }

    } else if (req->method & MA_REQ_OPTIONS) {

        if (resp->handler == 0) {
            maSetHeader(conn, 0, "Allow", "OPTIONS,TRACE");

        } else {
            flags = resp->handler->flags;
            maSetHeader(conn, 0, "Allow", "OPTIONS,TRACE%s%s%s",
                (flags & MA_STAGE_GET) ? ",GET" : "",
                (flags & MA_STAGE_HEAD) ? ",HEAD" : "",
                (flags & MA_STAGE_POST) ? ",POST" : "",
                (flags & MA_STAGE_PUT) ? ",PUT" : "",
                (flags & MA_STAGE_DELETE) ? ",DELETE" : "");
        }
        resp->length = 0;
    }
}


static void putFormattedHeader(MaPacket *packet, cchar *key, cchar *fmt, ...)
{
    va_list     args;
    char        *value;

    va_start(args, fmt);
    mprAllocVsprintf(packet, &value, MA_MAX_HEADERS, fmt, args);
    va_end(args);

    putHeader(packet, key, value);
    mprFree(value);
}


static void putHeader(MaPacket *packet, cchar *key, cchar *value)
{
    MaResponse  *resp;
    MprBuf      *buf;

    buf = packet->content;
    resp = packet->conn->response;

    mprPutStringToBuf(buf, key);
    mprPutStringToBuf(buf, ": ");
    if (value) {
        mprPutStringToBuf(buf, value);
    }
    mprPutStringToBuf(buf, "\r\n");
}


int maFormatBody(MaConn *conn, cchar *title, cchar *fmt, ...)
{
    MaResponse  *resp;
    va_list     args;
    char        *body;
    int         len;

    resp = conn->response;
    mprAssert(resp->altBody == 0);

    va_start(args, fmt);

    //  TODO - should this string be HTML encoded?
    mprAllocVsprintf(resp, &body, MA_MAX_HEADERS, fmt, args);

    len = mprAllocSprintf(resp, &resp->altBody, -1,
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
        "<html><head><title>%s</title></head>\r\n"
        "<body>\r\n%s\r\n</body>\r\n</html>\r\n",
        title, body);
    mprFree(body);
    va_end(args);
    return len;
}


/*
 *  Redirect the user to another web page. The targetUri may or may not have a scheme.
 */
void maRedirect(MaConn *conn, int code, cchar *targetUri)
{
    MaResponse  *resp;
    MaRequest   *req;
    MaHost      *host;
    MprUri      *target, *prev;
    char        path[MA_MAX_URL];
    char        *uri, *dir, *cp;

    mprAssert(targetUri);

    req = conn->request;
    resp = conn->response;
    host = req->host;

    mprLog(conn, 3, "redirect %d %s", code, targetUri);

    uri = 0;
    resp->code = code;

    prev = req->parsedUri;
    target = mprParseUri(resp, targetUri);

    if (strstr(targetUri, "://") == 0) {
        /*
         *  Use host->name to get the real ServerName from the config file
         */
        if (target->url[0] == '/') {
            uri = mprFormatUri(resp, prev->scheme, host->name, prev->port, target->url, target->query);

        } else {
            /*
             *  Redirection to a file in the same directory as the previous request.
             */
            dir = mprStrdup(resp, req->url);
            if ((cp = strrchr(dir, '/')) != 0) {
                *cp = '\0';
            }
            mprSprintf(path, sizeof(path), "%s/%s", dir, target->url);
            uri = mprFormatUri(resp, prev->scheme, host->name, prev->port, path, target->query);
        }
        targetUri = uri;
    }

    maSetHeader(conn, 0, "Location", "%s", targetUri);
    mprAssert(resp->altBody == 0);
    mprAllocSprintf(resp, &resp->altBody, -1,
        "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
        "<html><head><title>%s</title></head>\r\n"
        "<body><h1>%s</h1>\r\n</H1>\r\n<p>The document has moved <a href=\"%s\">here</a>.</p>\r\n"
        "<address>%s at %s Port %d</address></body>\r\n</html>\r\n",
        mprGetHttpCodeString(conn, code), mprGetHttpCodeString(conn, code), targetUri,
        MA_SERVER_NAME, host->name, prev->port);

    mprFree(uri);

    /*
     *  Pretent the request failed. This will prevent further processing of the pipeline.
     */
    conn->requestFailed = 1;
}


void maDontCacheResponse(MaConn *conn)
{
    conn->response->flags |= MA_RESP_DONT_CACHE;
}


void maSetHeader(MaConn *conn, bool allowMultiple, cchar *key, cchar *fmt, ...)
{
    MaResponse      *resp;
    char            *value;
    va_list         vargs;

    resp = conn->response;

    va_start(vargs, fmt);
    mprAllocVsprintf(resp, &value, MA_MAX_HEADERS, fmt, vargs);

    if (allowMultiple) {
        mprAddDuplicateHash(resp->headers, key, value);
    } else {
        //  TODO - this should be doing case mapping on the key
        mprAddHash(resp->headers, key, value);
    }
}


void maSetCookie(MaConn *conn, cchar *name, cchar *value, int lifetime, cchar *path, bool secure)
{
    MaResponse  *resp;
    struct tm   tm;
    time_t      when;
    char        dateStr[64];

    resp = conn->response;

    if (path == 0) {
        path = "/";
    }

    if (lifetime > 0) {
        when = conn->time + lifetime * MPR_TICKS_PER_SEC;
        mprGmtime(resp, &tm, when);
        mprRfctime(resp, dateStr, sizeof(dateStr), &tm);

        /*
         *  Other keywords:
         *      Domain=%s
         */
        maSetHeader(conn, 1, "Set-Cookie", "%s=%s; path=%s; Expires=%s;%s", name, value, path, dateStr, 
            secure ? " secure" : "");

    } else {
        maSetHeader(conn, 1, "Set-Cookie", "%s=%s; path=%s;%s", name, value, path, secure ? " secure" : "");
    }
    maSetHeader(conn, 0, "Cache-control", "no-cache=\"set-cookie\"");
}


void maSetEntityLength(MaConn *conn, int len)
{
    MaRequest       *req;
    MaResponse      *resp;

    resp = conn->response;
    req = conn->request;

    resp->entityLength = len;
    if (req->ranges == 0) {
        resp->length = len;
    }
}


void maSetResponseCode(MaConn *conn, int code)
{
    conn->response->code = code;
}


void maSetResponseMimeType(MaConn *conn, cchar *mimeType)
{
    MaResponse      *resp;

    resp = conn->response;

    /*  Can't free old mime type, as it is sometimes a literal constant */
    resp->mimeType = mprStrdup(resp, mimeType);
}


void maOmitResponseBody(MaConn *conn)
{
    if (conn->response) {
        conn->response->flags |= MA_RESP_NO_BODY;
    }
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

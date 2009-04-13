/*
 *  pipeline.c -- HTTP pipeline processing.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/***************************** Forward Declarations ***************************/

static char *addIndex(MaConn *conn, char *path, int pathsize, cchar *index);
static char *getExtension(MaConn *conn);
static MaStage *findHandlerByExtension(MaConn *conn);
static MaStage *findLocationHandler(MaConn *conn);
static char *makeFilename(MaConn *conn, MaAlias *alias, cchar *url, bool skipAliasPrefix);
static bool mapToFile(MaConn *conn, bool *rescan);
static bool matchFilter(MaConn *conn, MaFilter *filter);
static void openQ(MaQueue *q);
static void processDirectory(MaConn *conn, bool *rescan);
static void setEnv(MaConn *conn);
static void setPathInfo(MaConn *conn);

/*********************************** Code *************************************/

/*
 *  Find the matching handler for a request. If any errors occur, the pass handler is used to pass errors onto the 
 *  net/sendfile connectors to send to the client. This routine may rewrite the request URI and may redirect the request.
 */
void maMatchHandler(MaConn *conn)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host;
    MaAlias         *alias;
    MaStage         *handler;
    bool            rescan;
    int             loopCount;

    req = conn->request;
    resp = conn->response;
    host = req->host;

    /*
     *  Find the alias that applies for this url. There is always a catch-all alias for the document root.
     */
    alias = req->alias = maGetAlias(host, req->url);
    mprAssert(alias);
    if (alias->redirectCode) {
        // TODO - what about internal redirects?
        maRedirect(conn, alias->redirectCode, alias->uri);
        return;
    }

    if (conn->requestFailed || conn->request->method & (MA_REQ_OPTIONS | MA_REQ_TRACE)) {
        handler = conn->http->passHandler;
        return;
    }

    /*
     *  Get the best (innermost) location block and see if a handler is explicitly set for that location block.
     *  Possibly rewrite the url and retry.
     */
    loopCount = MA_MAX_REWRITE;
    do {
        rescan = 0;
        if ((handler = findLocationHandler(conn)) == 0) {
            /*
             *  Didn't find a location block handler, so try to match by extension and by handler match() routines.
             *  This may invoke processDirectory which may redirect and thus require reprocessing -- hence the loop.
             */
            handler = findHandlerByExtension(conn);
        }

        if (handler && !(handler->flags & MA_STAGE_VIRTUAL)) {
            if (!mapToFile(conn, &rescan)) {
                return;
            }
        }
    } while (handler && rescan && loopCount-- > 0);

    if (handler == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_METHOD, "Requested method %s not supported for URL: %s", 
            req->methodName, req->url);
        handler = conn->http->passHandler;
    }
    resp->handler = handler;

    mprLog(resp, 4, "Select handler: \"%s\" for \"%s\"", handler->name, req->url);

    setEnv(conn);
}


/*
 *  Create stages for the request pipeline.
 */
void maCreatePipeline(MaConn *conn)
{
    MaHttp          *http;
    MaHost          *host;
    MaResponse      *resp;
    MaRequest       *req;
    MaStage         *handler;
    MaLocation      *location;
    MaStage         *stage, *connector;
    MaFilter        *filter;
    MaQueue         *q, *qhead, *rq, *rqhead;
    int             next;

    req = conn->request;
    resp = conn->response;
    host = req->host;
    location = req->location;
    handler = resp->handler;
    http = conn->http;

    mprAssert(req);
    mprAssert(location->outputStages);

    /*
     *  Create the output pipeline for this request. Handler first, then filters, connector last.
     */
    resp->outputPipeline = mprCreateList(resp);

    /*
     *  Add the handler and filters. Switch to the pass handler if any errors have occurred so far. Only add the 
     *  filters if the request has not failed.
     */
    if (conn->requestFailed) {
        resp->handler = http->passHandler;
        mprAddItem(resp->outputPipeline, resp->handler);

    } else {
        mprAddItem(resp->outputPipeline, resp->handler);
        for (next = 0; (filter = mprGetNextItem(location->outputStages, &next)) != 0; ) {
            if (filter->stage == http->authFilter) {
                if (req->auth->type == 0 && req->auth->type == 0) {
                    continue;
                }
            }
            if (filter->stage == http->rangeFilter && (req->ranges == 0 || handler == http->fileHandler)) {
                continue;
            }
            if ((filter->stage->flags & MA_STAGE_ALL & req->method) == 0) {
                continue;
            }
            /*
             *  Remove the chunk filter chunking if it is explicitly turned off vi a the X_APPWEB_CHUNK_SIZE header setting
             *  the chunk size to zero. Also remove if using the fileHandler which always knows the entity length and an
             *  explicit chunk size has not been requested.
             */
            if (filter->stage == http->chunkFilter) {
                if ((handler == http->fileHandler && resp->chunkSize < 0) || resp->chunkSize == 0) {
                    continue;
                }
            }
            if (matchFilter(conn, filter)) {
                mprAddItem(resp->outputPipeline, filter->stage);
            }
        }
    }
    
    connector = location->connector;
#if BLD_FEATURE_SEND
    if (resp->handler == http->fileHandler && connector == http->netConnector && 
        http->sendConnector && !req->ranges && !host->secure) {
        /*
         *  Switch (transparently) to the send connector if serving whole static file content via the net connector
         */
        connector = http->sendConnector;
    }
#endif
    resp->connector = connector;
    if ((connector->flags & MA_STAGE_ALL & req->method) == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Connector \"%s\" does not support the \"%s\" method \"%s\"", 
            connector->name, req->methodName);
        return;
    }
    mprAddItem(resp->outputPipeline, connector);

    /*
     *  Create the outgoing queue heads and open the queues
     */
    q = &resp->queue[MA_QUEUE_SEND];
    for (next = 0; (stage = mprGetNextItem(resp->outputPipeline, &next)) != 0; ) {
        q = maCreateQueue(conn, stage, MA_QUEUE_SEND, q);
    }

    /*
     *  Create the receive pipeline for this request. Connector first, handler last
     */
    if (req->remainingContent > 0) {
        req->inputPipeline = mprCreateList(resp);

        mprAddItem(req->inputPipeline, connector);
        if (!conn->requestFailed) {
            for (next = 0; (filter = mprGetNextItem(location->inputStages, &next)) != 0; ) {
                if (filter->stage == http->authFilter || !matchFilter(conn, filter)) {
                    continue;
                }
                if ((filter->stage->flags & MA_STAGE_ALL & req->method) == 0) {
                    continue;
                }
                mprAddItem(req->inputPipeline, filter->stage);
            }
        }
        mprAddItem(req->inputPipeline, handler);

        /*
         *  Create the incoming queue heads and open the queues.
         */
        q = &resp->queue[MA_QUEUE_RECEIVE];
        for (next = 0; (stage = mprGetNextItem(req->inputPipeline, &next)) != 0; ) {
            q = maCreateQueue(conn, stage, MA_QUEUE_RECEIVE, q);
        }
    }

    /*
     *  Pair up the send and receive queues. NOTE: can't use a stage multiple times.
     */
    qhead = &resp->queue[MA_QUEUE_SEND];
    rqhead = &resp->queue[MA_QUEUE_RECEIVE];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        for (rq = rqhead->nextQ; rq != rqhead; rq = rq->nextQ) {
            if (q->stage == rq->stage) {
                q->pair = rq;
                rq->pair = q;
            }
        }
    }

    /*
     *  Open the queues (keep going on errors)
     */
    qhead = &resp->queue[MA_QUEUE_SEND];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        if (q->open && !(q->flags & MA_QUEUE_OPEN)) {
            q->flags |= MA_QUEUE_OPEN;
            openQ(q);
        }
    }

    if (req->remainingContent > 0) {
        qhead = &resp->queue[MA_QUEUE_RECEIVE];
        for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
            if (q->open && !(q->flags & MA_QUEUE_OPEN)) {
                if (q->pair == 0 || !(q->pair->flags & MA_QUEUE_OPEN)) {
                    q->flags |= MA_QUEUE_OPEN;
                    openQ(q);
                }
            }
        }
    }
}


/*
 *  Invoke the run routine for the handler and then pump the pipeline by servicing all scheduled queues.
 */
bool maRunPipeline(MaConn *conn)
{
    MaQueue     *q;
    
    q = conn->response->queue[MA_QUEUE_SEND].nextQ;
    
    if (q->stage->run) {
        q->stage->run(q);
    }
    return maServiceQueues(conn);
}


/*
 *  Run the queue service routines until there is no more work to be done. NOTE: all I/O is non-blocking.
 */
bool maServiceQueues(MaConn *conn)
{
    MaQueue     *q;
    bool        workDone;

    workDone = 0;
    while (!conn->abandonConnection && (q = maGetNextQueueForService(&conn->serviceq)) != NULL) {
        maServiceQueue(q);
        workDone = 1;
    }
    return workDone;
}


void maDiscardPipeData(MaConn *conn)
{
    MaResponse      *resp;
    MaQueue         *q, *qhead;

    resp = conn->response;

    qhead = &resp->queue[MA_QUEUE_SEND];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        maDiscardData(q, 0);
    }

    qhead = &resp->queue[MA_QUEUE_RECEIVE];
    for (q = qhead->nextQ; q != qhead; q = q->nextQ) {
        maDiscardData(q, 0);
    }
}


static char *addIndex(MaConn *conn, char *urlbuf, int bufsize, cchar *index)
{
    MaRequest       *req;
    char            *indexDelim;

    req = conn->request;

    indexDelim = (req->url[strlen(req->url) - 1] = '/') ? "" : "/";
    
    if (req->parsedUri->query && req->parsedUri->query[0]) {
        mprSprintf(urlbuf, bufsize, "%s%s%s?%s", req->url, indexDelim, index, req->parsedUri->query);
    } else {
        mprSprintf(urlbuf, bufsize, "%s%s%s", req->url, indexDelim, index);
    }
    return urlbuf;
}


static MaStage *checkStage(MaConn *conn, MaStage *stage)
{
    MaRequest   *req;

    req = conn->request;

    if (stage == 0) {
        return 0;
    }
    if ((stage->flags & MA_STAGE_ALL & req->method) == 0) {
        return 0;
    }
    if (stage->match && !stage->match(conn, stage, req->url)) {
        return 0;
    }
    return stage;
}


int maRewriteUri(MaConn *conn) { return 0; }

static MaStage *findLocationHandler(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaLocation  *location;
    int         loopCount;

    req = conn->request;
    resp = conn->response;
    loopCount = MA_MAX_REWRITE;

    do {
        location = req->location = maLookupBestLocation(req->host, req->url);
        mprAssert(location);
        req->auth = location->auth;
        resp->handler = checkStage(conn, location->handler);
    } while (maRewriteUri(conn) && --loopCount > 0);

    return resp->handler;
}


static char *getExtension(MaConn *conn)
{
    MaRequest   *req;
    char        *cp;
    char        *ep, ext[16];

    req = conn->request;

    /*
     *  This is not perfect, but manageable. If a directory after the location prefix has a "." in it, then it may
     *  be mis-interpreted as the extension.
     */
    if ((cp = strchr(&req->url[req->alias->prefixLen], '.')) != 0) {
        cp++;
        for (ep = ext; isalpha((int) *cp) && ep < &ext[sizeof(ext)]; ) {
            *ep++ = *cp++;
        }
        *ep = '\0';
        return mprStrdup(req, ext);
    }
    return "";
}


/*
 *  Search for a handler by request extension. If that fails, use handler custom matching.
 *  If all that fails, return the catch-all handler (fileHandler)
 */
static MaStage *findHandlerByExtension(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    MaStage     *handler;
    MaLocation  *location;
    int         next;

    req = conn->request;
    resp = conn->response;
    location = req->location;
    
    resp->extension = getExtension(conn);

    if (*resp->extension) {
        handler = maGetHandlerByExtension(location, resp->extension);
        if (checkStage(conn, handler)) {
            return handler;
        }
    }

    /*
     *  Failed to match by extension, so perform custom handler matching. May need a filename (dir handler)
     */
    resp->filename = makeFilename(conn, req->alias, req->url, 1);
    for (next = 0; (handler = mprGetNextItem(location->handlers, &next)) != 0; ) {
        if (handler->match && handler->match(conn, handler, req->url)) {
            if (checkStage(conn, handler)) {
                resp->handler = handler;
                return handler;
            }
        }
    }

    /*
     *  Failed to match. Return any catch-all handler.
     */
    handler = maGetHandlerByExtension(location, "");
    if (handler == 0) {
        /*
         *  Could be missing a catch-all in the config file, so invoke the file handler.
         */
        handler = maLookupStage(conn->http, "fileHandler");
    }

    mprAssert(handler);
    //  TODO - should we be setting this here?
    resp->handler = handler;
    
    return checkStage(conn, handler);
}


//  TODO - remove last parameter
static char *makeFilename(MaConn *conn, MaAlias *alias, cchar *url, bool skipAliasPrefix)
{
    char        *cleanPath, *path;
    int         len;

    mprAssert(alias);
    mprAssert(url && *url);

    if (skipAliasPrefix) {
        url += alias->prefixLen;
    }
    while (*url == '/') {
        url++;
    }

    len = (int) strlen(alias->filename);
    if ((path = mprAlloc(conn->request, len + (int) strlen(url) + 2)) == 0) {
        return 0;
    }
    strcpy(path, alias->filename);
    if (*url) {
        path[len++] = '/';
        strcpy(&path[len], url);
    }

    cleanPath = mprCleanFilename(conn, path);
    mprMapDelimiters(conn, cleanPath, '/');
    mprFree(path);

    return cleanPath;
}


static bool mapToFile(MaConn *conn, bool *rescan)
{
    MaRequest   *req;
    MaResponse  *resp;

    req = conn->request;
    resp = conn->response;

    if (resp->filename == 0) {
        resp->filename = makeFilename(conn, req->alias, req->url, 1);
    }
    req->dir = maLookupBestDir(req->host, resp->filename);

    if (req->dir == 0) {
        maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Missing directory block for %s", resp->filename);
        return 0;
    }

    mprAssert(req->dir);

    req->auth = req->dir->auth;

    if (!resp->fileInfo.valid && mprGetFileInfo(conn, resp->filename, &resp->fileInfo) < 0) {
#if UNUSED
        if (req->method & (MA_REQ_GET | MA_REQ_POST)) {
            maFailRequest(conn, MPR_HTTP_CODE_NOT_FOUND, "Can't open document: %s", resp->filename);
            return 0;
        }
#endif
    }
    if (resp->fileInfo.isDir) {
        processDirectory(conn, rescan);
    }
    return 1;
}


/*
 *  Match a filter by extension
 */
static bool matchFilter(MaConn *conn, MaFilter *filter)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaLocation      *location;
    MaStage         *stage;

    req = conn->request;
    resp = conn->response;
    location = req->location;
    stage = filter->stage;

    if (stage->match) {
        return stage->match(conn, stage, req->url);
    }

    if (filter->extensions && *resp->extension) {
        return maMatchFilterByExtension(filter, resp->extension);
    }
    return 1;
}


static void openQ(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;

    conn = q->conn;
    resp = conn->response;

    if (resp->chunkSize > 0) {
        q->packetSize = min(q->packetSize, resp->chunkSize);
    }
    q->flags |= MA_QUEUE_OPEN;
    if (q->open) {
        q->open(q);
    }
}


/*
 *  Manage requests to directories. This will either do an external redirect back to the browser or do an internal 
 *  (transparent) redirection and serve different content back to the browser. This routine may modify the requested 
 *  URI and/or the request handler.
 */
static void processDirectory(MaConn *conn, bool *rescan)
{
    MaRequest       *req;
    MaResponse      *resp;
    MprFileInfo     *info;
    char            path[MPR_MAX_FNAME], urlBuf[MPR_MAX_FNAME], *index;
    int             len;

    req = conn->request;
    resp = conn->response;
    info = &resp->fileInfo;

    mprAssert(info->isDir);
    index = req->dir->indexName;
    if (req->url[strlen(req->url) - 1] == '/') {
        /*
         *  Internal directory redirections
         */
        len = (int) strlen(resp->filename);
        if (resp->filename[len - 1] == '/') {
            resp->filename[len - 1] = '\0';
        }
        path[0] = '\0';
        mprAssert(resp->filename && *resp->filename);
        mprAssert(index && *index);
        mprStrcat(path, sizeof(path), NULL, resp->filename, "/", index, NULL);
        if (mprAccess(resp, path, R_OK)) {
            /*
             *  Index file exists, so do an internal redirect to it. Client will not be aware of this happening.
             *  Must rematch the handler on return.
             */
            maSetRequestUri(conn, addIndex(conn, urlBuf, sizeof(urlBuf), index));

            resp->filename = mprStrdup(resp, path);
            mprGetFileInfo(conn, resp->filename, &resp->fileInfo);

            resp->extension = getExtension(conn);
            if ((resp->mimeType = (char*) maLookupMimeType(conn->host, resp->extension)) == 0) {
                resp->mimeType = (char*) "text/html";
            }
            *rescan = 1;
        }
        return;
    }

    /*
     *  External redirect. Ask the client to re-issue a request for a new location. See if an index exists and if so, 
     *  construct a new location for the index. If the index can't be accessed, just append a "/" to the URI and redirect.
     */
    if (req->parsedUri->query && req->parsedUri->query[0]) {
        mprSprintf(path, sizeof(path), "%s/%s?%s", req->url, index, req->parsedUri->query);
    } else {
        mprSprintf(path, sizeof(path), "%s/%s", req->url, index);
    }
    if (!mprAccess(resp, path, R_OK)) {
        mprSprintf(path, sizeof(path), "%s/", req->url);
    }
    maRedirect(conn, MPR_HTTP_CODE_MOVED_PERMANENTLY, path);
    resp->handler = conn->http->passHandler;
}


static bool fileExists(MprCtx ctx, cchar *path) {
    if (mprAccess(ctx, path, R_OK)) {
        return 1;
    }
#if BLD_WIN_LIKE
    if (strchr(path, '.') == 0) {
        char    filename[MPR_MAX_FNAME];
        mprSprintf(filename, sizeof(filename), "%s.exe", path);
        if (mprAccess(ctx, filename, R_OK)) {
            return 1;
        }
        mprSprintf(filename, sizeof(filename), "%s.bat", path);
        if (mprAccess(ctx, filename, R_OK)) {
            return 1;
        }
    }
#endif
    return 0;
}


/*
 *  Set the pathInfo (PATH_INFO) and update the request uri. This may set the response filename if convenient.
 */
static void setPathInfo(MaConn *conn)
{
    MaStage     *handler;
    MaAlias     *alias;
    MaRequest   *req;
    MaResponse  *resp;
    char        *last, *start, *cp, *pathInfo;
    int         found;

    req = conn->request;
    resp = conn->response;
    alias = req->alias;
    handler = resp->handler;

    mprAssert(handler);

    if (/* (req->location->flags & MA_LOC_PATH_INFO) || */ (handler && handler->flags & MA_STAGE_PATH_INFO)) {
        if (!(handler->flags & MA_STAGE_VIRTUAL)) {
            /*
             *  Find the longest subset of the filename that matches a real file. Test each segment to see if 
             *  it corresponds to a real physical file. This also defines a new response filename without the 
             *  extra path info.
             */
            last = 0;
            resp->filename = makeFilename(conn, alias, req->url, 1);
            for (cp = start = &resp->filename[strlen(alias->filename)]; cp; ) {
                
                if ((cp = strchr(cp, '/')) != 0) {
                    *cp = '\0';
                }
                found = fileExists(conn, resp->filename);
                if (cp) {
                    *cp = '/';
                }
                if (found) {
                    if (cp) {
                        last = cp++;
                    } else {
                        last = &resp->filename[strlen(resp->filename)];
                        break;
                    }
                } else {
                    break;
                }
            }
            if (last) {
                pathInfo = &req->url[alias->prefixLen + last - start];
                req->pathInfo = mprStrdup(req, pathInfo);
                *last = '\0';
                pathInfo[0] = '\0';
                if (req->pathInfo[0]) {
                    req->pathTranslated = makeFilename(conn, alias, req->pathInfo, 0);
                }
            }
        }
        if (req->pathInfo == 0) {
            req->pathInfo = req->url;
            req->url = "";

            if ((cp = strchr(req->pathInfo, '.')) != 0) {
                resp->extension = mprStrdup(req, ++cp);
            } else {
                resp->extension = "";
            }
            req->pathTranslated = makeFilename(conn, alias, req->pathInfo, 0); 
            resp->filename = alias->filename;
        }
    }
}


static void setEnv(MaConn *conn)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaStage         *handler;
    MprFileInfo     *info;

    req = conn->request;
    resp = conn->response;
    handler = resp->handler;

    setPathInfo(conn);

    if (resp->extension == 0) {
        resp->extension = getExtension(conn);
    }
    if (resp->filename == 0) {
        resp->filename = makeFilename(conn, req->alias, req->url, 1);
    }

    if ((resp->mimeType = (char*) maLookupMimeType(conn->host, resp->extension)) == 0) {
        resp->mimeType = (char*) "text/html";
    }

    if (!(resp->handler->flags & MA_STAGE_VIRTUAL)) {
        /*
         *  Define an Etag for physical entities. Redo the file info if not valid now that extra path has been removed.
         */
        info = &resp->fileInfo;
        if (!info->valid) {
            mprGetFileInfo(conn, resp->filename, info);
        }
        if (info->valid) {
            mprAllocSprintf(resp, &resp->etag, -1, "%x-%Lx-%Lx", info->inode, info->size, info->mtime);
        }
    }

    if (handler->flags & MA_STAGE_FORM_VARS) {
        req->formVars = mprCreateHash(req, MA_VAR_HASH_SIZE);
        if (req->parsedUri->query) {
            maAddFormVars(conn, req->parsedUri->query, (int) strlen(req->parsedUri->query));
        }
    }
    if (handler->flags & MA_STAGE_ENV_VARS) {
        maCreateEnvVars(conn);
    }
}


//  TODO - should be MapUrl
char *maMapUriToStorage(MaConn *conn, cchar *url)
{
    MaAlias     *alias;

    alias = maGetAlias(conn->request->host, url);
    if (alias == 0) {
        return 0;
    }
    return makeFilename(conn, alias, url, 1);
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

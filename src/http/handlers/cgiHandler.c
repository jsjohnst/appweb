/* 
 *  cgiHandler.c -- Common Gateway Interface Handler
 *
 *  Support the CGI/1.1 standard for external gateway programs to respond to HTTP requests.
 *  This CGI handler uses async-pipes and non-blocking I/O for all communications.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "http.h"

#if BLD_FEATURE_CGI
/*********************************** Forwards *********************************/

static void buildArgs(MaConn *conn, MprCmd *cmd, int *argcp, char ***argvp);
static void cgiCallback(MprCmd *cmd, int fd, int channel, void *data);
static char *getCgiToken(MprBuf *buf, cchar *delim);
static bool parseFirstCgiResponse(MaConn *conn, MprCmd *cmd);
static bool parseHeader(MaConn *conn, MprCmd *cmd);
static void pushDataToCgi(MaQueue *q);
static void startCmd(MaQueue *q);

#if BLD_DEBUG
static void traceCGIData(MprCmd *cmd, char *src, int size);
#define traceData(cmd, src, size) traceCGIData(cmd, src, size)
#else
#define traceData(cmd, src, size)
#endif

#if BLD_WIN_LIKE
static void findExecutable(MaConn *conn, char **program, char **script, char **bangScript, char *fileName);
static void checkCompletion(MprCmd *cmd, MprEvent *event);
#endif

/************************************* Code ***********************************/
/*
 *  Open this handler instance for a new request
 */
static void openCgi(MaQueue *q)
{
    MaRequest       *req;
    MaConn          *conn;

    conn = q->conn;
    req = conn->request;

    startCmd(q);
    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maDontCacheResponse(conn);

    /*
     *  Create an empty header packet and add to the service queue
     */
    maPutForService(q, maCreateHeaderPacket(conn), 0);
}


/*
 *  Prepare and start the CGI command. Called from openCgi()
 */
static void startCmd(MaQueue *q)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaConn          *conn;
    MprCmd          *cmd;
    MprHashTable    *vars;
    MprHash         *hp;
    cchar           *baseName;
    char            **argv, **envv, *fileName, dir[MPR_MAX_FNAME];
    int             index, argc, varCount;

    argv = 0;
    vars = 0;
    argc = 0;

    conn = q->conn;
    req = conn->request;
    resp = conn->response;

    cmd = q->queueData = mprCreateCmd(req);

    /*
     *  Build the commmand line arguments
     */
    argc = 1;                                   /* argv[0] == programName */
    buildArgs(conn, cmd, &argc, &argv);
    fileName = argv[0];

    mprGetDirName(dir, sizeof(dir), fileName);
    baseName = mprGetBaseName(fileName);
    if (strncmp(baseName, "nph-", 4) == 0 || 
            (strlen(baseName) > 4 && strcmp(&baseName[strlen(baseName) - 4], "-nph") == 0)) {
        /*
         *  Pretend we've seen the header for Non-parsed Header CGI programs
         */
        cmd->userFlags |= MA_CGI_SEEN_HEADER;
    }

    //  TODO - refactor and move this into prepareArgs()
    /*
     *  Build environment variables. For unix, also export the PATH and LD_LIBRARY_PATH so add 2.
     */
    vars = req->headers;
    varCount = mprGetHashCount(vars);
#if BLD_HOST_UNIX
    varCount += 2;
#endif

    index = 0;
    envv = (char**) mprAlloc(cmd, (varCount + 1) * sizeof(char*));
    hp = mprGetFirstHash(req->headers);
    while (hp) {
        if (hp->data) {
            mprAllocSprintf(cmd, &envv[index], MPR_MAX_FNAME, "%s=%s", hp->key, (char*) hp->data);
            index++;
        }
        hp = mprGetNextHash(req->headers, hp);
    }

#if BLD_HOST_UNIX
    {
        char    *cp;
        if ((cp = getenv("PATH")) != 0) {
            mprAllocSprintf(cmd, &envv[index++], MPR_MAX_STRING, "PATH=%s", cp);
        }
        if ((cp = getenv("LD_LIBRARY_PATH")) != 0) {
            mprAllocSprintf(cmd, &envv[index++], MPR_MAX_STRING, "LD_LIBRARY_PATH=%s", cp);
        }
    }
#endif

    envv[index] = 0;
    mprAssert(index <= varCount);

#if BLD_DEBUG
{
    int     i;
    mprLog(q, 4, "CGI: running program: %s: ", fileName);
    for (i = 1; argv[i]; i++) {
        mprRawLog(q, 4, "%s ", argv[i]);
    }
    mprRawLog(q, 4, "\n");
    for (i = 0; i < index; i++) {
        mprLog(q, 4, "CGI ENV %s", envv[i]);
    }
}
#endif

    cmd->stdoutBuf = mprCreateBuf(cmd, MPR_BUFSIZE, -1);
    cmd->stderrBuf = mprCreateBuf(cmd, MPR_BUFSIZE, -1);

    mprSetCmdDir(cmd, dir);
    mprSetCmdCallback(cmd, cgiCallback, conn);

    if (mprStartCmd(cmd, argc, argv, envv, MPR_CMD_IN | MPR_CMD_OUT | MPR_CMD_ERR) < 0) {
        maFailRequest(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Can't run CGI process: %s, URI %s", fileName, req->url);
    }
}


/*
 *  Accept a new packet of data destined for the browser. Initially, these packets are empty and need filling will data
 *  from the CGI program, so just queue for service (don't enable the service routine yet).
 */
static void outgoingCgiData(MaQueue *q, MaPacket *packet)
{
    maPutForService(q, packet, 0);
}


/*
 *  Service outgoing data destined for the browser.
 */ 
static void outgoingCgiService(MaQueue *q)
{
    MprCmd      *cmd;

    cmd = (MprCmd*) q->queueData;

    /*
     *  This will copy outgoing packets downstream toward the network connector and on to the browser. This may disable this 
     *  queue if the downstream net connector queue overflows because the socket is full. In that case, conn.c:setupConnIO() 
     *  will setup to listen for writable events. When the socket is writable again, the connector will drain its queue
     *  which will re-enable this queue and schedule it for service again.
     */ 
    maDefaultOutgoingServiceStage(q);

    //  TODO OPT - need a flag so we don't do this first time.
    if (q->count < q->low) {
        mprEnableCmdEvents(cmd, MPR_CMD_STDOUT);
    }
}


/*
 *  Accept incoming data from the browser destined for the CGI gateway (POST | PUT dat).
 */
static void incomingCgiData(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaRequest   *req;
    MprCmd      *cmd;

    conn = q->conn;
    resp = conn->response;
    req = conn->request;

    cmd = (MprCmd*) q->pair->queueData;
    mprAssert(cmd);

    if (packet->count == 0) {
        /*
         *  End of input
         */
        if (req->remainingContent > 0) {
            /*
             *  Short incoming body data. Just kill the CGI process.
             */
            mprFree(cmd);
            q->queueData = 0;
            maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Client supplied iinsufficient body data");
        }
        /*
         *  End of input. request.c should handle transitioning to call the run() routine for us automatically.
         */

    } else {
        /*
         *  No service routine, we just need it to be queued for pushDataToCgi
         */
        maPutForService(q, packet, 0);
    }
    pushDataToCgi(q);
}


/*
 *  Write data to the CGI program. (may block). This is called from incomingCgiData and from the cgiCallback when the pipe
 *  to the CGI program becomes writabl.
 */
static void pushDataToCgi(MaQueue *q)
{
    MaConn      *conn;
    MaPacket    *packet;
    MprCmd      *cmd;
    MprBuf      *buf;
    int         len, rc;

    cmd = (MprCmd*) q->pair->queueData;
    mprAssert(cmd);
    conn = q->conn;

    for (packet = maGet(q); packet && !conn->requestFailed; packet = maGet(q)) {
        buf = packet->content;
        len = mprGetBufLength(buf);
        mprAssert(len > 0);
        rc = mprWriteCmdPipe(cmd, MPR_CMD_STDIN, mprGetBufStart(buf), len);
        if (rc < 0) {
            mprLog(q, 2, "CGI: write to gateway failed for %d bytes, rc %d, errno %d\n", len, rc, errno);
            mprCloseCmdFd(cmd, MPR_CMD_STDIN);
            maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Can't write body data to CGI gateway");
            break;

        } else {
            mprLog(q, 5, "CGI: write to gateway %d bytes asked to write %d\n", rc, len);
            mprAdjustBufStart(buf, rc);
            if (mprGetBufLength(buf) > 0) {
                maPutBack(q, packet);
            }
            if (rc < len) {
                /*
                 *  CGI gateway didn't accept all the data. Enable CGI write events to be notified when the gateway
                 *  can read more data.
                 */
                mprEnableCmdEvents(cmd, MPR_CMD_STDIN);
            }
        }
    }
}


/*
 *  Run after all incoming data has been received. Can now close the client's stdin.
 */
static void runCgi(MaQueue *q)
{
    MprCmd      *cmd;
    MaConn      *conn;

    conn = q->conn;
    cmd = (MprCmd*) q->queueData;

    /*
     *  Close the CGI program's stdin. This will allow it to exit if it was expecting input data.
     */
    if (q->queueData) {
        mprCloseCmdFd(cmd, MPR_CMD_STDIN);
    }
    if (conn->requestFailed) {
        maPutForService(q, maCreateEndPacket(conn), 1);
    }

#if BLD_WIN_LIKE
    /*
     *  Windows polls for completion
     */
    mprCreateTimerEvent(cmd, checkCompletion, MA_CGI_PERIOD, MPR_NORMAL_PRIORITY, cmd, 0);
#endif
}


#if BLD_WIN_LIKE
static void checkCompletion(MprCmd *cmd, MprEvent *event)
{
    mprPollCmd(cmd);
}
#endif


static int writeToBrowser(MaQueue *q, MprCmd *cmd, MprBuf *buf, int channel)
{
    MaConn  *conn;
    int     servicedQueues, rc, len;

    conn = q->conn;

    /*
     *  Write to the browser. We write as much as we can. Service queues to get the filters and connectors pumping.
     */
    for (servicedQueues = 0; (len = mprGetBufLength(buf)) > 0 ; ) {

        if (!conn->requestFailed) {
            rc = maWriteBlock(q, mprGetBufStart(buf), len, 0);
            mprLog(cmd, 5, "Write to browser ask %d, actual %d", len, rc);
        } else {
            rc = len;
        }
        if (rc > 0) {
            mprAdjustBufStart(buf, rc);
            mprResetBufIfEmpty(buf);

        } else if (rc == 0) {
            if (servicedQueues) {
                /*
                 *  Can't write anymore data. Block the CGI gateway. outgoingCgiService will enable.
                 */
                mprAssert(q->count >= q->max);
                mprAssert(q->flags & MA_QUEUE_DISABLED);
                mprDisableCmdEvents(cmd, channel);
                return MPR_ERR_CANT_WRITE;
                
            } else {
                maServiceQueues(conn);
                servicedQueues++;
            }
        }
    }
    return 0;
}


/*
 *  Read the output data from the CGI script and return it to the client
 */
static void cgiCallback(MprCmd *cmd, int fd, int channel, void *data)
{
    MaConn      *conn;
    MaResponse  *resp;
    MprBuf      *buf;
    MaQueue     *q;
    int         space, rc, bytesRead, closed;

    conn = data;
    resp = conn->response;
    q = resp->queue[MA_QUEUE_SEND].nextQ;
    buf = 0;
    closed = 0;

    switch (channel) {
    case MPR_CMD_STDIN:
        /*
         *  CGI's stdin is now accepting more data
         */
        mprDisableCmdEvents(cmd, MPR_CMD_STDIN);
        pushDataToCgi(q->pair);
        return;

    case MPR_CMD_STDOUT:
        buf = cmd->stdoutBuf;
        break;

    case MPR_CMD_STDERR:
        buf = cmd->stderrBuf;
        break;
    }

    mprAssert(buf);
    mprResetBufIfEmpty(buf);

    /*
     *  Come here for CGI stdout, stderr events. ie. we can read data from the CGI program.
     */
    while (!cmd->completed) {

        /*
         *  Read data from the CGI pipe and try to totally fill the buffer
         */
        while ((space = mprGetBufSpace(buf)) > 0) {
            bytesRead = mprReadCmdPipe(cmd, channel, mprGetBufEnd(buf), space);
            if (bytesRead <= 0) {
                if (bytesRead == 0 || !(errno == EAGAIN || EWOULDBLOCK || errno == EINTR)) {
                    /*
                     *  This will set cmd->completed when we've closed both stderr and stdout
                     */
                    rc = errno;
                    mprCloseCmdFd(cmd, channel);
                    mprLog(cmd, 5, "CGI Read from %s got %d, ask %d, errno %d",
                        (channel == MPR_CMD_STDOUT) ? "stdout" : "stderr", bytesRead, space, rc);
                    closed++;
                }
                break;

            } else {
                mprLog(cmd, 5, "CGI Read from %s got %d, ask %d",
                       (channel == MPR_CMD_STDOUT) ? "stdout" : "stderr", bytesRead, space);
                mprAdjustBufEnd(buf, bytesRead);
                traceData(cmd, mprGetBufStart(buf), bytesRead);
            }
        }
        if (mprGetBufLength(buf) == 0) {
            break;
        }
        if (channel == MPR_CMD_STDERR) {
            mprAddNullToBuf(buf);
            mprError(conn, mprGetBufStart(buf));
            mprFlushBuf(buf);
            
        } else {
            if (!(cmd->userFlags & MA_CGI_SEEN_HEADER)) {
                if (!parseHeader(conn, cmd)) {
                    return;
                }
            }
            if (writeToBrowser(q, cmd, buf, channel) < 0) {
                return;
            }
        }
        if (mprGetBufLength(buf) == 0 && closed) {
            break;
        }
    }

    if (cmd->completed) {
        if ((rc = mprReapCmd(cmd, MA_CGI_TIMEOUT) != 0) || cmd->status != 0) {
            maFailRequest(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE,
                "CGI process %s: exited abnormally with return code %d, exit status: %d", resp->filename, rc, cmd->status);
        } else if (!(cmd->userFlags & MA_CGI_SEEN_HEADER)) {
            maFailRequest(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Header not seen");
        }
        /*
         *  Write an EOF packet downstream to the client and enable for service. This will Schedule outgoingCgiService.
         */            
        maPutForService(q, maCreateEndPacket(q->conn), 1);
        maServiceQueues(conn);
    }
    if (conn->state == MPR_HTTP_STATE_COMPLETE) {
        /*
         *  TODO - multithread race 
         *  Issue a dummy read event to cycle through the last stage of the request pipeline. This will complete
         *  the request and cleanup. WARNING - the request and cmd will be deleted after this.
         */
        maProcessReadEvent(conn, 0);
        maAwakenConn(conn);

    } else if (conn->requestFailed) {
        maServiceQueues(conn);
    }
}


/*
 *  Parse the CGI output first line
 */
static bool parseFirstCgiResponse(MaConn *conn, MprCmd *cmd)
{
    MaResponse      *resp;
    MprBuf          *buf;
    char            *protocol, *code, *message;
    
    resp = conn->response;
    buf = mprGetCmdBuf(cmd, MPR_CMD_STDOUT);
    
    protocol = getCgiToken(buf, " ");
    if (protocol == 0 || protocol[0] == '\0') {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Bad CGI HTTP protocol response");
        return 0;
    }
    
    if (strncmp(protocol, "HTTP/1.", 7) != 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Unsupported CGI protocol");
        return 0;
    }
    
    code = getCgiToken(buf, " ");
    if (code == 0 || *code == '\0') {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_GATEWAY, "Bad CGI header response");
        return 0;
    }
    message = getCgiToken(buf, "\n");
    
    mprLog(conn, 4, "CGI status line: %s %s %s", protocol, code, message);
    return 1;
}


/*
 *  Parse the CGI output headers. 
 *  Sample CGI program:
 *
 *  Content-type: text/html
 * 
 *  <html.....
 */
static bool parseHeader(MaConn *conn, MprCmd *cmd)
{
    MaResponse      *resp;
    MprBuf          *buf;
    char            *endHeaders, *headers, *key, *value, *location;
    int             len;

    resp = conn->response;
    location = 0;
    value = 0;

    buf = mprGetCmdBuf(cmd, MPR_CMD_STDOUT);
    headers = mprGetBufStart(buf);

    /*
     *  Split the headers from the body.
     */
    len = 0;
    if ((endHeaders = strstr(headers, "\r\n\r\n")) == NULL) {
        if ((endHeaders = strstr(headers, "\n\n")) == NULL) {
            return 0;
        } 
        len = 2;
    } else {
        len = 4;
    }

    endHeaders[len - 1] = '\0';
    endHeaders += len;

    /*
     *  Want to be tolerant of CGI programs that omit the status line.
     */
    if (strncmp((char*) buf->start, "HTTP/1.", 7) == 0) {
        if (!parseFirstCgiResponse(conn, cmd)) {
            return 0;
        }
    }
    
    if (strchr(mprGetBufStart(buf), ':')) {
        mprLog(conn, 4, "CGI: parseHeader: header\n%s\n", headers);

        while (mprGetBufLength(buf) > 0 && buf->start[0] && (buf->start[0] != '\r' && buf->start[0] != '\n')) {

            if ((key = getCgiToken(buf, ":")) == 0) {
                maFailConnection(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad header format");
                return 0;
            }
            value = getCgiToken(buf, "\n");
            while (isspace((int) *value)) {
                value++;
            }
            len = (int) strlen(value);
            while (len > 0 && (value[len - 1] == '\r' || value[len - 1] == '\n')) {
                value[len - 1] = '\0';
                len--;
            }
            mprStrLower(key);

            if (strcmp(key, "location") == 0) {
                location = value;

            } else if (strcmp(key, "status") == 0) {
                maSetResponseCode(conn, atoi(value));

            } else if (strcmp(key, "content-type") == 0) {
                maSetResponseMimeType(conn, value);

            } else {
                /*
                 *  Now pass all other headers back to the client
                 */
                maSetHeader(conn, 0, key, "%s", value);
            }
        }
        buf->start = (uchar*) endHeaders;
    }
    if (location) {
        maRedirect(conn, resp->code, value);
    }
    cmd->userFlags |= MA_CGI_SEEN_HEADER;
    return 1;
}


/*
 *  Build the command arguments. NOTE: argv is untrusted input.
 */
static void buildArgs(MaConn *conn, MprCmd *cmd, int *argcp, char ***argvp)
{
    MaRequest   *req;
    MaResponse  *resp;
    char        *fileName, **argv, *program, *cmdScript, status[8], *indexQuery, escape[MPR_MAX_STRING], *cp, *tok;
    cchar       *actionProgram;
    int         argc, argind, len;

    req = conn->request;
    resp = conn->response;

    fileName = resp->filename;
    mprAssert(fileName);

    program = cmdScript = 0;
    actionProgram = 0;
    argind = 0;
    argc = *argcp;

    if (req->mimeType) {
        actionProgram = maGetMimeActionProgram(req->host, req->mimeType);
        if (actionProgram != 0) {
            argc++;
            /*
             *  This is an Apache compatible hack
             */
            mprItoa(status, sizeof(status), MPR_HTTP_CODE_MOVED_TEMPORARILY, 10);
            mprAddHash(req->headers, "REDIRECT_STATUS", status);
        }
    }

    /*
     *  Count the args for ISINDEX queries. Only valid if there is not a "=" in the query. 
     *  If this is so, then we must not have these args in the query env also?
     */
    indexQuery = req->parsedUri->query;
    if (indexQuery && !strchr(indexQuery, '=')) {
        argc++;
        for (cp = indexQuery; *cp; cp++) {
            if (*cp == '+') {
                argc++;
            }
        }
    } else {
        indexQuery = 0;
    }

#if BLD_WIN_LIKE
{
    char    *bangScript, *cmdBuf;

    /*
     *  On windows we attempt to find an executable matching the fileName.
     *  We look for *.exe, *.bat and also do unix style processing "#!/program"
     */
    findExecutable(conn, &program, &cmdScript, &bangScript, fileName);
    mprAssert(program);

    if (cmdScript) {
        /*
         *  Cmd/Batch script (.bat | .cmd)
         *  Convert the command to the form where there are 4 elements in argv
         *  that cmd.exe can interpret.
         *
         *      argv[0] = cmd.exe
         *      argv[1] = /Q
         *      argv[2] = /C
         *      argv[3] = ""script" args ..."
         */
        argc = 4;

        len = (argc + 1) * sizeof(char*);
        argv = (char**) mprAlloc(cmd, len);
        memset(argv, 0, len);

        argv[argind++] = program;               /* Duped in findExecutable */
        argv[argind++] = mprStrdup(cmd, "/Q");
        argv[argind++] = mprStrdup(cmd, "/C");

        len = (int) strlen(cmdScript) + 2 + 1;
        cmdBuf = (char*) mprAlloc(cmd, len);
        mprSprintf(cmdBuf, len, "\"%s\"", cmdScript);
        argv[argind++] = cmdBuf;

        mprSetCmdDir(cmd, cmdScript);
        mprFree(cmdScript);
        /*  program will get freed when argv[] gets freed */
        
    } else if (bangScript) {
        /*
         *  Script used "#!/program". NOTE: this may be overridden by a mime
         *  Action directive.
         */
        argc++;     /* Adding bangScript arg */

        len = (argc + 1) * sizeof(char*);
        argv = (char**) mprAlloc(cmd, len);
        memset(argv, 0, len);

        argv[argind++] = program;       /* Will get freed when argv[] is freed */
        argv[argind++] = bangScript;    /* Will get freed when argv[] is freed */
        mprSetCmdDir(cmd, bangScript);

    } else {
        /*
         *  Either unknown extension or .exe program.
         */
        len = (argc + 1) * sizeof(char*);
        argv = (char**) mprAlloc(cmd, len);
        memset(argv, 0, len);

        if (actionProgram) {
            argv[argind++] = mprStrdup(cmd, actionProgram);
        }
        argv[argind++] = program;
    }
}
#else
    len = (argc + 1) * sizeof(char*);
    argv = (char**) mprAlloc(cmd, len);
    memset(argv, 0, len);

    if (actionProgram) {
        argv[argind++] = mprStrdup(cmd, actionProgram);
    }
    argv[argind++] = mprStrdup(cmd, fileName);

#endif

    /*
     *  ISINDEX queries. Only valid if there is not a "=" in the query. If this is so, then we must not
     *  have these args in the query env also?
     *  TODO - should query vars be set in the env?
     */
    if (indexQuery) {
        indexQuery = mprStrdup(cmd, indexQuery);

        cp = mprStrTok(indexQuery, "+", &tok);
        while (cp) {
            mprUrlDecode(cp, (int) strlen(cp), cp);
            mprEscapeCmd(escape, sizeof(escape), cp, 0);
            argv[argind++] = mprStrdup(cmd, escape);
            cp = mprStrTok(NULL, "+", &tok);
        }
    }
    
    mprAssert(argind == argc);
    argv[argind] = 0;
    *argcp = argc;
    *argvp = argv;
}


#if BLD_WIN_LIKE
/*
 *  If the program has a UNIX style "#!/program" string at the start of the file that program will be selected 
 *  and the original program will be passed as the first arg to that program with argv[] appended after that. If 
 *  the program is not found, this routine supports a safe intelligent search for the command. If all else fails, 
 *  we just return in program the fileName we were passed in. script will be set if we are modifying the program 
 *  to run and we have extracted the name of the file to run as a script.
 */
static void findExecutable(MaConn *conn, char **program, char **script, char **bangScript, char *fileName)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaLocation      *location;
    MprHash         *hp;
    MprFile         *file;
    cchar           *actionProgram, *ext, *cmdShell;
    char            buf[MPR_MAX_FNAME + 1], pathBuf[MPR_MAX_FNAME];
    char            dirBuf[MPR_MAX_FNAME + 1], tmp[MPR_MAX_FNAME];
    char            *tok;

    req = conn->request;
    resp = conn->response;
    location = req->location;

    *bangScript = 0;
    *script = 0;
    *program = 0;

    actionProgram = maGetMimeActionProgram(conn->host, req->mimeType);
    ext = resp->extension;

    /*
     *  If not found, go looking for the fileName with the extensions defined in appweb.conf. 
     *  NOTE: we don't use PATH deliberately!!!
     */
    if (access(fileName, X_OK) < 0 && *ext == '\0') {
        for (hp = 0; (hp = mprGetNextHash(location->extensions, hp)) != 0; ) {
            mprSprintf(pathBuf, sizeof(pathBuf), "%s.%s", fileName, hp->key);
            if (access(pathBuf, X_OK) == 0) {
                break;
            }
        }
        if (hp) {
            ext = hp->key;
        } else {
            mprStrcpy(pathBuf, sizeof(pathBuf), fileName);
        }

    } else {
        mprStrcpy(pathBuf, sizeof(pathBuf), fileName);
    }

    if (ext && (strcmp(ext, ".bat") == 0 || strcmp(ext, ".cmd") == 0)) {
        /*
         *  Let a mime action override COMSPEC
         */
        if (actionProgram) {
            cmdShell = actionProgram;
        } else {
            cmdShell = getenv("COMSPEC");
        }
        if (cmdShell == 0) {
            cmdShell = "cmd.exe";
        }
        *script = mprStrdup(resp, pathBuf);
        *program = mprStrdup(resp, cmdShell);
        return;
    }

    if ((file = mprOpen(resp, pathBuf, O_RDONLY, 0)) != 0) {
        if (mprRead(file, buf, MPR_MAX_FNAME) > 0) {
            mprFree(file);
            buf[MPR_MAX_FNAME] = '\0';
            if (buf[0] == '#' && buf[1] == '!') {
                cmdShell = mprStrTok(&buf[2], " \t\r\n", &tok);
                if (cmdShell[0] != '/' && (cmdShell[0] != '\0' && cmdShell[1] != ':')) {
                    /*
                     *  If we can't access the command shell and the command 
                     *  is not an absolute path, look in the same directory 
                     *  as the script.
                     */
                    if (mprAccess(resp, cmdShell, X_OK)) {
                        mprGetDirName(dirBuf, sizeof(dirBuf), pathBuf);
                        mprSprintf(tmp, sizeof(tmp), "%s/%s", dirBuf, cmdShell);
                        cmdShell = tmp;
                    }
                }
                if (actionProgram) {
                    *program = mprStrdup(resp, actionProgram);
                } else {
                    *program = mprStrdup(resp, cmdShell);
                }
                *bangScript = mprStrdup(resp, pathBuf);
                return;
            }
        } else {
            mprFree(file);
        }
    }

    if (actionProgram) {
        *program = mprStrdup(resp, actionProgram);
        *bangScript = mprStrdup(resp, pathBuf);
    } else {
        *program = mprStrdup(resp, pathBuf);
    }
    return;
}
#endif
 

/*
 *  Get the next input token. The content buffer is advanced to the next token. This routine always returns a 
 *  non-zero token. The empty string means the delimiter was not found.
 */
static char *getCgiToken(MprBuf *buf, cchar *delim)
{
    char    *token, *nextToken;
    int     len;

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


#if BLD_DEBUG
/*
 *  Trace output received from the cgi process
 */
static void traceCGIData(MprCmd *cmd, char *src, int size)
{
    char    dest[512];
    int     index, i;

    mprRawLog(cmd, 5, "@@@ CGI process wrote => \n");

    for (index = 0; index < size; ) { 
        for (i = 0; i < (sizeof(dest) - 1) && index < size; i++) {
            dest[i] = src[index];
            index++;
        }
        dest[i] = '\0';
        mprRawLog(cmd, 5, "%s", dest);
    }
    mprRawLog(cmd, 5, "\n");
}
#endif


#if BLD_FEATURE_CONFIG_PARSE
static int parseCgi(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    MaLocation  *location;
    MaServer    *server;
    MaHost      *host;
    MaAlias     *alias;
    MaDir       *dir, *parent;
    char        pathBuf[MPR_MAX_FNAME];
    char        *program, *mimeType, *prefix, *path;

    host = state->host;
    server = state->server;
    location = state->location;

    if (mprStrcmpAnyCase(key, "Action") == 0) {
        if (maSplitConfigValue(http, &mimeType, &program, value, 1) < 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
        maSetMimeActionProgram(host, mimeType, program);
        return 1;

    } else if (mprStrcmpAnyCase(key, "ScriptAlias") == 0) {
        if (maSplitConfigValue(server, &prefix, &path, value, 1) < 0 || path == 0 || prefix == 0) {
            return MPR_ERR_BAD_SYNTAX;
        }

        /*
         *  Create an alias and location with a cgiHandler and pathInfo processing
         */
        maMakePath(host, pathBuf, sizeof(pathBuf), path);

        dir = maLookupDir(host, pathBuf);
        if (maLookupDir(host, pathBuf) == 0) {
            parent = mprGetFirstItem(host->dirs);
            dir = maCreateDir(host, pathBuf, parent);
#if UNUSED
            mprError(http, "Missing directory block at %s for alias %s", pathBuf, prefix);
            return MPR_ERR_NOT_FOUND;
#endif
        }
        alias = maCreateAlias(host, prefix, pathBuf, 0);
        mprLog(server, 4, "ScriptAlias \"%s\" for \"%s\"", prefix, pathBuf);
        maInsertAlias(host, alias);

        if ((location = maLookupLocation(host, prefix)) == 0) {
            location = maCreateLocation(host, state->location);
            maSetLocationAuth(location, state->dir->auth);
            maSetLocationPrefix(location, prefix);
            maAddLocation(host, location);
        } else {
            maSetLocationPrefix(location, prefix);
        }
#if UNUSED
        maSetLocationFlags(location, MA_LOC_PATH_INFO);
#endif
        maSetHandler(http, host, location, "cgiHandler");
        return 1;
    }

    return 0;
}
#endif


/*
 *  Dynamic module initialization
 */
MprModule *maCgiHandlerInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *handler;

    module = mprCreateModule(http, "cgiHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "cgiHandler", 
        MA_STAGE_ALL | MA_STAGE_FORM_VARS | MA_STAGE_ENV_VARS | MA_STAGE_PATH_INFO);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->open = openCgi; 
    handler->outgoingData = outgoingCgiData;
    handler->outgoingService = outgoingCgiService;
    handler->incomingData = incomingCgiData; 
    handler->run = runCgi; 
    handler->parse = parseCgi; 

    return module;
}


#else
void mprCgiHandlerDummy() {}

#endif /* BLD_FEATURE_CGI */

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

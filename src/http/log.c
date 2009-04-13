/*
 *  log.c -- Logging
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/************************************ Code *************************************/
/*
 *  Turn on logging. If no logSpec is specified, default to stdout:2. If the user specifies --log "none" then 
 *  the log is disabled. This is useful when specifying the log via the appweb.conf.
 */
//  TODO - would be nice to have logging per host

static void logHandler(MprCtx ctx, int flags, int level, const char *msg)
{
    Mpr         *mpr;
    MprFile     *file;
    char        *prefix;

    mpr = mprGetMpr(ctx);
    file = (MprFile*) mpr->logHandlerData;
    prefix = mpr->name;

    while (*msg == '\n') {
        mprFprintf(file, "\n");
        msg++;
    }

    if (flags & MPR_LOG_SRC) {
        mprFprintf(file, "%s: %d: %s\n", prefix, level, msg);

    } else if (flags & MPR_ERROR_SRC) {
        /*
         *  Use static printing to avoid malloc when the messages are small.
         *  This is important for memory allocation errors.
         */
        if (strlen(msg) < (MPR_MAX_STRING - 32)) {
            mprStaticErrorPrintf(file, "%s: Error: %s\n", prefix, msg);
        } else {
            mprFprintf(file, "%s: Error: %s\n", prefix, msg);
        }

    } else if (flags & MPR_FATAL_SRC) {
        mprFprintf(file, "%s: Fatal: %s\n", prefix, msg);
        
    } else if (flags & MPR_ASSERT_SRC) {
        mprFprintf(file, "%s: Assertion %s, failed\n", prefix, msg);

    } else if (flags & MPR_RAW) {
        mprFprintf(file, "%s", msg);
    }
}



/*
 *  Start error and information logging. Note: this is not per-request access logging
 */
int maStartLogging(MprCtx ctx, cchar *logSpec)
{
    Mpr         *mpr;
    MprFile     *file;
    MprTime     now;
    char        *levelSpec, *spec, timeText[80];
    int         level;

    level = 0;
    mpr = mprGetMpr(ctx);

    if (logSpec == 0) {
        logSpec = "stdout:2";
    }
    if (*logSpec && strcmp(logSpec, "none") != 0) {
        spec = mprStrdup(mpr, logSpec);
        if ((levelSpec = strrchr(spec, ':')) != 0 && isdigit((int) levelSpec[1])) {
            *levelSpec++ = '\0';
            level = atoi(levelSpec);
        }

        if (strcmp(spec, "stdout") == 0) {
            file = mpr->fileService->console;

        } else {
            if ((file = mprOpen(mpr, spec, O_CREAT | O_WRONLY | O_TRUNC | O_TEXT, 0664)) == 0) {
                mprErrorPrintf(mpr, "Can't open log file %s\n", spec);
                return -1;
            }
        }
        mprSetLogLevel(mpr, level);
        mprSetLogHandler(mpr, logHandler, (void*) file);

        now = mprGetTime(mpr);
        mprCtime(mpr, timeText, sizeof(timeText), now);

        mprLog(mpr, MPR_CONFIG, "Configuration for %s", mprGetAppTitle(mpr));
        mprLog(mpr, MPR_CONFIG, "--------------------------------------------");
        mprLog(mpr, MPR_CONFIG, "Host:               %s", mprGetHostName(mpr));
        mprLog(mpr, MPR_CONFIG, "CPU:                %s", BLD_HOST_CPU);
        mprLog(mpr, MPR_CONFIG, "OS:                 %s", BLD_HOST_OS);
        mprLog(mpr, MPR_CONFIG, "Distribution:       %s %s", BLD_HOST_DIST, BLD_HOST_DIST_VER);
        mprLog(mpr, MPR_CONFIG, "OS:                 %s", BLD_HOST_OS);
        mprLog(mpr, MPR_CONFIG, "Version:            %s.%d", BLD_VERSION, BLD_NUMBER);
        mprLog(mpr, MPR_CONFIG, "BuildType:          %s", BLD_TYPE);
        mprLog(mpr, MPR_CONFIG, "Started at:         %s", timeText);
        mprLog(mpr, MPR_CONFIG, "--------------------------------------------");
    }

    return 0;
}


/*
 *  Stop the error and information logging. Note: this is not per-request access logging
 */
int maStopLogging(MprCtx ctx)
{
    MprFile     *file;
    Mpr         *mpr;

    mpr = mprGetMpr(ctx);

    file = mpr->logHandlerData;
    if (file) {
        mprFree(file);
        mpr->logHandlerData = 0;
        mprSetLogHandler(mpr, 0, 0);
    }
    return 0;
}

#if BLD_FEATURE_ACCESS_LOG

int maStartAccessLogging(MaHost *host)
{
#if !BLD_FEATURE_ROMFS
    if (host->logPath) {
        host->accessLog = mprOpen(host, host->logPath, O_CREAT | O_APPEND | O_WRONLY | O_TEXT, 0664);
        if (host->accessLog ==0) {
            mprError(host, "Can't open log file %s", host->logPath);
        }
    }
#endif
    return 0;
}


int maStopAccessLogging(MaHost *host)
{
    if (host->accessLog) {
        mprFree(host->accessLog);
        host->accessLog = 0;
    }
    return 0;
}


void maSetAccessLog(MaHost *host, cchar *path, cchar *format)
{
    char    *src, *dest;

    mprAssert(host);
    mprAssert(path && *path);
    mprAssert(format);

    mprFree(host->logPath);
    host->logPath = mprStrdup(host, path);

    mprFree(host->logFormat);
    host->logFormat = mprStrdup(host, format);

    for (src = dest = host->logFormat; *src; src++) {
        if (*src == '\\' && src[1] != '\\') {
            continue;
        }
        *dest++ = *src;
    }
    *dest = '\0';
}


void maSetLogHost(MaHost *host, MaHost *logHost)
{
    host->logHost = logHost;
}


void maWriteAccessLogEntry(MaHost *host, cchar *buf, int len)
{
    static int once = 0;

    if (mprWrite(host->accessLog, (char*) buf, len) != len && once++ == 0) {
        mprError(host, "Can't write to access log %s", host->logPath);
    }
}


/*
 *  Called to rotate the access log
 */
void maRotateAccessLog(MaHost *host)
{
    MprFileInfo     info;
    struct tm       tm;
    MprTime         when;
    char            bak[MPR_MAX_FNAME];

    /*
     *  Rotate logs when full
     */
    if (mprGetFileInfo(host, host->logPath, &info) == 0 && info.size > MA_MAX_ACCESS_LOG) {

        when = mprGetTime(host);
        mprGmtime(host, &tm, when);

        mprSprintf(bak, sizeof(bak), "%s-%02d-%02d-%02d-%02d:%02d:%02d", host->logPath, 
            tm.tm_mon, tm.tm_mday, tm.tm_year, tm.tm_hour, tm.tm_min, tm.tm_sec);

        mprFree(host->accessLog);
        rename(host->logPath, bak);
        unlink(host->logPath);

        host->accessLog = mprOpen(host, host->logPath, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0664);
    }
}


void maLogRequest(MaConn *conn)
{
    MaHost      *logHost, *host;
    MaResponse  *resp;
    MaRequest   *req;
    MprBuf      *buf;
    MprTime     now;
    char        keyBuf[80], timeBuf[64], *fmt, *cp, *qualifier, *value, c;
    int         len;

    resp = conn->response;
    req = conn->request;
    host = req->host;

    logHost = host->logHost;
    if (logHost == 0) {
        return;
    }
    fmt = logHost->logFormat;
    if (fmt == 0) {
        return;
    }

    len = MA_MAX_URL + 256;
    buf = mprCreateBuf(resp, len, len);

    while ((c = *fmt++) != '\0') {
        if (c != '%' || (c = *fmt++) == '%') {
            mprPutCharToBuf(buf, c);
            continue;
        }

        switch (c) {
        case 'a':                           /* Remote IP */
            mprPutStringToBuf(buf, conn->remoteIpAddr);
            break;

        case 'A':                           /* Local IP */
            mprPutStringToBuf(buf, conn->sock->listenSock->ipAddr);
            break;

        case 'b':
            if (resp->bytesWritten == 0) {
                mprPutCharToBuf(buf, '-');
            } else {
                mprPutIntToBuf(buf, resp->bytesWritten);
            } 
            break;

        case 'B':                           /* Bytes written (minus headers) */
            mprPutIntToBuf(buf, resp->bytesWritten - resp->headerSize);
            break;

        case 'h':                           /* Remote host */
            //  TODO - Should this trigger a reverse DNS?
            mprPutStringToBuf(buf, conn->remoteIpAddr);
            break;

        case 'n':                           /* Local host */
            mprPutStringToBuf(buf, req->parsedUri->host);
            break;

        case 'l':                           /* Supplied in authorization */
            mprPutStringToBuf(buf, req->user ? req->user : "-");
            break;

        case 'O':                           /* Bytes written (including headers) */
            mprPutIntToBuf(buf, resp->bytesWritten);
            break;

        case 'r':                           /* First line of request */
            mprPutFmtToBuf(buf, "%s %s %s", req->methodName, req->parsedUri->originalUri, req->httpProtocol);
            break;

        case 's':                           /* Response code */
            mprPutIntToBuf(buf, resp->code);
            break;

        case 't':                           /* Time */
            now = mprGetTime(conn);
            mprCtime(conn, timeBuf, sizeof(timeBuf), now);
            if ((cp = strchr(timeBuf, '\n')) != 0) {
                *cp = '\0';
            }
            mprPutCharToBuf(buf, '[');
            mprPutStringToBuf(buf, timeBuf);
            mprPutCharToBuf(buf, ']');
            break;

        case 'u':                           /* Remote username */
            mprPutStringToBuf(buf, req->user ? req->user : "-");
            break;

        case '{':                           /* Header line */
            qualifier = fmt;
            if ((cp = strchr(qualifier, '}')) != 0) {
                fmt = &cp[1];
                *cp = '\0';
                c = *fmt++;
                mprStrcpy(keyBuf, sizeof(keyBuf), "HTTP_");
                mprStrcpy(&keyBuf[5], sizeof(keyBuf) - 5, qualifier);
                mprStrUpper(keyBuf);
                switch (c) {
                case 'i':
                    value = (char*) mprLookupHash(req->headers, keyBuf);
                    mprPutStringToBuf(buf, value ? value : "-");
                    break;
                default:
                    mprPutStringToBuf(buf, qualifier);
                }
                *cp = '}';

            } else {
                mprPutCharToBuf(buf, c);
            }
            break;

        case '>':
            if (*fmt == 's') {
                fmt++;
                mprPutIntToBuf(buf, resp->code);
            }
            break;

        default:
            mprPutCharToBuf(buf, c);
            break;
        }
    }
    mprPutCharToBuf(buf, '\n');
    mprAddNullToBuf(buf);

    mprWrite(logHost->accessLog, mprGetBufStart(buf), mprGetBufLength(buf));
}

#else
void maLogRequest(MaConn *conn) {}
#endif /* BLD_FEATURE_ACCESS_LOG */



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

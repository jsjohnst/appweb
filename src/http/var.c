/*
 *  var.c -- Create header and query variables.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 *
 *  TODO - rethink this while approach
 */

/********************************* Includes ***********************************/

#include    "http.h"

/*********************************** Code *************************************/
/*
 *  Define standard CGI environment variables
 */
void maCreateEnvVars(MaConn *conn)
{
    MaRequest       *req;
    MaResponse      *resp;
    MaHost          *host;
    MprSocket       *listenSock;
    MprHashTable    *vars;
    char            port[16];

    req = conn->request;
    resp = conn->response;
    host = conn->host;
    
    vars = req->headers;

    //  TODO - BUG COOKIE
    
    /*
     *  Alias for REMOTE_USER. Define both for broader compatibility with CGI.
     */
    mprAddHash(vars, "AUTH_TYPE", req->authType);
    mprAddHash(vars, "AUTH_USER", (req->user && *req->user) ? req->user : 0);
    mprAddHash(vars, "AUTH_GROUP", req->group);
    mprAddHash(vars, "AUTH_ACL", "");
    mprAddHash(vars, "CONTENT_LENGTH", req->contentLengthStr);
    mprAddHash(vars, "CONTENT_TYPE", req->mimeType);
    mprAddHash(vars, "DOCUMENT_ROOT", host->documentRoot);
    mprAddHash(vars, "GATEWAY_INTERFACE", "CGI/1.1");
    mprAddHash(vars, "QUERY_STRING", req->parsedUri->query);

    mprAddHash(vars, "REMOTE_ADDR", conn->remoteIpAddr);
    mprItoa(port, sizeof(port) - 1, conn->remotePort, 10);
    mprAddHash(vars, "REMOTE_PORT", mprStrdup(vars, port));


#if BLD_FEATURE_REVERSE_DNS && !WIN
    /*
     *  This feature has denial of service risks. Doing a reverse DNS will be slower,
     *  and can potentially hang the web server. Use at your own risk!!  Not supported for windows.
     */
    {
        struct addrinfo *result;
        char            name[MPR_MAX_STRING];
        int             rc;

        if (getaddrinfo(remoteIpAddr, NULL, NULL, &result) == 0) {
            rc = getnameinfo(result->ai_addr, sizeof(struct sockaddr), name, sizeof(name), NULL, 0, NI_NAMEREQD);
            freeaddrinfo(result);
            if (rc == 0) {
                mprAddHash(vars, "REMOTE_HOST", remoteIpAddr);
            }
        }
        mprAddHash(vars, "REMOTE_HOST", (rc == 0) ? name : remoteIpAddr);
    }
#else
    mprAddHash(vars, "REMOTE_HOST", conn->remoteIpAddr);
#endif

    /*
     *  Same as AUTH_USER (yes this is right)
     */
    mprAddHash(vars, "REMOTE_USER", (req->user && *req->user) ? req->user : 0);
    mprAddHash(vars, "REQUEST_METHOD", req->methodName);

#if BLD_FEATURE_SSL
    mprAddHash(vars, "REQUEST_TRANSPORT", (char*) ((host->secure) ? "https" : "http"));
#else
    mprAddHash(vars, "REQUEST_TRANSPORT", "http");
#endif
    
    listenSock = conn->sock->listenSock;
    mprAddHash(vars, "SERVER_ADDR", listenSock->ipAddr);
    mprItoa(port, sizeof(port) - 1, listenSock->port, 10);
    mprAddHash(vars, "SERVER_PORT", mprStrdup(req, port));
    mprAddHash(vars, "SERVER_NAME", host->name);
    mprAddHash(vars, "SERVER_PROTOCOL", req->parsedUri->scheme);
    mprAddHash(vars, "SERVER_SOFTWARE", MA_SERVER_NAME);

    /*
     *  This is the complete URI before decoding
     */ 
    mprAddHash(vars, "REQUEST_URI", req->parsedUri->originalUri);

    /*
     *  URLs are broken into the following: http://{SERVER_NAME}:{SERVER_PORT}{SCRIPT_NAME}{PATH_INFO}
     */
    mprAddHash(vars, "SCRIPT_NAME", req->url);
    mprAddHash(vars, "SCRIPT_FILENAME", resp->filename);
    mprAddHash(vars, "PATH_INFO", req->pathInfo);

    if (req->pathTranslated) {
        /*
         *  Only set PATH_TRANSLATED if PATH_INFO is set (CGI spec)
         */
        mprAddHash(vars, "PATH_TRANSLATED", req->pathTranslated);
    }
}


/*
 *  Add variables to the form params[] data. This comes from the query string and urlencoded post data.
 *  Make variables for each keyword in a query string. The buffer must be url encoded (ie. key=value&key2=value2..., 
 *  spaces converted to '+' and all else should be %HEX encoded).
 */
void maAddFormVars(MaConn *conn, cchar *buf, int len)
{
    MaResponse      *resp;
    MaRequest       *req;
    MprHashTable    *vars;
    cchar           *oldValue;
    char            *newValue, *decoded, *keyword, *value, *tok;

    resp = conn->response;
    req = conn->request;
    vars = req->formVars;
    
    mprAssert(vars);
    
    decoded = (char*) mprAlloc(resp, len + 1);
    decoded[len] = '\0';
    memcpy(decoded, buf, len);

    keyword = mprStrTok(decoded, "&", &tok);
    while (keyword != 0) {
        if ((value = strchr(keyword, '=')) != 0) {
            *value++ = '\0';
            mprUrlDecode(value, (int) strlen(value) + 1, value);
        } else {
            value = "";
        }
        mprUrlDecode(keyword, (int) strlen(keyword) + 1, keyword);

        if (*keyword) {
            /*
             *  Append to existing keywords.
             */
            oldValue = mprLookupHash(vars, keyword);
            if (oldValue != 0) {
                mprAllocSprintf(vars, &newValue, MA_MAX_HEADERS, "%s %s", oldValue, value);
                mprAddHash(vars, keyword, newValue);
                mprFree(newValue);
            } else {
                mprAddHash(vars, keyword, value);
            }
        }
        keyword = mprStrTok(0, "&", &tok);
    }
    /*
     *  Must not free "decoded". This will be freed when the response completes.
     */
}


int maTestFormVar(MaConn *conn, cchar *var)
{
    MprHashTable    *vars;
    
    vars = conn->request->formVars;
    return vars && mprLookupHash(vars, var) != 0;
}


cchar *maGetFormVar(MaConn *conn, cchar *var, cchar *defaultValue)
{
    MprHashTable    *vars;
    cchar           *value;
    
    vars = conn->request->formVars;

    if (vars) {
        value = mprLookupHash(vars, var);
        return (value) ? value : defaultValue;
    }
    return defaultValue;
}


int maGetIntFormVar(MaConn *conn, cchar *var, int defaultValue)
{
    MprHashTable    *vars;
    cchar           *value;
    
    vars = conn->request->formVars;

    if (vars) {
        value = mprLookupHash(vars, var);
        return (value) ? mprAtoi(value, 10) : defaultValue;
    }
    return defaultValue;
}


void maSetFormVar(MaConn *conn, cchar *var, cchar *value) 
{
    MprHashTable    *vars;
    
    vars = conn->request->formVars;
    mprAssert(vars);
    
    if (vars == 0) {
        return;
    }
    
    mprAddHash(vars, var, (void*) value);
}


void maSetIntFormVar(MaConn *conn, cchar *var, int value) 
{
    MprHashTable    *vars;
    char            buf[16];
    
    vars = conn->request->formVars;
    mprAssert(vars);
    
    if (vars == 0) {
        return;
    }
    
    mprItoa(buf, sizeof(buf) - 1, value, 10);
    mprAddHash(vars, var, buf);
}


int maCompareFormVar(MaConn *conn, cchar *var, cchar *value)
{
    MprHashTable    *vars;
    
    vars = conn->request->formVars;
    mprAssert(vars);
    
    if (vars == 0) {
        return 0;
    }
 
    if (strcmp(value, maGetFormVar(conn, var, " __UNDEF__ ")) == 0) {
        return 1;
    }
    return 0;
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

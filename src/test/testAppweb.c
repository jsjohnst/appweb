/*
 *  testAppweb.c - Main program for the Appweb unit tests
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "testAppweb.h"

/****************************** Test Definitions ******************************/

extern MprTestDef testAlias;
extern MprTestDef testAuth;
extern MprTestDef testCgi;
extern MprTestDef testEgi;
extern MprTestDef testEjs;
extern MprTestDef testGet;
extern MprTestDef testHttp;
extern MprTestDef testPhp;
extern MprTestDef testPost;
extern MprTestDef testUpload;
extern MprTestDef testVhost;

static MprTestDef *groups[] = 
{
    &testHttp,
    &testGet,
    &testPost,
#if BLD_FEATURE_AUTH
    &testAlias,
#endif
    &testAuth,
#if BLD_FEATURE_EGI && BLD_DEBUG
    &testEgi,
#endif
#if BLD_FEATURE_EJS
    &testEjs,
#endif
#if BLD_FEATURE_CGI
    &testCgi,
#endif
#if BLD_FEATURE_PHP
    &testPhp,
#endif
#if FUTURE
    &testUpload,
#endif
    &testVhost,
    0
};
 
static MprTestDef master = {
    "appweb",
    groups,
    0, 0, 
    { { 0 } },
};


static MprTestService  *ts;
static MprHttp         *http;
static int             defaultPort;
static char            *defaultHost;

/******************************* Forward Declarations *************************/

static void parseHostSwitch(MprCtx ctx, char **host, int *port);

/************************************* Code ***********************************/

int main(int argc, char *argv[]) 
{
    Mpr             *mpr;
    MprTestGroup    *gp;
    cchar           *programName;
    int             rc;

    mpr = mprCreate(argc, argv, 0);
    programName = mprGetBaseName(argv[0]);
    mprSetAppName(mpr, programName, programName, BLD_VERSION);

    defaultHost = "127.0.0.1";
    defaultPort = 4010;

    ts = mprCreateTestService(mpr);
    if (ts == 0) {
        mprError(mpr, "Can't create test service");
        exit(2);
    }
    
    if (mprParseTestArgs(ts, argc, argv) < 0) {
        mprErrorPrintf(mpr, "\n"
            "  Commands specifically for %s\n"
            "    --host ip:port      # Set the default host address for testing\n",
            programName);
        mprFree(mpr);
        exit(3);
    }
    
    parseHostSwitch(ts, &defaultHost, &defaultPort);
    gp = mprAddTestGroup(ts, &master);
    if (gp == 0) {
        exit(4);
    }

    http = mprCreateHttp(gp);
    assert(http != 0);
    mprSetHttpFollowRedirects(http, 0);

#if BLD_FEATURE_SSL
    if (!mprLoadSsl(mpr, 0)) {
        return 0;
    }
#endif

    /*
     *  Need a background event thread as we use the main thread to run the tests.
     */
    if (mprStart(mpr, MPR_SERVICE_THREAD)) {
        mprError(mpr, "Can't start mpr services");
        exit(5);
    }

    if (!mprGetDebugMode(gp)) {
        if (!simpleGet(gp, "/index.html", 0)) {
            mprError(mpr, "Can't access web server at http://%s:%d/index.html", defaultHost, defaultPort);
            exit(5);
        }
    }

    /*
     *  Run the tests and return zero if 100% success
     */
    rc = mprRunTests(ts);
    mprReportTestResults(ts);

    mprFree(mpr);
    return (rc == 0) ? 0 : -1;
}


static void parseHostSwitch(MprCtx ctx, char **host, int *port) 
{
    char    *ip, *cp;
    int     i;

    mprAssert(host);
    mprAssert(port);

    for (i = 0; i < ts->argc; i++) {
        if (strcmp(ts->argv[i], "--host") == 0) {
            ip = ts->argv[++i];
            if (ip == 0) {
                continue;
            }
            ip = mprStrdup(ctx, ip);
            if ((cp = strchr(ip, ':')) != 0) {
                *cp++ = '\0';
                *port = atoi(cp);
            } else {
                *port = 80;
            }
            *host = ip;
        }
    }
}


/*
 *  Convenience routines for all tests
 */
int httpRequest(MprHttp *http, cchar *method, cchar *uri)
{
    if (*uri == '/') {
        mprSetHttpDefaultPort(http, defaultPort);
        mprSetHttpDefaultHost(http, defaultHost);
    }
#if UNUSED
    mprSetHttpHeader(http, "X-Appweb-Chunk-Size", "0", 1);
#endif
    return mprHttpRequest(http, method, uri, 0);
}


bool simpleGet(MprTestGroup *gp, cchar *uri, int expectCode)
{
    int         code;

    if (expectCode <= 0) {
        expectCode = 200;
    }

    if (httpRequest(http, "GET", uri) < 0) {
        return 0;
    }

    code = mprGetHttpCode(http);

    assert(code == expectCode);
    if (code != expectCode) {
        mprLog(gp, 0, "simpleGet: HTTP response code %d, expected %d", code, expectCode);
        return 0;
    }

    assert(mprGetHttpError(http) != 0);
    assert(mprGetHttpContent(http) != 0);

    mprLog(gp, 4, "Response content %s", mprGetHttpContent(http));

    return 1;
}


bool simpleForm(MprTestGroup *gp, char *uri, char *formData, int expectCode)
{
    MprHttp     *http;
    cchar       *content;
    int         contentLen, code;

    content = 0;
    contentLen = 0;
    
    http = getHttp(gp);

    if (expectCode <= 0) {
        expectCode = 200;
    }

    if (formData) {
        mprSetHttpForm(http, formData, strlen(formData));
    }
    if (httpRequest(http, "POST", uri) < 0) {
        return 0;
    }

    code = mprGetHttpCode(http);
    if (code != expectCode) {
        mprLog(gp, 0, "Client failed for %s, response code: %d, msg %s\n", uri, code, mprGetHttpMessage(http));
        return 0;
    }
    content = mprGetHttpContent(http);
    contentLen = mprGetHttpContentLength(http);
    if (! assert(content != 0 && contentLen > 0)) {
        return 0;
    }
    mprLog(gp, 4, "Response content %s", content);
    return 1;
}


bool simplePost(MprTestGroup *gp, char *uri, char *bodyData, int len, int expectCode)
{
    MprHttp     *http;
    cchar       *content;
    int         contentLen, code;

    content = 0;
    contentLen = 0;
    
    http = getHttp(gp);

    if (expectCode <= 0) {
        expectCode = 200;
    }

    if (bodyData) {
        mprSetHttpBody(http, bodyData, len);
    }
    if (httpRequest(http, "POST", uri) < 0) {
        return 0;
    }

    code = mprGetHttpCode(http);
    if (code != expectCode) {
        mprLog(gp, 0, "Client failed for %s, response code: %d, msg %s\n", uri, code, mprGetHttpMessage(http));
        return 0;
    }
    content = mprGetHttpContent(http);
    contentLen = mprGetHttpContentLength(http);
    if (! assert(content != 0 && contentLen > 0)) {
        return 0;
    }
    mprLog(gp, 4, "Response content %s", content);
    return 1;
}


bool bulkPost(MprTestGroup *gp, char *url, int size, int expectCode)
{
    char    *post;
    int     i, j;
    bool    success;

    post = (char*) mprAlloc(gp, size + 1);
    assert(post != 0);

    for (i = 0; i < size; i++) {
        if (i > 0) {
            mprSprintf(&post[i], 10, "&%07d=", i / 64);
        } else {
            mprSprintf(&post[i], 10, "%08d=", i / 64);
        }
        for (j = i + 9; j < (i + 63); j++) {
            post[j] = 'a';
        }
        post[j] = '\n';
        i = j;
    }
    post[i] = '\0';

    success = simplePost(gp, url, post, strlen(post), expectCode);
    assert(success);

    mprFree(post);
    return success;
}


/*
 *  Return the shared http instance. Using this minimizes TIME_WAITS by using keep alive.
 */
MprHttp *getHttp(MprTestGroup *gp)
{
    return http;
}


/*
 *  Match a keyword in the content returned from the last request
 *  Format is:
 *
 *      KEYWORD = value
 */
bool match(MprTestGroup *gp, char *key, char *value)
{
    char    *vp;

    vp = lookupValue(gp, key);
    if (vp == 0 && value == 0) {
        return 1;
    }
    if (vp == 0 || value == 0 || strcmp(vp, value) != 0) {
        mprLog(gp, 1, "Match %s failed. Got \"%s\" expected \"%s\"", key, vp, value);
        return 0;
    }
    return 1;
}


/*
 *
 *  Match a keyword an ignore case for windows
 */
bool matchAnyCase(MprTestGroup *gp, char *key, char *value)
{
    char    *vp;

    vp = lookupValue(gp, key);
    if (vp == 0 && value == 0) {
        return 1;
    }
#if WIN
    if (vp == 0 || mprStrcmpAnyCase(vp, value) != 0)
#else
    if (vp == 0 || value == 0 || strcmp(vp, value) != 0)
#endif
    {
        mprLog(gp, 1, "Match %s failed. Got %s expected %s", key, vp, value);
        return 0;
    }
    return 1;
}


char *getValue(MprTestGroup *gp, char *key)
{
    char    *value;

    value = lookupValue(gp, key);
    if (value == 0) {
        return "";
    } else {
        return value;
    }
}


/*
 *  Return the value of a keyword in the content returned from the last request
 *  Format either: 
 *      KEYWORD=value<
 *      KEYWORD: value,
 *
 *  Return 0 on errors
 *  WARNING: this uses a static buffer and will not preserve the result across multiple calls.
 *  WARNING: this is not intended to be a rigorous parser. Just enough for our unit test needs.
 */
char *lookupValue(MprTestGroup *gp, char *key)
{
    cchar   *content;
    char    *nextToken, *cp, *bp;
    static char result[MPR_MAX_STRING];

    content = mprGetHttpContent(getHttp(gp));
    if (content == 0 || (nextToken = strstr(content, key)) == 0) {
        return 0;
    }
    nextToken += strlen(key);
    if (*nextToken != '=' && *nextToken != ':') {
        return 0;
    }

    if (*nextToken == ':') {
        cp = nextToken + 2;
    } else {
        cp = nextToken + 1;
    }

    for (bp = result; bp < &result[sizeof(result)] && *cp && *cp != '<' && *cp != ','; ) {
        *bp++ = *cp++;
    }
    *bp++ = '\0';
    if (strcmp(result, "null") == 0) {
        return 0;
    }
    return mprStrTrim(result, "\"");
}


int getDefaultPort(MprTestGroup *gp)
{
    return defaultPort;
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

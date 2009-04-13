/*
 *  testCgi.c - Unit tests for the CGI handler
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

#if BLD_FEATURE_CGI
/*********************************** Forwards *********************************/

static void setSwitches(MprTestGroup *gp, cchar *switches);

/************************************ Code ************************************/

static void basic(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/cgi-bin/cgiProgram", 0));
    assert(simpleGet(gp, "/cgiProgram.cgi", 0));
}


static void waysToInvoke(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/cgi-bin/cgiProgram", 0));
    assert(simpleGet(gp, "/cgi-bin/testScript", 0));

    /*
     *  YourScripts does not have a location block, just an alias so we must use one of the CGI extensions 
     */
    assert(simpleGet(gp, "/YourScripts/cgiProgram.cgi", 0));
}


static void waysToInvokeOnWin(MprTestGroup *gp)
{
#if WIN
    /*
     *  On windows you can find the program with or without the ".exe"
     */
    assert(simpleGet(gp, "/cgi-bin/cgiProgram.exe", 0));

    /*
     *  This emulates a Unix style "#!cgiProgram"
     */
    assert(simpleGet(gp, "/cgi-bin/test", 0));

    /*
     *  Standard batch file processor (cmd.exe)
     */
    assert(simpleGet(gp, "/cgi-bin/test.bat", 0));
#endif
}


static void extraPath(MprTestGroup *gp)
{
    char    dir[MPR_MAX_FNAME], translatedPath[MPR_MAX_FNAME];

    assert(simpleGet(gp, "/cgiProgram.cgi", 0));
    assert(match(gp, "PATH_INFO", ""));
    assert(match(gp, "PATH_TRANSLATED", ""));

    /*
     *  Add query to make sure the extra path gets terminated correctly
     */
    assert(simpleGet(gp, "/cgiProgram.cgi/extra/path?a=b&c=d&e=f", 0));
    assert(matchAnyCase(gp, "SCRIPT_NAME", "/cgiProgram.cgi"));
    assert(match(gp, "PATH_INFO", "/extra/path"));

    mprGetDirName(dir, sizeof(dir), getValue(gp, "SCRIPT_FILENAME"));
    mprSprintf(translatedPath, sizeof(translatedPath), "%s/extra/path", dir);
    assert(match(gp, "PATH_TRANSLATED", translatedPath));
}



static void queryString(MprTestGroup *gp)
{
    char    *post;

    assert(simpleGet(gp, "/cgi-bin/cgiProgram?a+b+c", 0));
    assert(match(gp, "QUERY_STRING", "a+b+c"));
    assert(match(gp, "QVAR a b c", ""));

    assert(simpleGet(gp, "/cgi-bin/cgiProgram/extra/path?var1=a+a&var2=b%20b&var3=c", 0));

    /*
     *  Query string vars should not be turned into variables for GETs
     */
    assert(matchAnyCase(gp, "SCRIPT_NAME", "/cgi-bin/cgiProgram"));
    assert(match(gp, "QUERY_STRING", "var1=a+a&var2=b%20b&var3=c"));
    assert(match(gp, "QVAR var1", "a a"));
    assert(match(gp, "QVAR var2", "b b"));
    assert(match(gp, "QVAR var3", "c"));

    /*
     *  Post data should be turned into variables
     */
    post = "name=Peter&address=777+Mulberry+Lane";
    assert(simpleForm(gp, "/cgi-bin/cgiProgram/extra/path?var1=a+a&var2=b%20b&var3=c", post, 0));
    assert(match(gp, "QUERY_STRING", "var1=a+a&var2=b%20b&var3=c"));
    assert(match(gp, "QVAR var1", "a a"));
    assert(match(gp, "QVAR var2", "b b"));
    assert(match(gp, "QVAR var3", "c"));
    assert(match(gp, "PVAR name", "Peter"));
    assert(match(gp, "PVAR address", "777 Mulberry Lane"));
}


static void args(MprTestGroup *gp)
{
    /*
     *  Note: args are split at '+' characters and are then shell character encoded. Typical use is "?a+b+c+d
     */
    assert(simpleGet(gp, "/cgi-bin/cgiProgram", 0));
    assert(strcmp(getValue(gp, "ARG[1]"), "") == 0);

    assert(simpleGet(gp, "/cgi-bin/cgiProgram/extra/path", 0));
    assert(strcmp(getValue(gp, "ARG[1]"), "") == 0);

    assert(simpleGet(gp, "/cgi-bin/cgiProgram?a+b+c", 0));
    assert(match(gp, "QUERY_STRING", "a+b+c"));

    assert(simpleGet(gp, "/cgi-bin/cgiProgram?var1=a+a&var2=b%20b&var3=c", 0));
    assert(match(gp, "QUERY_STRING", "var1=a+a&var2=b%20b&var3=c"));
}


static void encoding(MprTestGroup *gp)
{
    char    dir[MPR_MAX_FNAME], translatedPath[MPR_MAX_FNAME];

    assert(simpleGet(gp, "/cgi-bin/cgi%20Program/extra%20long/a/../path|/c/..?var%201=value%201", 0));
    assert(match(gp, "QUERY_STRING", "var%201=value%201"));
    assert(matchAnyCase(gp, "SCRIPT_NAME", "/cgi-bin/cgi Program"));
    assert(match(gp, "QVAR var 1", "value 1"));

    assert(match(gp, "PATH_INFO", "/extra long/path|/")); 
    mprGetDirName(dir, sizeof(dir), getValue(gp, "SCRIPT_FILENAME"));
    mprSprintf(translatedPath, sizeof(translatedPath), "%s/extra long/path|/", dir);
    assert(match(gp, "PATH_TRANSLATED", translatedPath));
}


static void alias(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/MyScripts/cgiProgram", 0));
    assert(match(gp, "QUERY_STRING", ""));
    assert(matchAnyCase(gp, "SCRIPT_NAME", "/MyScripts/cgiProgram"));
    assert(match(gp, "PATH_TRANSLATED", ""));
    assert(match(gp, "PATH_INFO", ""));
}


static void status(MprTestGroup *gp)
{
    MprHttp     *http;
    char        *uri;
    int         rc, code;

    http = getHttp(gp);

    /*
     *  Have cgiProgram exit with a 711 status
     */
    setSwitches(gp, "-s%20711");
    uri = "/cgi-bin/cgiProgram";

    rc = httpRequest(http, "GET", uri);
    assert(rc == 0);
    if (rc != 0) {
        return;
    }
    code = mprGetHttpCode(http);
    assert(code == 711);
    if (code != 711) {
        mprLog(gp, 0, "Client failed for %s, response code: %d, msg %s\n", uri, code, mprGetHttpMessage(http));
    }
}


static void location(MprTestGroup *gp)
{
    MprHttp     *http;
    char        *uri;
    int         rc;

    http = getHttp(gp);

    /*
     *  Have cgiProgram redirect to. We should not get a 302 but rather be be given the redirected content
     */
    uri = "/cgi-bin/cgiProgram";
    setSwitches(gp, "-l%20/index.html");
    rc = simpleGet(gp, uri, 302);
    assert(rc == 1);
    if (rc != 1) {
        return;
    }    
}



static void nph(MprTestGroup *gp)
{
    MprHttp     *http;
    cchar       *uri, *data;
    int         rc, code;

    http = getHttp(gp);

    /*
     *  Have cgiProgram redirect to. We should not get a 302 but rather be be given the redirected content
     */
    uri = "/cgi-bin/nph-cgiProgram";
    setSwitches(gp, "-n");

    rc = httpRequest(http, "GET", uri);
    assert(rc == 0);
    if (rc != 0) {
        return;
    }
    code = mprGetHttpCode(http);
    assert(code == 200);
    if (code != 200) {
        mprLog(gp, 0, "Client failed for %s, response code: %d, msg %s\n", uri, code, mprGetHttpMessage(http));
    }
    if (code != 200) {
        return;
    }
    data = mprGetHttpContent(http);
    assert(strstr(data, "HTTP/1.0") != 0);
    assert(strstr(data, "X-CGI-CustomHeader: Any value at all") != 0);
}



static void toughArgQuoting(MprTestGroup *gp)
{
    /*
     *  Ensure that args are passed via scripts correctly
     */
    assert(simpleGet(gp, "/cgi-bin/testScript?a+b+c", 0));
    assert(match(gp, "QUERY_STRING", "a+b+c"));
    assert(match(gp, "QVAR a b c", ""));

    assert(simpleGet(gp, "/cgi-bin/testScript?a=1&b=2&c=3", 0));
    assert(match(gp, "QUERY_STRING", "a=1&b=2&c=3"));
    assert(match(gp, "QVAR a", "1"));
    assert(match(gp, "QVAR b", "2"));
    assert(match(gp, "QVAR c", "3"));

    assert(simpleGet(gp, "/cgi-bin/testScript?a%20a=1%201+b%20b=2%202", 0));
    assert(match(gp, "QUERY_STRING", "a%20a=1%201+b%20b=2%202"));
    assert(match(gp, "QVAR a a", "1 1 b b=2 2"));

    assert(simpleGet(gp, "/cgi-bin/testScript?a%20a=1%201&b%20b=2%202", 0));
    assert(match(gp, "QUERY_STRING", "a%20a=1%201&b%20b=2%202"));
    assert(match(gp, "QVAR a a", "1 1"));
    assert(match(gp, "QVAR b b", "2 2"));

    assert(simpleGet(gp, "/cgi-bin/testScript?a|b+c>d+e?f+g>h+i'j+k\"l+m%20n", 0));
    assert(match(gp, "ARG[2]", "a\\|b"));
    assert(match(gp, "ARG[3]", "c\\>d"));
    assert(match(gp, "ARG[4]", "e\\?f"));
    assert(match(gp, "ARG[5]", "g\\>h"));
    assert(match(gp, "ARG[6]", "i\\'j"));
#if WIN
	/*
	 *	TODO - Windows seems to be eating the backslash
	 */
    assert(match(gp, "ARG[7]", "k\"l"));
#else
    assert(match(gp, "ARG[7]", "k\\\"l"));
#endif
    assert(match(gp, "ARG[8]", "m n"));
    assert(match(gp, "QUERY_STRING", "a|b+c>d+e?f+g>h+i'j+k\"l+m%20n"));
    assert(match(gp, "QVAR a|b c>d e?f g>h i'j k\"l m n", ""));
}


static void setSwitches(MprTestGroup *gp, cchar *switches)
{
    MprHttp     *http;

    http = getHttp(gp);
    mprSetHttpHeader(http, "SWITCHES", switches ? switches : "", 1);
}


MprTestDef testCgi = {
    "cgi", 0, 0, 0,
    {
        MPR_TEST(0, basic),
        MPR_TEST(0, waysToInvoke),
        MPR_TEST(0, waysToInvokeOnWin),
        MPR_TEST(0, extraPath),
        MPR_TEST(0, queryString),
        MPR_TEST(0, args),
        MPR_TEST(0, encoding),
        MPR_TEST(0, alias),
        MPR_TEST(0, status),
        MPR_TEST(0, location),
        MPR_TEST(0, nph),
        MPR_TEST(0, toughArgQuoting),
        MPR_TEST(0, 0),
    },
};

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

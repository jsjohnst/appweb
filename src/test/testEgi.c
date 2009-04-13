/*
 *  testEgi.c - Unit tests for the EGI handler
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

/*
 *	Only test in debug mode as it requires test code in the server itself.
 */
#if BLD_FEATURE_EGI && BLD_DEBUG
/*********************************** Code *************************************/

static void basic(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/egi/egiProgram", 0));
    assert(simpleGet(gp, "/egiProgram.egi", 0));
}


static void queryString(MprTestGroup *gp)
{
    char    *post;

    post = "name=Peter&address=777+Mulberry+Lane";
    assert(simpleForm(gp, "/egi/egiProgram.egi?var1=a+a&var2=b%20b&var3=c", post, 0));
    assert(match(gp, "QUERY_STRING", "var1=a+a&var2=b%20b&var3=c"));
    assert(match(gp, "QVAR var1", "a a"));
    assert(match(gp, "QVAR var2", "b b"));
    assert(match(gp, "QVAR var3", "c"));
    assert(match(gp, "PVAR name", "Peter"));
    assert(match(gp, "PVAR address", "777 Mulberry Lane"));

    assert(simpleGet(gp, "/egi/egiProgram.egi?var1=a+a&var2=b%20b&var3=c", 0));

    /*
     *  Query string vars should not be turned into variables for GETs
     */
    assert(matchAnyCase(gp, "SCRIPT_NAME", "/egi/egiProgram.egi"));
    assert(match(gp, "QUERY_STRING", "var1=a+a&var2=b%20b&var3=c"));
    assert(match(gp, "QVAR var1", "a a"));
    assert(match(gp, "QVAR var2", "b b"));
    assert(match(gp, "QVAR var3", "c"));
}


static void encoding(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/egi/egi%20Program.egi?var%201=value%201", 0));
    assert(match(gp, "QUERY_STRING", "var%201=value%201"));
    assert(matchAnyCase(gp, "SCRIPT_NAME", "/egi/egi Program.egi"));
    assert(match(gp, "QVAR var 1", "value 1"));
}


static void alias(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/MyInProcScripts/egiProgram.egi", 0));
    assert(match(gp, "QUERY_STRING", ""));
    assert(matchAnyCase(gp, "SCRIPT_NAME", "/MyInProcScripts/egiProgram.egi"));
    assert(match(gp, "PATH_INFO", ""));
    assert(match(gp, "PATH_TRANSLATED", NULL));
}


static void status(MprTestGroup *gp)
{
    MprHttp     *http;
    char        *uri;
    int         rc, code;

    http = getHttp(gp);

    /*
     *  Have egiProgram exit with a 711 status
     */
    uri = "/egi/egiProgram?SWITCHES=-s%20711";

    rc = httpRequest(http, "GET", uri);
    assert(rc == 0);
    if (rc != 0) {
        return;
    }
    code = mprGetHttpCode(http);
    if (code != 711) {
        mprLog(gp, 0, "Client failed for %s, response code: %d, msg %s\n", uri, code, mprGetHttpMessage(http));
    }
}


static void location(MprTestGroup *gp)
{
    MprHttp     *http;
    char        *uri;
    int         rc, code;

    http = getHttp(gp);
    
    uri = "/egi/egiProgram?SWITCHES=-l%20http://www.redhat.com/";
    rc = httpRequest(http, "GET", uri);
    assert(rc == 0);
    if (rc != 0) {
        return;
    }
    code = mprGetHttpCode(http);
    if (code != 302) {
        mprLog(gp, 0, "Client failed for %s, response code: %d, msg %s\n", uri, code, mprGetHttpMessage(http));
    }
}


MprTestDef testEgi = {
    "egi", 0, 0, 0,
    {
        MPR_TEST(0, basic),
        MPR_TEST(0, queryString),
        MPR_TEST(0, encoding),
        MPR_TEST(0, alias),
        MPR_TEST(0, status),
        MPR_TEST(0, location),
        MPR_TEST(0, 0),
    },
};

#endif /* BLD_FEATURE_EGI */

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


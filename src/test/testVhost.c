/*
 *  testVhost.c - Unit tests for virtual hosts
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

/********************************** Defines ***********************************/
/*
 *  The thorough Vhost tests can only be run on the local host because
 *  we don't want to require people to modify their hosts files to setup
 *  named virtual hosts. Luckily, each system already has four unique names
 *  for itself: localhost, 127.0.0.1, its IP address and its name. 
 *
 *  We expect to have a configuration (see http.conf) :
 *      Standard HTTP: port
 *      Virtual host: port + 2
 *      Virtual host: port + 3
 */
#define TEST_HOST_1     "localhost"
#define TEST_HOST_2     "127.0.0.1"

/********************************* Forwards ***********************************/

static bool get(MprTestGroup *gp, cchar *host, int port, cchar *uri, int expectCode);
extern int  getDefaultPort(MprTestGroup *gp);

/*********************************** Code *************************************/

static void basic(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/index.html", 0));
#if BLD_FEATURE_EGI && BLD_DEBUG
    assert(simpleGet(gp, "/egi/egiProgram", 0));
#endif
#if BLD_FEATURE_EJS
    assert(simpleGet(gp, "/test.ejs", 0));
#endif
}


static void mainServer(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/mainServer.html", 0));
}


static void ipHost(MprTestGroup *gp)
{
    /*
     *  Get a unique file 
     */
    assert(get(gp, NULL, 2, "/ipHost.ejs", 0));

    /*
     *  These should fail (wrong urls for this host)
     */
    assert(get(gp, NULL, 2, "/mainServer.html", 404));
    assert(get(gp, NULL, 2, "/local1.html", 404));
    assert(get(gp, NULL, 2, "/local2.html", 404));
}


static void namedHost(MprTestGroup *gp)
{
    assert(get(gp, TEST_HOST_1, 1, "/local1.html", 0));
    assert(get(gp, TEST_HOST_2, 1, "/index.ejs", 0));

    /*
     *  These should fail
     */
    assert(get(gp, TEST_HOST_1, 1, "/index.ejs", 404));
    assert(get(gp, TEST_HOST_1, 1, "/mainServer.html", 404));
    assert(get(gp, NULL, 1, "/local2.html", 404));
}


static void inheritHandlers(MprTestGroup *gp)
{
    /*
     *  The virtual host with the name TEST_HOST_1 inherits handlers from the main server (but not directories or location)
     */
    assert(get(gp, TEST_HOST_1, 1, "/local1.ejs", 0));
}


static void nestedLocation(MprTestGroup *gp)
{
#if BLD_FEATURE_EGI && BLD_DEBUG
    assert(get(gp, TEST_HOST_2, 1, "/myEgi/egiProgram", 0));
#endif
}


/*
 *  Test authorization within a virtual host
 */
static void auth(MprTestGroup *gp)
{
    MprHttp     *http;

    http = getHttp(gp);

    assert(get(gp, NULL, 2, "/acme.html", 0));

    mprSetHttpCredentials(http, "mary", "pass2");
    assert(get(gp, NULL, 2, "/protected/private.html", 0));
    mprResetHttpCredentials(http);
}


/*
 *  Just like simpleGet but with an explicit port number
 */
static bool get(MprTestGroup *gp, cchar *host, int port, cchar *uri, int expectCode)
{
    MprHttp     *http;
    int         code;

    http = getHttp(gp);

    if (expectCode <= 0) {
        expectCode = 200;
    }
    if (host) {
        mprSetHttpDefaultHost(http, host);
    }
    if (port > 0) {
        mprSetHttpDefaultPort(http, getDefaultPort(gp) + port);
    }

    if (mprHttpRequest(http, "GET", uri, 0) < 0) {
        return 0;
    }

    code = mprGetHttpCode(http);
    assert(code == expectCode);

    if (code != expectCode) {
        mprLog(gp, 0, "get: HTTP response code %d, expected %d", code, expectCode);
        return 0;
    }

    assert(mprGetHttpError(http) != 0);
    assert(mprGetHttpContent(http) != 0);

    return 1;
}


MprTestDef testVhost = {
    "vhost", 0, 0, 0,
    {
        MPR_TEST(0, basic),
        MPR_TEST(0, mainServer),
        MPR_TEST(0, ipHost),
        MPR_TEST(0, namedHost),
        MPR_TEST(0, inheritHandlers),
        MPR_TEST(0, nestedLocation),
        MPR_TEST(0, auth),
        MPR_TEST(0, 0),
    },
};


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

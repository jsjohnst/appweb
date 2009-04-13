/*
 *  testGet.c - Unit tests for the HTTP GET method. 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

/****************************** Test Definitions ******************************/

static void basic(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/index.html", 0));
    assert(simpleGet(gp, "/simpleDir/index.html", 0));
}


static void dir(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/simpleDir/", 0));
}


static void alias(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/AliasForMyDocuments/index.html", 0));
}


static void query(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/simpleDir/index.html?a&b&c", 0));
    assert(simpleGet(gp, "/simpleDir/index.html?a=x&b=y&c=z", 0));
}


static void withCustomHeader(MprTestGroup *gp)
{
    MprHttp     *http;
    int         rc;

    http = getHttp(gp);
    assert(http != 0);

    mprSetHttpHeader(http, "MyFunnyKeyword", "any value at all", 1);

    rc = httpRequest(http, "GET", "/index.html");
    assert(rc == 0);
    if (rc < 0) {
        assert(mprGetHttpCode(http) == 200);
        assert(mprGetHttpContent(http) != 0);
    }
}


MprTestDef testGet = {
    "get", 0, 0, 0,
    {
        MPR_TEST(0, basic),
        MPR_TEST(0, dir),
        MPR_TEST(0, alias),
        MPR_TEST(0, query),
        MPR_TEST(0, withCustomHeader),
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

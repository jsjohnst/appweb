/*
 *  testPost.c - Unit tests for HTTP POST method
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

/*********************************** Code *************************************/

static void basic(MprTestGroup *gp)
{
    char    *form;

    form = "name=Peter&Address=777+Mulberry+Lane";

#if BLD_FEATURE_EJS
    assert(simpleForm(gp, "/test.ejs", form, 0));
#endif
#if BLD_FEATURE_EGI && BLD_DEBUG
    assert(simpleForm(gp, "/egi/egiProgram", form, 0));
#endif
#if BLD_FEATURE_CGI
    assert(simpleForm(gp, "/cgi-bin/cgiProgram", form, 0));
#endif
}


static void medium(MprTestGroup *gp)
{
#if BLD_FEATURE_CGI
    //  TODO - move to the cgi module
    assert(bulkPost(gp, "/cgi-bin/cgiProgram", 1 * 1024, 200));
#endif
#if 0
#if BLD_FEATURE_EJS
    assert(bulkPost(gp, "/test.ejs", 128 * 1024, 200));
    assert(bulkPost(gp, "/test.ejs", 1024, 200));
#endif
#if BLD_FEATURE_EGI && BLD_DEBUG
    assert(bulkPost(gp, "/egi/egiProgram", 128 * 1024, 200));
#endif
#endif
}


static void large(MprTestGroup *gp)
{
#if BLD_FEATURE_CGI
    //  TODO - move to the cgi module
    assert(bulkPost(gp, "/cgi-bin/cgiProgram", 10 * 1024 * 1024, 200));
#endif
}


#if MANUAL
static void bad(MprTestGroup *gp)
{
    MprHttp     *http;
    char        *post;
    int         rc;

    http = getHttp(gp);
    post = "name=Peter&Address=777+Mulberry+Lane";

    /*
     *  Post to static pages (should fail)
     */
    rc = httpRequest(http, "POST", "/index.html");
    assert(rc == 0);
    assert(mprGetHttpCode(http) == 405);
}
#endif


MprTestDef testPost = {
    "post", 0, 0, 0,
    {
        MPR_TEST(0, basic),
        MPR_TEST(0, medium),
        MPR_TEST(0, large),
#if MANUAL_ONLY
        MPR_TEST(0, bad),
#endif
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

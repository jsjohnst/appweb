/*
 *  testEjs.c - Unit tests for the Ejscript handler
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/*
 *  TODO
 *  - test form data as objects    user.name.first
 *  - Ejs applications
 *  - View controls
 */
/********************************** Includes **********************************/

#include    "testAppweb.h"

#if BLD_FEATURE_EJS
/*********************************** Code *************************************/

static void basic(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/ejsProgram.ejs", 0));

#if WIN
    /*
     *  Windows should be case insensitive
     */
    assert(simpleGet(gp, "/ejsProgram.EjS", 0));
#endif
}


static void queryString(MprTestGroup *gp)
{
    char    *post;

    assert(simpleGet(gp, "/ejs/ejsProgram.ejs?var1=a+a&var2=b%20b&var3=c", 0));

    /*
     *  Query string vars should not be turned into variables for GETs
     */
    assert(match(gp, "url", "/ejs/ejsProgram.ejs"));
    assert(match(gp, "query", "var1=a+a&var2=b%20b&var3=c"));
    assert(match(gp, "var1", "a a"));
    assert(match(gp, "var2", "b b"));
    assert(match(gp, "var3", "c"));

    /*
     *  Post data should be turned into variables
     */
    post = "firstname=Peter&address=777+Mulberry+Lane";
    assert(simpleForm(gp, "/ejs/ejsProgram.ejs?var1=a+a&var2=b%20b&var3=c", post, 0));
    assert(match(gp, "query", "var1=a+a&var2=b%20b&var3=c"));
    assert(match(gp, "var1", "a a"));
    assert(match(gp, "var2", "b b"));
    assert(match(gp, "var3", "c"));
    assert(match(gp, "firstname", "Peter"));
    assert(match(gp, "address", "777 Mulberry Lane"));
}


static void encoding(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/ejs/ejsProgram.ejs?var%201=value%201", 0));
    assert(match(gp, "query", "var%201=value%201"));
    assert(match(gp, "url", "/ejs/ejsProgram.ejs"));
    assert(match(gp, "var 1", "value 1"));
}


static void alias(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/SimpleAlias/ejsProgram.ejs", 0));
    assert(match(gp, "query", NULL));
    assert(matchAnyCase(gp, "url", "/SimpleAlias/ejsProgram.ejs"));
    assert(match(gp, "pathTranslated", NULL));
    assert(match(gp, "pathInfo", NULL));
}


MprTestDef testEjs = {
    "ejs", 0, 0, 0,
    {
        MPR_TEST(0, basic),
        MPR_TEST(0, queryString),
        MPR_TEST(0, encoding),
        MPR_TEST(0, alias),
        MPR_TEST(0, 0),
    },
};
#endif /* BLD_FEATURE_EJS */

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

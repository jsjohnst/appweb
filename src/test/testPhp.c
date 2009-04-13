/*
 *  testPhp.c - Unit tests for  the PHP Handler 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

#if BLD_FEATURE_PHP
/*********************************** Code *************************************/

static void basic(MprTestGroup *gp)
{
    assert(simpleGet(gp, "/test.php", 0));
}


static void basicPost(MprTestGroup *gp)
{
    char    *post;

    post = "name=Peter&address=777+Mulberry+Lane";

    assert(simpleForm(gp, "/form.php", post, 0));
}


static void longResponse(MprTestGroup *gp)
{
    char    *post;

    post = "iterations=999";

    assert(simpleForm(gp, "/form.php", post, 0));
}


static void mediumPost(MprTestGroup *gp)
{
    assert(bulkPost(gp, "/form.php", 16 * 1024, 0));
}


static void longPost(MprTestGroup *gp)
{
    assert(bulkPost(gp, "/form.php", 128 * 1024, 0));
}


MprTestDef testPhp = {
    "php", 0, 0, 0,
    {
#if !WIN
        MPR_TEST(0, basic),
        MPR_TEST(0, basicPost),
        MPR_TEST(0, longResponse),
        MPR_TEST(0, mediumPost),
        MPR_TEST(0, longPost),
#endif
        MPR_TEST(0, 0),
    },
};

#else
void mprTestPhpDummy();
#endif /* BLD_FEATURE_PHP5 */

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

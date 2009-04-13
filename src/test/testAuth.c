/*
 *  testAuth.c - Unit tests for basic and digest authorization.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

/*********************************** Forwards *********************************/

static bool authGet(MprTestGroup *gp, char *uri, char *realm, char *user, char *password, int expectCode);

/************************************* Code ***********************************/

static char *authType = "basic";

/*
 *  WARNING: we come through most routines twice. Once for basic auth and once for digest auth.
 */

static void simple(MprTestGroup *gp)
{
    char    *saveType;

    /*
     *  Access non-protected URIs
     */
    saveType = authType;
    authType = 0;
    assert(authGet(gp, "/index.html", "Acme Inc", "joshua", "PASSWORD WONT MATTER", 200));
    authType = saveType;

    /*
     *  Access a protected part of the acme site (any valid-user will do)
     */
    assert(authGet(gp, "/acme/acme.html", "Acme Inc", "joshua", "pass1", 200));

#if WIN
    /*
     *  Windows should be case insensitive
     */
    assert(authGet(gp, "/acME/ACme.html", "Acme Inc", "joshua", "pass1", 200));
#endif
}


static void badPassword(MprTestGroup *gp)
{
    /*
     *  Test a bad password
     */
    assert(authGet(gp, "/acme/acme.html", "Acme Inc", "joshua", "WRONG PASSWORD", 401));
#if WIN
    /*
     *  Should still be blocked when changing case
     */
    assert(authGet(gp, "/acME/acme.html", "Acme Inc", "joshua", "WRONG PASSWORD", 401));
#endif
}


static void groupAccess(MprTestGroup *gp)
{
    /*
     *  /acme/group requires users in the exec group. Mary is.
     */
    assert(authGet(gp, "/acme/group/acmeGroup.html", "Acme Inc", "mary", "pass2", 200));
    
    /*
     *  Joshua is not
     */
    assert(authGet(gp, "/acme/group/acmeGroup.html", "Acme Inc", "joshua", "pass1", 401));
}


static void userAccess(MprTestGroup *gp)
{
    /*
     *  /acme/joshua requires user joshua
     */
    assert(authGet(gp, "/acme/joshua/joshuaIndex.html", "Acme Inc", "joshua", "pass1", 200));
    
    /*
     *  Valid acme users should fail to access group files but are okay for top level files
     */
    assert(authGet(gp, "/acme/joshua/joshuaIndex.html", "Acme Inc", "mary", "pass2", 401));
    assert(authGet(gp, "/acme/acme.html", "Acme Inc", "mary", "pass2", 200));

    /*
     *  All access by Acme's competitor coyote should fail. But they can access their own files.
     */
    assert(authGet(gp, "/acme/acme.html", "Acme Inc", "peter", "pass3", 401));
    assert(authGet(gp, "/acme/group/acmeGroup.html", "Acme Inc", "peter", "pass3", 401));

    assert(authGet(gp, "/coyote/coyote.html", "Coyote Corp", "peter", "pass3", 200));
}


static void switchToDigest(MprTestGroup *gp)
{
    authType = "digest";
}


static bool authGet(MprTestGroup *gp, char *uri, char *realm, char *user, char *password, int expectCode)
{
    MprHttp     *http;
    char        uriBuf[MPR_MAX_STRING];
    cchar       *content;
    int         rc, code, contentLength;

    http = getHttp(gp);

    if (authType) {
        mprSprintf(uriBuf, sizeof(uriBuf), "/%s%s", authType, uri);
        mprSetHttpCredentials(http, user, password);

    } else {
        mprStrcpy(uriBuf, sizeof(uriBuf), uri);
        mprResetHttpCredentials(http);
    }

    rc = httpRequest(http, "GET", uriBuf);
    if (!assert(rc == 0)) {
        return 0;
    }

    code = mprGetHttpCode(http);
    assert(code == expectCode);
    if (code != expectCode) {
        mprLog(gp, 0, "Client failed for %s, response code: %d, expected %d, msg %s\n", uriBuf, code, expectCode,
                mprGetHttpMessage(http));
        return 0;
    }

    if (expectCode != 200) {
        contentLength = mprGetHttpContentLength(http);
        content = mprGetHttpContent(http);
        if (! assert(content != 0 && contentLength > 0)) {
            return 0;
        }
    }
    return 1;
}


MprTestDef testAuth = {
    "auth", 0, 0, 0,
    {
        MPR_TEST(0, simple),
        MPR_TEST(0, badPassword),
        MPR_TEST(0, userAccess),
        MPR_TEST(0, groupAccess),
        MPR_TEST(0, switchToDigest),
        MPR_TEST(0, simple),
        MPR_TEST(0, badPassword),
        MPR_TEST(0, userAccess),
        MPR_TEST(0, groupAccess),
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

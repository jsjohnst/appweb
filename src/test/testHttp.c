/*
 *  testHttp.c - Test URL validation and encoding routines
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

/********************************** Forwards **********************************/

static bool isValidUri(MprTestGroup *gp, char *uri, char *expectedUri);
static bool okEscapeUri(MprTestGroup *gp, char *uri, char *expectedUri);
static bool okEscapeCmd(MprTestGroup *gp, char *cmd, char *validCmd);
static bool okEscapeHtml(MprTestGroup *gp, char *html, char *expectedHtml);

/*********************************** Code *************************************/

static void validateUri(MprTestGroup *gp)
{
    assert(isValidUri(gp, "", ""));
    assert(isValidUri(gp, "/", "/"));
    assert(isValidUri(gp, "/index.html", "/index.html"));
    assert(isValidUri(gp, "/a/index.html", "/a/index.html"));

    assert(isValidUri(gp, "..", ""));
    assert(isValidUri(gp, "../", ""));
    assert(isValidUri(gp, "/..", ""));
    assert(isValidUri(gp, "/a/..", "/"));
    assert(isValidUri(gp, "/a/../", "/"));
    assert(isValidUri(gp, "/a/../b/..", "/"));
    assert(isValidUri(gp, "../a/b/..", "a/"));

    assert(isValidUri(gp, "./", ""));
    assert(isValidUri(gp, "./.", ""));
    assert(isValidUri(gp, "././", ""));
    assert(isValidUri(gp, "/a/./", "/a/"));
    assert(isValidUri(gp, "/a/./.", "/a/"));
    assert(isValidUri(gp, "/a/././", "/a/"));
    assert(isValidUri(gp, "/a/.", "/a/"));

    assert(isValidUri(gp, "/*a////b/", "/*a/b/"));
    assert(isValidUri(gp, "/*a/////b/", "/*a/b/"));

#if WIN || NW || OS2
    assert(isValidUri(gp, "\\a\\b\\", "/a/b/"));
#else
    assert(isValidUri(gp, "\\a\\b\\", "\\a\\b\\"));
#endif
}


static void escape(MprTestGroup *gp)
{
    /*  
     *  URI unsafe chars are:
     *      0x00-0x1F, 0x7F, 0x80-0xFF, <>'"#%{}|\^~[]
     *      Space, \t, \r, \n
     *      Reserved chars with special meaning are:
     *          ;/?:@=& 
     *      FUTURE -- should we not allow "'~"
     *      FUTURE -- shold we not allow ";?" but allow "/?:@=&"
     */
    assert(okEscapeUri(gp, " \t\r\n\x01\x7f\xff?<>\"#%{}|\\^[]?;", 
        "+%09%0d%0a%01%7f%ff%3f%3c%3e%22%23%25%7b%7d%7c%5c%5e%5b%5d%3f%3b"));
    assert(okEscapeCmd(gp, "&;`'\"|*?~<>^()[]{}$\\\n", 
        "\\&\\;\\`\\\'\\\"\\|\\*\\\?\\~\\<\\>\\^\\(\\)\\[\\]\\{\\}\\$\\\\\\\n"));
    assert(okEscapeHtml(gp, "<>&", "&lt;&gt;&amp;"));
    assert(okEscapeHtml(gp, "#()", "&#35;&#40;&#41;"));
}


static void descape(MprTestGroup *gp)
{
    char    *uri, escaped[MPR_MAX_STRING], descaped[MPR_MAX_STRING];
    bool    match;
    int     i;

    uri = " \t\r\n\x01\x7f\xff?<>\"#%{}|\\^[]?;";
    mprEncode64(escaped, sizeof(escaped), uri);
    mprDecode64(descaped, sizeof(descaped), escaped);
    match = (strcmp(descaped, uri) == 0);
    if (!match) {
        mprLog(gp, 0, "Uri \"%s\" descaped to \"%s\" from \"%s\"\n", uri, descaped, escaped);
        mprLog(gp, 0, "Lengths %d %d\n", strlen(descaped), strlen(uri));
        for (i = 0; i < (int) strlen(descaped); i++) {
            mprLog(gp, 0, "Chr[%d] descaped %x, uri %x\n", i, descaped[i], uri[i]);
        }
    }
    assert(match);
}


static bool isValidUri(MprTestGroup *gp, char *uri, char *expectedUri)
{
    char    buf[MPR_MAX_STRING];

    mprStrcpy(buf, sizeof(buf), uri);
    mprValidateUrl(buf);
    if (strcmp(expectedUri, buf) == 0) {
        return 1;
    } else {
        mprLog(gp, 0, "Uri \"%s\" validated to \"%s\" instead of \"%s\"\n", uri, buf, expectedUri);
        return 0;
    }
}


static bool okEscapeUri(MprTestGroup *gp, char *uri, char *expectedUri)
{
    char    buf[MPR_MAX_STRING];

    mprStrcpy(buf, sizeof(buf), uri);
    mprUrlEncode(buf, sizeof(buf), uri);
    if (strcmp(expectedUri, buf) == 0) {
        return 1;
    } else {
        mprLog(gp, 0, "Uri \"%s\" is escaped to be \n" "\"%s\" instead of \n\"%s\"\n", uri, buf, expectedUri);
        return 0;
    }
}


static bool okEscapeCmd(MprTestGroup *gp, char *cmd, char *validCmd)
{
    char    buf[MPR_MAX_STRING];

    mprStrcpy(buf, sizeof(buf), cmd);
    mprEscapeCmd(buf, sizeof(buf), cmd, '\\');
    if (strcmp(validCmd, buf) == 0) {
        return 1;
    } else {
        mprLog(gp, 0, "Cmd \"%s\" is escaped to be \n"
            "\"%s\" instead of \n"
            "\"%s\"\n", cmd, buf, validCmd);
        return 0;
    }
}


static bool okEscapeHtml(MprTestGroup *gp, char *html, char *expectedHtml)
{
    char    buf[MPR_MAX_STRING];

    mprStrcpy(buf, sizeof(buf), html);
    mprEscapeHtml(buf, sizeof(buf), html);
    if (strcmp(expectedHtml, buf) == 0) {
        return 1;
    } else {
        mprLog(gp, 0, "HTML \"%s\" is escaped to be \n"
            "\"%s\" instead of \n"
            "\"%s\"\n", html, buf, expectedHtml);
        return 0;
    }
}


MprTestDef testHttp = {
    "http", 0, 0, 0,
    {
        MPR_TEST(0, validateUri),
        MPR_TEST(0, escape),
        MPR_TEST(0, descape),
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

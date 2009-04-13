/*
 *  testUpload.c - Unit tests for file upload
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************** Includes **********************************/

#include    "testAppweb.h"

#if BLD_FEATURE_UPLOAD && BLD_FEATURE_CGI && 0
/*********************************** Code *************************************/

void quick(MprTestGroup *gp)
{
    /*
     *  Test one medium sized upload
     */
    uploadFile(gp, 38000);
}


void small(MprTestGroup *gp)
{
    int     size;

    /*
     *  Test very small file uploads
     */
    for (size = 0; size < 8; size++) {
        uploadFile(gp, size);
    }
}


void medium(MprTestGroup *gp)
{
    int     size;

    /*
     *  Test medium file uploads. Choose a value that is right at an important internal buffer boundary for the upload 
     *  handler. This number is from internal (debugging) knowlege.
     */
    for (size = 3830; size < 3830 + 10; size++) {
        uploadFile(gp, size);
    }
}


void large(MprTestGroup *gp)
{
    int     i, size;

    /*
     *  Test medium file uploads. Choose a value that is right at an important internal buffer boundary for the upload 
     *  handler. This number is from internal (debugging) knowlege.
     */
    for (i = 1; i < 8; i++) {
        size = (512 << (i - 1));
        uploadFile(gp, size);
    }
}


void huge(MprTestGroup *gp)
{
    int     i, size;

    /*
     *  Test medium file uploads. Choose a value that is right at an important internal buffer boundary for the 
     *  upload handler. This number is from internal (debugging) knowlege.
     */
    for (i = 8; i < 14; i++) {
        size = (512 << (i - 1));
        uploadFile(gp, size);
    }
}


int uploadFile(MprTestGroup *gp, int size)
{
#if UNUSED
    MprCmd  *run;
    char    cmd[MPR_MAX_FNAME];
    char    *path;
    int     rc, status;

    path = "/tmp/uploadData";
    if (makeTestFile(gp, path, size) < 0) {
        return MPR_ERR_CANT_CREATE;
    }

#if WIN
{
    char cwd[MPR_MAX_FNAME], pathBuf[MPR_MAX_FNAME];
    getcwd(pathBuf, sizeof(pathBuf));
    mprGetFullPathName(cwd, sizeof(cwd), pathBuf);
    mprSprintf(cmd, sizeof(cmd), "PATH=%s;../../winTools", cwd);
    rc = putenv(cmd);

    mprSprintf(cmd, sizeof(cmd), "../../winTools/bash %s/unitTestUpload.sh %s", 
        cwd, path);
}
#else
    mprSprintf(cmd, sizeof(cmd), "/bin/bash ./unitTestUpload.sh %s", path);
#endif

    run = new MprCmd();
    /* run->setCwd("."); */
    /* rc = run->start(cmd, MPR_CMD_WAIT | MPR_CMD_CHDIR); */

    rc = run->start(cmd, MPR_CMD_WAIT);
    assert(rc == 0);
    if (rc == 0) {
        rc = run->getExitCode(&status);
        assert(rc == 0);
        assert(status == 0);
    }
    gp->adjustTestCount(1, 1);

    unlink(path);
#endif
    return 0;
}


int makeTestFile(MprTestGroup *gp, char *path, int size)
{
    MprFile     file;
    char        buf[512];
    int         sofar, thisWrite, written, lineNumber, len;

    if (file.open(path, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600) < 0) {
        assert(0);
        return MPR_ERR_CANT_OPEN;
    }
    
    sofar = 0;
    lineNumber = 0;
    while (sofar < size) {
        len = mprSprintf(buf, sizeof(buf), 
            "%05d - aaaaaaaaaa bbbbbbbbbb cccccccccc dddddddddd eeeeeeeeee\r\n",
                lineNumber);
        thisWrite = min(size - sofar, len);
        written = file.write(buf, thisWrite);
        if (written != thisWrite) {
            assert(written == thisWrite);
            return MPR_ERR_CANT_OPEN;
        }
        sofar += written;
        lineNumber++;
    }
    file.close();
    return 0;
}

#else /* BLD_FEATURE_UPLOAD && BLD_FEATURE_CGI */

void mprTestUploadDummy() {}

#endif /* BLD_FEATURE_UPLOAD && BLD_FEATURE_CGI */

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

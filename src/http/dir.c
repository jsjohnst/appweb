/*
 *  dir.c -- Support authorization on a per-directory basis.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/************************************ Code *************************************/

MaDir *maCreateBareDir(MaHost *host, cchar *path)
{
    MaDir   *dir;

    mprAssert(host);
    mprAssert(path);

    dir = mprAllocObjZeroed(host, MaDir);
    if (dir == 0) {
        return 0;
    }
    dir->indexName = mprStrdup(dir, "index.html");
    dir->host = host;

#if BLD_FEATURE_AUTH
    dir->auth = maCreateAuth(dir, 0);
#endif

    if (path) {
        dir->path = mprStrdup(dir, path);
        dir->pathLen = mprStrlen(path, MPR_MAX_FNAME);
    }

    return dir;
}


MaDir *maCreateDir(MaHost *host, cchar *path, MaDir *parent)
{
    MaDir   *dir;

    mprAssert(host);
    mprAssert(path);

    dir = mprAllocObjZeroed(host, MaDir);
    if (dir == 0) {
        return 0;
    }
    
    dir->host = host;
    dir->indexName = mprStrdup(dir, parent->indexName);

    if (path == 0) {
        path = parent->path;
    }
    maSetDirPath(dir, path);

#if BLD_FEATURE_AUTH
    dir->auth = maCreateAuth(dir, parent->auth);
#endif

    return dir;
}


void maSetDirPath(MaDir *dir, cchar *fileName)
{
    char    buf[MPR_MAX_FNAME], *path;

    mprAssert(dir);
    mprAssert(fileName);

    mprFree(dir->path);
    path = mprGetAbsFilename(dir, fileName);
    mprStrcpy(buf, sizeof(buf), path);
    mprFree(path);

#if UNUSED
    /*
     *  Append a trailing "/"
     */
    len = (int) strlen(buf);
    if (buf[len - 1] != '/') {
        buf[len] = '/';
        buf[++len] = '\0';
    }
#endif

    dir->path = mprStrdup(dir, buf);
    dir->pathLen = (int) strlen(dir->path);

    /*
     *  Always strip trailing "/"
     */
    if (dir->pathLen > 0 && dir->path[dir->pathLen - 1] == '/') {
        dir->path[--dir->pathLen] = '\0';
    }
}


void maSetDirIndex(MaDir *dir, cchar *name) 
{ 
    mprAssert(dir);
    mprAssert(name && *name);

    mprFree(dir->indexName);
    dir->indexName = mprStrdup(dir, name); 
}


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

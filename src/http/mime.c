/* 
 *  mime.c
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/*********************************** Code *************************************/

int maOpenMimeTypes(MaHost *host, cchar *path)
{
    MprFile     *file;
    char        buf[80], *tok, *ext, *type;
    int         line;

    host->mimeFile = mprStrdup(host, path);

    if (host->mimeTypes == 0) {
        host->mimeTypes = mprCreateHash(host, MA_MIME_HASH_SIZE);
    }
    file = mprOpen(host, path, O_RDONLY | O_TEXT, 0);
    if (file == 0) {
        return MPR_ERR_CANT_OPEN;
    }

    line = 0;
    while (mprGets(file, buf, sizeof(buf)) != 0) {
        line++;
        if (buf[0] == '#' || isspace((int) buf[0])) {
            continue;
        }
        type = mprStrTok(buf, " \t\n\r", &tok);
        ext = mprStrTok(0, " \t\n\r", &tok);
        if (type == 0 || ext == 0) {
            mprError(host, "Bad mime spec in %s at line %d", path, line);
            continue;
        }
        while (ext) {
            maAddMimeType(host, ext, type);
            ext = mprStrTok(0, " \t\n\r", &tok);
        }
    }

    mprFree(file);

    return 0;
}


/*
 *  Add a mime type to the mime lookup table. Action Programs are added separately.
 */
MaMimeType *maAddMimeType(MaHost *host, cchar *ext, cchar *mimeType)
{
    MaMimeType  *mime;

    mime = mprAllocObjZeroed(host->mimeTypes, MaMimeType);
    if (mime == 0) {
        return 0;
    }
    mime->type = mprStrdup(host, mimeType);

    if (host->mimeTypes == 0) {
        host->mimeTypes = mprCreateHash(host, MA_MIME_HASH_SIZE);
    }
    if (*ext == '.') {
        ext++;
    }
    mprAddHash(host->mimeTypes, ext, mime);
    return mime;
}


int maSetMimeActionProgram(MaHost *host, cchar *mimeType, cchar *actionProgram)
{
    MaMimeType      *mime;
    MprHash         *hp;
    
    hp = 0;
    mime = 0;
    while ((hp = mprGetNextHash(host->mimeTypes, hp)) != 0) {
        mime = (MaMimeType*) hp->data;
        if (mime->type[0] == mimeType[0] && strcmp(mime->type, mimeType) == 0) {
            break;
        }
    }
    if (mime == 0) {
        mprError(host, "Can't find mime type %s for action program %s", mimeType, actionProgram);
        return MPR_ERR_NOT_FOUND;
    }

    mprFree(mime->actionProgram);
    mime->actionProgram = mprStrdup(host, actionProgram);

    return 0;
}


cchar *maGetMimeActionProgram(MaHost *host, cchar *mimeType)
{
    MaMimeType      *mime;

    if (mimeType == 0 || *mimeType == '\0') {
        return 0;
    }
    mime = (MaMimeType*) mprLookupHash(host->mimeTypes, mimeType);
    if (mime == 0) {
        return 0;
    }
    return mime->actionProgram;
}


cchar *maLookupMimeType(MaHost *host, cchar *ext)
{
    MaMimeType      *mime;

    if (ext == 0 || *ext == '\0') {
        return 0;
    }
    mime = (MaMimeType*) mprLookupHash(host->mimeTypes, ext);
    if (mime == 0) {
        return 0;
    }

    return mime->type;
}


void maAddStandardMimeTypes(MaHost *host)
{
    maAddMimeType(host, "ai", "application/postscript");
    maAddMimeType(host, "asc", "text/plain");
    maAddMimeType(host, "au", "audio/basic");
    maAddMimeType(host, "avi", "video/x-msvideo");
    maAddMimeType(host, "bin", "application/octet-stream");
    maAddMimeType(host, "bmp", "image/bmp");
    maAddMimeType(host, "class", "application/octet-stream");
    maAddMimeType(host, "css", "text/css");
    maAddMimeType(host, "dll", "application/octet-stream");
    maAddMimeType(host, "doc", "application/msword");
    maAddMimeType(host, "ejs", "text/html");
    maAddMimeType(host, "eps", "application/postscript");
    maAddMimeType(host, "es", "application/x-javascript");
    maAddMimeType(host, "exe", "application/octet-stream");
    maAddMimeType(host, "gif", "image/gif");
    maAddMimeType(host, "gz", "application/x-gzip");
    maAddMimeType(host, "htm", "text/html");
    maAddMimeType(host, "html", "text/html");
    maAddMimeType(host, "ico", "image/x-icon");
    maAddMimeType(host, "jar", "application/octet-stream");
    maAddMimeType(host, "jpeg", "image/jpeg");
    maAddMimeType(host, "jpg", "image/jpeg");
    maAddMimeType(host, "js", "application/x-javascript");
    maAddMimeType(host, "mp3", "audio/mpeg");
    maAddMimeType(host, "pdf", "application/pdf");
    maAddMimeType(host, "php", "text/html");
    maAddMimeType(host, "png", "image/png");
    maAddMimeType(host, "ppt", "application/vnd.ms-powerpoint");
    maAddMimeType(host, "ps", "application/postscript");
    maAddMimeType(host, "ra", "audio/x-realaudio");
    maAddMimeType(host, "ram", "audio/x-pn-realaudio");
    maAddMimeType(host, "rmm", "audio/x-pn-realaudio");
    maAddMimeType(host, "rtf", "text/rtf");
    maAddMimeType(host, "rv", "video/vnd.rn-realvideo");
    maAddMimeType(host, "so", "application/octet-stream");
    maAddMimeType(host, "swf", "application/x-shockwave-flash");
    maAddMimeType(host, "tar", "application/x-tar");
    maAddMimeType(host, "tgz", "application/x-gzip");
    maAddMimeType(host, "tiff", "image/tiff");
    maAddMimeType(host, "txt", "text/plain");
    maAddMimeType(host, "wav", "audio/x-wav");
    maAddMimeType(host, "xls", "application/vnd.ms-excel");
    maAddMimeType(host, "zip", "application/zip");
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

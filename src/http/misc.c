/*
 *  misc.c -- Bits and pieces
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/************************************ Code ************************************/
/*
 *  Build an ASCII time string.  If sbuf is NULL we use the current time, else we use the last modified time of sbuf
 */
char *maGetDateString(MprCtx ctx, MprFileInfo *sbuf)
{
    char        *dateStr;
    struct tm   tm;
    MprTime     when;

    if (sbuf == 0) {
        when = mprGetTime(ctx);
    } else {
        when = (MprTime) sbuf->mtime * MPR_TICKS_PER_SEC;
    }
    dateStr = (char*) mprAlloc(ctx, 64);
    mprGmtime(ctx, &tm, when);
    mprStrftime(ctx, dateStr, 64, "%a, %d %b %Y %T %Z", &tm);
    return dateStr;
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

/*
 *  dirHandler.c - Directory listing handler.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "http.h"

#if BLD_FEATURE_DIR
/********************************** Defines ***********************************/

/*
 *  Handler configuration
 */
typedef struct Dir {
    cchar           *defaultIcon;
    MprList         *dirList;
    bool            enabled;
    MprList         *extList;
    int             fancyIndexing;
    bool            foldersFirst;
    MprList         *ignoreList;
    cchar           *pattern;
    char            *sortField;
    int             sortOrder;              /* 1 == ascending, -1 descending */
} Dir;


/****************************** Forward Declarations **************************/

static void filterDirList(MaConn *conn, MprList *list);
static int  match(cchar *pattern, cchar *file);
static void outputFooter(MaQueue *q);
static void outputHeader(MaQueue *q, cchar *dir, int nameSize);
static void outputLine(MaQueue *q, MprDirEntry *ep, cchar *dir, int nameSize);
static void parseQuery(MaConn *conn);
static void parseWords(MprList *list, cchar *str);
static void sortList(MaConn *conn, MprList *list);

/************************************* Code ***********************************/
/*
 *  Match if the url maps to a directory.
 */
static bool matchDir(MaConn *conn, MaStage *handler, cchar *url)
{
    MaResponse      *resp;
    MprFileInfo     *info;
    Dir             *dir;

    resp = conn->response;
    info = &resp->fileInfo;
    dir = handler->stageData;
    
    if (!info->valid && mprGetFileInfo(conn, resp->filename, info) < 0) {
        return 0;
    }
    return dir->enabled && info->isDir;
}


static void runDir(MaQueue *q)
{
    MaConn          *conn;
    MaResponse      *resp;
    MaRequest       *req;
    MprList         *list;
    MprDirEntry     *dp;
    Dir             *dir;
    cchar           *filename;
    uint            nameSize;
    int             next;

    conn = q->conn;
    req = conn->request;
    resp = conn->response;
    dir = q->stage->stageData;

    filename = resp->filename;
    mprAssert(filename);

    maDontCacheResponse(conn);
    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maPutForService(q, maCreateHeaderPacket(conn), 0);

    parseQuery(conn);

    list = mprGetDirList(conn, filename, 1);
    if (list == 0) {
        maWrite(q, "<h2>Can't get file list</h2>\r\n");
        outputFooter(q);
        return;
    }

    if (dir->pattern) {
        filterDirList(conn, list);
    }

    sortList(conn, list);

    /*
     *  Get max filename
     */
    nameSize = 0;
    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        nameSize = max((int) strlen(dp->name), nameSize);
    }
    nameSize = max(nameSize, 22);

    outputHeader(q, req->url, nameSize);
    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        outputLine(q, dp, filename, nameSize);
    }
    outputFooter(q);

    maPutForService(q, maCreateEndPacket(conn), 1);

    mprFree(list);
}
 

static void parseQuery(MaConn *conn)
{
    MaRequest   *req;
    MaResponse  *resp;
    Dir         *dir;
    char        *value, *query, *next, *tok;

    req = conn->request;
    resp = conn->response;
    dir = resp->handler->stageData;
    
    query = mprStrdup(req, req->parsedUri->query);
    if (query == 0) {
        return;
    }

    tok = mprStrTok(query, ";&", &next);
    while (tok) {
        if ((value = strchr(tok, '=')) != 0) {
            *value++ = '\0';
            if (*tok == 'C') {                  /* Sort column */
                mprFree(dir->sortField);
                if (*value == 'N') {
                    dir->sortField = "Name";
                } else if (*value == 'M') {
                    dir->sortField = "Date";
                } else if (*value == 'S') {
                    dir->sortField = "Size";
                }
                dir->sortField = mprStrdup(dir, dir->sortField);

            } else if (*tok == 'O') {           /* Sort order */
                if (*value == 'A') {
                    dir->sortOrder = 1;
                } else if (*value == 'D') {
                    dir->sortOrder = -1;
                }

            } else if (*tok == 'F') {           /* Format */ 
                if (*value == '0') {
                    dir->fancyIndexing = 0;
                } else if (*value == '1') {
                    dir->fancyIndexing = 1;
                } else if (*value == '2') {
                    dir->fancyIndexing = 2;
                }

            } else if (*tok == 'P') {           /* Pattern */ 
                dir->pattern = mprStrdup(dir, value);
            }
        }
        tok = mprStrTok(next, ";&", &next);
    }
    
    mprFree(query);
}


static void sortList(MaConn *conn, MprList *list)
{
    MaRequest       *req;
    MaResponse      *resp;
    MprDirEntry     *tmp, **items;
    Dir             *dir;
    int             count, i, j, rc;

    req = conn->request;
    resp = conn->response;
    dir = resp->handler->stageData;
    
    if (dir->sortField == 0) {
        return;
    }

    count = mprGetListCount(list);
    items = (MprDirEntry**) list->items;
    if (mprStrcmpAnyCase(dir->sortField, "Name") == 0) {
        for (i = 1; i < count; i++) {
            for (j = 0; j < i; j++) {
                rc = strcmp(items[i]->name, items[j]->name);
                if (dir->foldersFirst) {
                    if (items[i]->isDir && !items[j]->isDir) {
                        rc = -dir->sortOrder;
                    } else if (items[j]->isDir && !items[i]->isDir) {
                        rc = dir->sortOrder;
                    } 
                }
                rc *= dir->sortOrder;
                if (rc < 0) {
                    tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }

    } else if (mprStrcmpAnyCase(dir->sortField, "Size") == 0) {
        for (i = 1; i < count; i++) {
            for (j = 0; j < i; j++) {
                rc = (items[i]->size < items[j]->size) ? -1 : 1;
                if (dir->foldersFirst) {
                    if (items[i]->isDir && !items[j]->isDir) {
                        rc = -dir->sortOrder;
                    } else if (items[j]->isDir && !items[i]->isDir) {
                        rc = dir->sortOrder;
                    }
                }
                rc *= dir->sortOrder;
                if (rc < 0) {
                    tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }

    } else if (mprStrcmpAnyCase(dir->sortField, "Date") == 0) {
        for (i = 1; i < count; i++) {
            for (j = 0; j < i; j++) {
                rc = (items[i]->lastModified < items[j]->lastModified) ? -1: 1;
                if (dir->foldersFirst) {
                    if (items[i]->isDir && !items[j]->isDir) {
                        rc = -dir->sortOrder;
                    } else if (items[j]->isDir && !items[i]->isDir) {
                        rc = dir->sortOrder;
                    }
                }
                rc *= dir->sortOrder;
                if (rc < 0) {
                    tmp = items[i];
                    items[i] = items[j];
                    items[j] = tmp;
                }
            }
        }
    }
}


static void outputHeader(MaQueue *q, cchar *path, int nameSize)
{
    Dir     *dir;
    char    parent[MPR_MAX_FNAME], *parentSuffix;
    int     order, reverseOrder, fancy, isRootDir;

    dir = q->stage->stageData;
    
    fancy = 1;

    maWrite(q, "<!DOCTYPE HTML PUBLIC \"-/*W3C//DTD HTML 3.2 Final//EN\">\r\n");
    maWrite(q, "<html>\r\n <head>\r\n  <title>Index of %s</title>\r\n", path);
    maWrite(q, " </head>\r\n");
    maWrite(q, "<body>\r\n");

    maWrite(q, "<h1>Index of %s</h1>\r\n", path);

    if (dir->sortOrder > 0) {
        order = 'A';
        reverseOrder = 'D';
    } else {
        order = 'D';
        reverseOrder = 'A';
    }

    if (dir->fancyIndexing == 0) {
        fancy = '0';
    } else if (dir->fancyIndexing == 1) {
        fancy = '1';
    } else if (dir->fancyIndexing == 2) {
        fancy = '2';
    }

    mprGetDirName(parent, sizeof(parent), (char*) path);

    if (parent[strlen(parent) - 1] != '/') {
        parentSuffix = "/";
    } else {
        parentSuffix = "";
    }

    isRootDir = (strcmp(path, "/") == 0);

    if (dir->fancyIndexing == 2) {
        maWrite(q, "<table><tr><th><img src=\"/icons/blank.gif\" alt=\"[ICO]\" /></th>");

        maWrite(q, "<th><a href=\"?C=N;O=%c;F=%c\">Name</a></th>", reverseOrder, fancy);
        maWrite(q, "<th><a href=\"?C=M;O=%c;F=%c\">Last modified</a></th>", reverseOrder, fancy);
        maWrite(q, "<th><a href=\"?C=S;O=%c;F=%c\">Size</a></th>", reverseOrder, fancy);
        maWrite(q, "<th><a href=\"?C=D;O=%c;F=%c\">Description</a></th>\r\n", reverseOrder, fancy);

        maWrite(q, "</tr><tr><th colspan=\"5\"><hr /></th></tr>\r\n");

        if (! isRootDir) {
            maWrite(q, "<tr><td valign=\"top\"><img src=\"/icons/back.gif\"");
            maWrite(q, "alt=\"[DIR]\" /></td><td><a href=\"%s%s\">", parent, parentSuffix);
            maWrite(q, "Parent Directory</a></td>");
            maWrite(q, "<td align=\"right\">  - </td></tr>\r\n");
        }

    } else if (dir->fancyIndexing == 1) {
        maWrite(q, "<pre><img src=\"/icons/space.gif\" alt=\"Icon\" /> ");

        maWrite(q, "<a href=\"?C=N;O=%c;F=%c\">Name</a>%*s", reverseOrder, fancy, nameSize - 3, " ");
        maWrite(q, "<a href=\"?C=M;O=%c;F=%c\">Last modified</a>       ", reverseOrder, fancy);
        maWrite(q, "<a href=\"?C=S;O=%c;F=%c\">Size</a>               ", reverseOrder, fancy);
        maWrite(q, "<a href=\"?C=D;O=%c;F=%c\">Description</a>\r\n", reverseOrder, fancy);

        maWrite(q, "<hr />");

        if (! isRootDir) {
            maWrite(q, "<img src=\"/icons/parent.gif\" alt=\"[DIR]\" />");
            maWrite(q, " <a href=\"%s%s\">Parent Directory</a>\r\n", parent, parentSuffix);
        }

    } else {
        maWrite(q, "<ul>\n");
        if (! isRootDir) {
            maWrite(q, "<li><a href=\"%s%s\"> Parent Directory</a></li>\r\n", parent, parentSuffix);
        }
    }
}


static void fmtNum(char *buf, int bufsize, int num, int divisor, char *suffix)
{
    int     whole, point;

    whole = num / divisor;
    point = (num % divisor) / (divisor / 10);

    if (point == 0) {
        mprSprintf(buf, bufsize, "%6d%s", whole, suffix);
    } else {
        mprSprintf(buf, bufsize, "%4d.%d%s", whole, point, suffix);
    }
}


static void outputLine(MaQueue *q, MprDirEntry *ep, cchar *path, int nameSize)
{
    MprFileInfo fileInfo;
    Dir         *dir;
    MprTime     when;
    MaHost      *host;
    char        newpath[MPR_MAX_FNAME], sizeBuf[16], timeBuf[48], *icon;
    struct tm   tm;
    bool        isDir;
    int         len;
    cchar       *ext, *mimeType;
    char        *dirSuffix;
    char        *months[] = { 
                    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
                };

    dir = q->stage->stageData;
    if (ep->size >= (1024*1024*1024)) {
        fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024 * 1024 * 1024, "G");

    } else if (ep->size >= (1024*1024)) {
        fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024 * 1024, "M");

    } else if (ep->size >= 1024) {
        fmtNum(sizeBuf, sizeof(sizeBuf), (int) ep->size, 1024, "K");

    } else {
        mprSprintf(sizeBuf, sizeof(sizeBuf), "%6d", (int) ep->size);
    }

    mprSprintf(newpath, sizeof(newpath), "%s/%s", path, ep->name);

    if (mprGetFileInfo(q, newpath, &fileInfo) < 0) {
        when = mprGetTime(q);
        isDir = 0;

    } else {
        isDir = fileInfo.isDir ? 1 : 0;
    }

    if (isDir) {
        icon = "folder";
        dirSuffix = "/";
    } else {
        host = q->conn->host;
        ext = mprGetExtension(ep->name);
        if ((mimeType = maLookupMimeType(host, ext)) != 0) {
            if (strcmp(ext, "es") == 0 || strcmp(ext, "ejs") == 0 || strcmp(ext, "php") == 0) {
                icon = "text";
            } else if (strstr(mimeType, "text") != 0) {
                icon = "text";
            } else {
                icon = "compressed";
            }
        } else {
            icon = "compressed";
        }
        dirSuffix = "";
    }

    when = (MprTime) fileInfo.mtime * MPR_TICKS_PER_SEC;
    mprLocaltime(q, &tm, when);

    mprSprintf(timeBuf, sizeof(timeBuf), "%02d-%3s-%4d %02d:%02d",
        tm.tm_mday, months[tm.tm_mon], tm.tm_year + 1900, tm.tm_hour,  tm.tm_min);

    len = (int) strlen(ep->name) + (int) strlen(dirSuffix);

    if (dir->fancyIndexing == 2) {

        maWrite(q, "<tr><td valign=\"top\">");
        maWrite(q, "<img src=\"/icons/%s.gif\" alt=\"[   ]\", /></td>", icon);
        maWrite(q, "<td><a href=\"%s%s\">%s%s</a></td>", ep->name, dirSuffix, ep->name, dirSuffix);
        maWrite(q, "<td>%s</td><td>%s</td></tr>\r\n", timeBuf, sizeBuf);

    } else if (dir->fancyIndexing == 1) {

        maWrite(q, "<img src=\"/icons/%s.gif\" alt=\"[   ]\", /> ", icon);
        maWrite(q, "<a href=\"%s%s\">%s%s</a>%-*s %17s %4s\r\n", ep->name, dirSuffix, ep->name, dirSuffix, nameSize - len, "", 
            timeBuf, sizeBuf);

    } else {
        maWrite(q, "<li><a href=\"%s%s\"> %s%s</a></li>\r\n", ep->name, dirSuffix, ep->name, dirSuffix);
    }
}


static void outputFooter(MaQueue *q)
{
    MaRequest   *req;
    MaConn      *conn;
    MprSocket   *sock;
    Dir         *dir;
    
    conn = q->conn;
    req = conn->request;
    dir = q->stage->stageData;
    
    if (dir->fancyIndexing == 2) {
        maWrite(q, "<tr><th colspan=\"5\"><hr /></th></tr>\r\n</table>\r\n");
        
    } else if (dir->fancyIndexing == 1) {
        maWrite(q, "<hr /></pre>\r\n");
    } else {
        maWrite(q, "</ul>\r\n");
    }
    
    sock = conn->sock->listenSock;
    maWrite(q, "<address>%s %s at %s Port %d</address>\r\n", BLD_NAME, BLD_VERSION, sock->ipAddr, sock->port);
    maWrite(q, "</body></html>\r\n");
}


static void filterDirList(MaConn *conn, MprList *list)
{
    Dir             *dir;
    MprDirEntry     *dp;
    int             next;

    dir = conn->response->handler->stageData;
    
    /*
     *  Do pattern matching. Entries that don't match, free the name to mark
     */
    for (next = 0; (dp = mprGetNextItem(list, &next)) != 0; ) {
        if (! match(dir->pattern, dp->name)) {
            mprRemoveItem(list, dp);
            mprFree(dp);
            next--;
        }
    }
}


/*
 *  Return true if the file matches the pattern. Supports '?' and '*'
 */
static int match(cchar *pattern, cchar *file)
{
    cchar   *pp, *fp;

    if (pattern == 0 || *pattern == '\0') {
        return 1;
    }
    if (file == 0 || *file == '\0') {
        return 0;
    }

    for (pp = pattern, fp = file; *pp; ) {
        if (*fp == '\0') {
            if (*pp == '*' && pp[1] == '\0') {
                /* Trailing wild card */
                return 1;
            }
            return 0;
        }

        if (*pp == '*') {
            if (match(&pp[1], &fp[0])) {
                return 1;
            }
            fp++;
            continue;

        } else if (*pp == '?' || *pp == *fp) {
            fp++;

        } else {
            return 0;
        }
        pp++;
    }
    if (*fp == '\0') {
        /* Match */
        return 1;
    }
    return 0;
}


#if BLD_FEATURE_CONFIG_PARSE
static int parseDir(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    MaStage     *handler;
    Dir         *dir;
    
    char    *name, *extensions, *option, *nextTok, *junk;

    handler = maLookupStage(http, "dirHandler");
    dir = handler->stageData;
    mprAssert(dir);
    
    if (mprStrcmpAnyCase(key, "AddIcon") == 0) {
        /*  AddIcon file ext ext ext */
        /*  Not yet supported */
        name = mprStrTok(value, " \t", &extensions);
        parseWords(dir->extList, extensions);
        return 1;

    } else if (mprStrcmpAnyCase(key, "DefaultIcon") == 0) {
        /*  DefaultIcon file */
        /*  Not yet supported */
        dir->defaultIcon = mprStrTok(value, " \t", &junk);
        return 1;

    } else if (mprStrcmpAnyCase(key, "IndexOrder") == 0) {
        /*  IndexOrder ascending|descending name|date|size */
        mprFree(dir->sortField);
        dir->sortField = 0;
        option = mprStrTok(value, " \t", &dir->sortField);
        if (mprStrcmpAnyCase(option, "ascending") == 0) {
            dir->sortOrder = 1;
        } else {
            dir->sortOrder = -1;
        }
        if (dir->sortField) {
            dir->sortField = mprStrdup(dir, dir->sortField);
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "IndexIgnore") == 0) {
        /*  IndexIgnore pat ... */
        /*  Not yet supported */
        parseWords(dir->ignoreList, value);
        return 1;

    } else if (mprStrcmpAnyCase(key, "IndexOptions") == 0) {
        /*  IndexOptions FancyIndexing|FoldersFirst ... (set of options) */
        option = mprStrTok(value, " \t", &nextTok);
        while (option) {
            if (mprStrcmpAnyCase(option, "FancyIndexing") == 0) {
                dir->fancyIndexing = 1;
            } else if (mprStrcmpAnyCase(option, "HTMLTable") == 0) {
                dir->fancyIndexing = 2;
            } else if (mprStrcmpAnyCase(option, "FoldersFirst") == 0) {
                dir->foldersFirst = 1;
            }
            option = mprStrTok(nextTok, " \t", &nextTok);
        }
        return 1;

    } else if (mprStrcmpAnyCase(key, "Options") == 0) {
        /*  Options Indexes */
        option = mprStrTok(value, " \t", &nextTok);
        while (option) {
            if (mprStrcmpAnyCase(option, "Indexes") == 0) {
                dir->enabled = 1;
            }
            option = mprStrTok(nextTok, " \t", &nextTok);
        }
        return 1;
    }
    return 0;
}
#endif


static void parseWords(MprList *list, cchar *str)
{
    char    *word, *tok, *strTok;

    mprAssert(str);
    if (str == 0 || *str == '\0') {
        return;
    }

    strTok = mprStrdup(list, str);
    word = mprStrTok(strTok, " \t\r\n", &tok);
    while (word) {
        mprAddItem(list, word);
        word = mprStrTok(0, " \t\r\n", &tok);
    }
}


/*
 *  Dynamic module initialization
 */
MprModule *maDirHandlerInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *handler;
    Dir         *dir;

    module = mprCreateModule(http, "dirHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "dirHandler", MA_STAGE_GET | MA_STAGE_HEAD);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->match = matchDir; 
    handler->run = runDir; 
    handler->parse = parseDir; 

    handler->stageData = dir = mprAllocObjZeroed(handler, Dir);
    dir->sortOrder = 1;
    http->dirHandler = handler;

    return module;
}

#else
void __mprDirHandlerDummy() {}
#endif /* BLD_FEATURE_DIR */


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

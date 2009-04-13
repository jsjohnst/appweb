/*
 *  uploadHandler.c - Form-based file upload handler. 
 *
 *  The upload handler processes post data according to RFC-1867 ("multipart/form-data" post data). 
 *  It saves the uploaded files in a configured upload directory and creates files[] variables to 
 *  describe the uploaded files.  
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "http.h"
//  TODO - temp
#include    "ejs.h"

#if BLD_FEATURE_UPLOAD
/*********************************** Locals ***********************************/

#define UPLOAD_BUF_SIZE         4096        /* Post data buffer size */

/*
 *  Configuration for the upload handler
 */
typedef struct UploadHandler {
    MaUploadCallback callback;          /* User fn to process upload data */
    void            *callbackData;      /* User fn callback data */
    MprList         *handlerHeaders;    /* List of handler headers */
    MaLocation      *location;          /* Upload URL location prefix */
    char            *uploadDir;         /* Default upload directory */
#if BLD_FEATURE_MULTITHREAD
    MprMutex        *mutex;
#endif
} UploadHandler;

#define MA_UPLOAD_REQUEST_HEADER        1   /* Request header */
#define MA_UPLOAD_BOUNDARY              2   /* Boundary divider */
#define MA_UPLOAD_CONTENT_HEADER        3   /* Content part header */
#define MA_UPLOAD_CONTENT_DATA          4   /* Content encoded data */
#define MA_UPLOAD_CONTENT_END           5   /* End of multipart message */

/*
 *  State for an upload request
 */
typedef struct Upload {
    char            *boundary;          /* Boundary signature */
    int             boundaryLen;        /* Length of boundary */
    MaUploadCallback callback;          /* User fn to process upload data */
    void            *callbackData;      /* User fn callback data */
    int             contentState;       /* Input states */
    MaUploadFile    *file;              /* Current file */
    MprHashTable    *files;             /* List of uploaded files */
    char            *fileName;          /* Current file filename */
    char            *filePath;          /* Current file incoming filename */
    //  TODO - remove this fileSize as we can use file->size
    int             fileSize;           /* Current file size */
    MaLocation      *location;          /* Upload URL location prefix */
    char            *nameField;         /* Current name keyword value */
    MprFile         *upfile;            /* Incoming file object */
    char            *uploadDir;         /* Upload dir */
} Upload;

/********************************** Forwards **********************************/

static char *findBoundary(void *buf, int bufLen, void *boundary, int boundaryLen);
static int  processContentBoundary(MaQueue *q, char *line);
static int  processContentHeader(MaQueue *q, char *line);
static int  processContentData(MaQueue *q);

/************************************* Code ***********************************/

static bool uploadMatch(MaConn *conn, MaStage *handler, cchar *uri)
{
    char    *pat;
    int     len;
    
    pat = "multipart/form-data";
    len = (int) strlen(pat);
    return mprStrcmpAnyCaseCount(conn->request->mimeType, pat, len) == 0;
}


static void uploadOpen(MaQueue *q)
{
    MaConn          *conn;
    MaResponse      *resp;
    Upload          *up;
    UploadHandler   *uph;

    conn = q->conn;
    resp = conn->response;

    uph = resp->handler->stageData;
    mprAssert(uph);

    up = mprAllocObjZeroed(resp, Upload);
    if (up == 0) {
        return;
    }
    q->queueData = up;

    up->contentState = MA_UPLOAD_REQUEST_HEADER;
    up->files = mprCreateHash(up, -1);
    
    //  TDOO - why replicate this
    up->uploadDir = mprStrdup(up, uph->uploadDir);
    up->callback = uph->callback;
    up->callbackData = uph->callbackData;
}


static void uploadClose(MaQueue *q)
{
    Upload      *up;

    up = q->queueData;
    if (up->filePath && *up->filePath) {
        mprDelete(q, up->filePath);
    }
}


static void uploadIncomingData(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaRequest   *req;
    MprBuf      *content;
    MprHash     *hp;
    Upload      *up;
    char        *line, *nextTok;
    
    mprAssert(packet);
    
    conn = q->conn;
    req = conn->request;
    up = q->queueData;
    
    if (packet->count == 0) {
        /* TODO END OF INPUT */
    }
    
    mprLog(conn, 5, "uploadIncomingData: %d bytes", packet->count);
    
    maJoinPacket(q->first, packet);

    content = q->first->content;

    line = 0;

    while (1) {
        /*
         *  Read the next line
         */
        switch (up->contentState) {
        case MA_UPLOAD_BOUNDARY:
        case MA_UPLOAD_CONTENT_HEADER:
            line = mprGetBufStart(content);
            mprStrTok(line, "\n", &nextTok);
            if (nextTok == 0) {
                return;                         /* Incomplete line */
            }
            mprAdjustBufStart(content, (int) (nextTok - line));
            mprStrTrim(line, "\r");
            break;

        case MA_UPLOAD_CONTENT_DATA:
        default:
            /* Data read below */
            ;
        }

        /*
         *  Do per state processing
         */
        switch (up->contentState) {
        case MA_UPLOAD_BOUNDARY:
            if (processContentBoundary(q, line) < 0) {
                return;
            }
            break;

        case MA_UPLOAD_CONTENT_HEADER:
            if (processContentHeader(q, line) < 0) {
                return;
            }
            break;

        case MA_UPLOAD_CONTENT_DATA:
            if (processContentData(q) < 0) {
                return;
            }
            if (mprGetBufLength(content) < up->boundaryLen) {
                /*  Incomplete boundary. Return and get more data */
                return;
            }
            break;

        case MA_UPLOAD_CONTENT_END:
            if (up->callback) {
                hp = mprGetFirstHash(up->files);
                while (hp) {
                    (*up->callback)(conn, up->callbackData, (MaUploadFile*) hp->data);
                    hp = mprGetNextHash(up->files, hp);
                }
            }
            return;
        }
    }

    if (req->remainingContent <= 0) {
        /*
         *  All done. TODO - see above.
         */
        up->contentState = MA_UPLOAD_CONTENT_END;
    }
}


/*
 *  Process the mime boundary division
 *
 *  Returns  < 0 on a request or state error
 *          == 0 if successful
 */
static int processContentBoundary(MaQueue *q, char *line)
{
    MaConn      *conn;
    Upload      *up;

    conn = q->conn;
    up = q->queueData;

    /*
     *  Expecting a multipart boundary string
     */
    if (strncmp(up->boundary, line, up->boundaryLen) != 0) {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad upload state. Incomplete boundary\n");
        return MPR_ERR_BAD_STATE;
    }
    if (line[up->boundaryLen] && strcmp(&line[up->boundaryLen], "--") == 0) {
        mprLog(conn, 7, "End of content.");
        up->contentState = MA_UPLOAD_CONTENT_END;

    } else {
        mprLog(conn, 7, "Starting new header.");
        up->contentState = MA_UPLOAD_CONTENT_HEADER;
    }
    return 0;
}


/*
 *  Expecting content headers. A blank line indicates the start of the data.
 *
 *  Returns  < 0  Request or state error
 *  Returns == 0  Successfully parsed the input line.
 */
static int processContentHeader(MaQueue *q, char *line)
{
    MaConn          *conn;
    MaUploadFile    *file;
    Upload          *up;
    char            tmpFile[MPR_MAX_FNAME];
    char            *key, *headerTok, *rest, *nextPair, *value;

    conn = q->conn;
    up = q->queueData;
    
    if (line[0] == '\0') {
        up->contentState = MA_UPLOAD_CONTENT_DATA;
        return 0;
    }
    mprLog(conn, 5, "Header line: %s", line);

    headerTok = line;
    mprStrTok(line, ": ", &rest);

    if (mprStrcmpAnyCase(headerTok, "Content-Disposition") == 0) {
        /*
         *  The content disposition header describes either a form
         *  variable or an uploaded file.
         *
         *      Content-Disposition: form-data; name="field1"
         *      >>blank line
         *      Field Data
         *      ---boundary
         *
         *      Content-Disposition: form-data; name="field1" ->
         *          filename="user.file"
         *      >>blank line
         *      File data
         *      ---boundary
         */
        key = rest;
        up->nameField = up->fileName = 0;
        while (key && mprStrTok(key, ";\r\n", &nextPair)) {

            key = mprStrTrim(key, " ");
            mprStrTok(key, "= ", &value);
            value = mprStrTrim(value, "\"");

            if (mprStrcmpAnyCase(key, "form-data") == 0) {
                /* Nothing to do */

            } else if (mprStrcmpAnyCase(key, "name") == 0) {
                mprFree(up->nameField);
                up->nameField = mprStrdup(up, value);

            } else if (mprStrcmpAnyCase(key, "filename") == 0) {
                if (up->nameField == 0) {
                    maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad upload state. Missing name field.");
                    return MPR_ERR_BAD_STATE;
                }
                mprFree(up->fileName);
                up->fileName = mprStrdup(up, value);

                /*
                 *  Create the file to hold the uploaded data
                 */
                mprMakeTempFileName(up, tmpFile, sizeof(tmpFile), up->uploadDir);
                up->filePath = mprStrdup(up, tmpFile);

                mprLog(conn, 5, "File upload of: %s stored as %s", up->fileName, up->filePath);

                up->upfile = mprOpen(up, up->filePath, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600);
                if (up->upfile == 0) {
                    maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't open upload temp file %s", up->filePath);
                    return MPR_ERR_BAD_STATE;
                }

                /*
                 *  Create the files[entry]
                 */
                //  TODO - remove nameField, fileName, filePath, size
                file = up->file = mprAllocObjZeroed(up, MaUploadFile);
                file->name = mprStrdup(file, up->nameField);
                file->clientFilename = mprStrdup(file, up->fileName);
                file->filename = mprStrdup(file, up->filePath);
                mprAddHash(up->files, up->nameField, file);
            }
            key = nextPair;
        }

    } else if (mprStrcmpAnyCase(headerTok, "Content-Type") == 0) {
        if (up->fileName) {
            mprLog(conn, 5, "Set files[%s][CONTENT_TYPE] = %s", up->file->name, rest);
            up->file->contentType = mprStrdup(up->file, rest);
        }
    }

    return 1;
}


/*
 *  Process the content data.
 *
 *  Returns < 0 on error
 *          == 0 when more data is needed
 *          == 1 when data successfully written
 */
static int processContentData(MaQueue *q)
{
    MaLimits    *limits;
    MaConn      *conn;
    Upload      *up;
    MprBuf      *content;
    char        *fileData, *bp;
    int         size, fileDataLen, rc;

    conn = q->conn;
    up = q->queueData;
    content = q->first->content;
    limits = conn->host->limits;

    size = mprGetBufLength(content);
    if (size < up->boundaryLen) {
        /*  Incomplete boundary. Return and get more data */
        return 0;
    }

    bp = findBoundary(mprGetBufStart(content), size, up->boundary, up->boundaryLen);
    if (bp == 0) {

        if (up->fileName) {
            /*
             *  No signature found yet. probably more data to come. Since we did not match the boundary above we 
             *  know there is no boundary. But there could be most of the boundary with the boundary tail in the 
             *  next block of data. Must always preserve boundary-1 bytes.
             */
            fileData = mprGetBufStart(content);
            fileDataLen = ((int) (mprGetBufEnd(content) - fileData)) - (up->boundaryLen - 1);

            if ((up->fileSize + fileDataLen) > limits->maxUploadSize) {
                maFailRequest(conn, MPR_HTTP_CODE_REQUEST_TOO_LARGE, 
                    "Uploaded file %s exceeds maximum %d\n", up->filePath, limits->maxUploadSize);
                return MPR_ERR_CANT_WRITE;
            }
            if (fileDataLen > 0) {
                mprAdjustBufStart(content, fileDataLen);

                /*
                 *  File upload. Write the file data.
                 */
                rc = mprWrite(up->upfile, fileData, fileDataLen);
                if (rc != fileDataLen) {
                    maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, 
                        "Can't write to upload temp file %s, rc %d, errno %d\n", up->filePath, rc, mprGetOsError(up));
                    return MPR_ERR_CANT_WRITE;
                }
                up->fileSize += fileDataLen;
                up->file->size = up->fileSize;
            }
        }
        return 0;       /* Get more data */
    }

    mprLog(conn, 7, "Boundary found");
    fileData = mprGetBufStart(content);
    fileDataLen = (int) (bp - fileData);

    if (fileDataLen > 0) {

        mprAdjustBufStart(content, fileDataLen);

        /*
         *  This is the CRLF before the boundary
         */
        if (fileDataLen >= 2 && fileData[fileDataLen - 2] == '\r' && fileData[fileDataLen - 1] == '\n') {
            fileDataLen -= 2;
        }

        if (up->fileName) {
            /*
             *  File upload. Write the file data
             */
            rc = mprWrite(up->upfile, fileData, fileDataLen);
            if (rc != fileDataLen) {
                maFailRequest(conn, MPR_HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't write trailing data to upload temp file %s, "
                    "rc %d, errno %d\n", 
                    up->filePath, rc, mprGetOsError());
                return MPR_ERR_CANT_WRITE;
            }
            up->fileSize += fileDataLen;
            up->file->size = up->fileSize;

            /*
             *  Now have all the data (we've seen the boundary)
             */
            mprFree(up->upfile);
            up->upfile = 0;
            //  TODO - free?
            up->fileName = 0;

        } else {
            /*
             *  Normal string form data variables
             */
            fileData[fileDataLen] = '\0';
            mprLog(conn, 5, "Set form[%s] = %s", up->nameField, fileData);
            //  TODO - not right
            maSetFormVar(conn, up->nameField, fileData);
        }
    }
    up->contentState = MA_UPLOAD_BOUNDARY;
    return 1;
}


static void uploadOutgoingService(MaQueue *q)
{
    MaRequest   *req;
    MaConn      *conn;
    Upload      *up;
    char        *mimeType, *param;

    conn = q->conn;
    req = conn->request;
    up = q->queueData;


//  TODO BUG  - what should param be set to
param = "";
    /*
     *  The string to use as the multipart boundary divider is the next token
     */
    if (mprStrcmpAnyCaseCount(param, "boundary=", 9) == 0) {
        param += 9;
    }

    up->boundaryLen = mprAllocSprintf(up, &up->boundary, MPR_MAX_STRING, "--%s", param);

//  TODO BUG
//  mprFree(mimeType);
    mimeType = 0;

    if (up->boundaryLen == 0 || *up->boundary == '\0') {
        maFailRequest(conn, MPR_HTTP_CODE_BAD_REQUEST, "Bad boundary");
        return;
    }

    maSetFormVar(conn, "UPLOAD_DIR", up->uploadDir);

    up->contentState = MA_UPLOAD_BOUNDARY;
}


/*
 *  Find the boundary signature in memory. Returns pointer to the first match.
 */ 
static char *findBoundary(void *buf, int bufLen, void *boundary, int boundaryLen)
{
    char    *cp, *endp;
    char    first;

    mprAssert(buf);
    mprAssert(boundary);
    mprAssert(boundaryLen > 0);

    first = *((char*) boundary);
    cp = (char*) buf;

    if (bufLen < boundaryLen) {
        return 0;
    }
    endp = cp + (bufLen - boundaryLen) + 1;
    while (cp < endp) {
        cp = (char *) memchr(cp, first, endp - cp);
        if (!cp) {
            return 0;
        }
        if (memcmp(cp, boundary, boundaryLen) == 0) {
            return cp;
        }
        cp++;
    }
    return 0;
}


extern void maSetUploadCallback(MaHttp *http, MaUploadCallback userCallback, void *data)
{
    MaStage             *handler;
    UploadHandler       *uph;
    
    handler = maLookupStage(http, "uploadHandler");
    if (handler) {
        uph = handler->stageData;
        uph->callback = userCallback;
        uph->callbackData = data;
    }
}


#if BLD_FEATURE_CONFIG_PARSE || 1
static int uploadParseConfig(MaHttp *http, cchar *key, char *value, MaConfigState *state)
{
    UploadHandler       *uph;
    MaHost              *host;
    char                pathBuf[MPR_MAX_FNAME], pathBuf2[MPR_MAX_FNAME];

    uph = maLookupStageData(http, "uploadHandler");

    if (mprStrcmpAnyCase(key, "FileUploadDir") == 0) {
        value = mprStrTrim(value, "\"");

        host = http->defaultServer->defaultHost;
        maReplaceReferences(http->defaultServer->defaultHost, pathBuf2, sizeof(pathBuf2), value);
        if (maMakePath(host, pathBuf, sizeof(pathBuf), pathBuf2) == 0) {
            mprError(http, "FormUploadDir path is too long");
            return -1;
        }
        mprAssert(pathBuf2[0]);

        mprFree(uph->uploadDir);
        uph->uploadDir = mprStrdup(uph, pathBuf);
        uph->location = state->location;

        mprLog(http, MPR_CONFIG, "Upload directory: %s", uph->uploadDir);
        return 1;
    }
    return 0;
}
#endif


/*
 *  Dynamic module initialization
 */
MprModule *maUploadHandlerInit(MaHttp *http, cchar *path)
{
    MprModule       *module;
    MaStage         *handler;
    UploadHandler   *uph;

    module = mprCreateModule(http, "uploadHandler", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    handler = maCreateHandler(http, "uploadHandler", MA_STAGE_POST | MA_STAGE_HEAD | MA_STAGE_FORM_VARS | MA_STAGE_VIRTUAL);
    if (handler == 0) {
        mprFree(module);
        return 0;
    }

    handler->match = uploadMatch; 
    handler->parse = uploadParseConfig; 
    handler->open = uploadOpen; 
    handler->close = uploadClose; 
    handler->incomingData = uploadIncomingData; 
    handler->outgoingService = uploadOutgoingService; 

    handler->stageData = uph = mprAllocObjZeroed(handler, UploadHandler);

#if WIN
{
    char *cp;
    uph->uploadDir = mprStrdup(uph, getenv("TEMP"));
    //  TODO - Need an MPR routine to do this
    for (cp = uph->uploadDir; *cp; cp++) {
        if (*cp == '\\') {
            *cp = '/';
        }
    }
}
#else
    uph->uploadDir = mprStrdup(uph, "/tmp");
#endif
    
#if BLD_FEATURE_MULTITHREAD && FUTURE && TODO
    upload->mutex = new MprMutex();
#endif

    return module;
}

#else
void __mprUploadHandlerDummy() { }

#endif /* BLD_FEATURE_UPLOAD */
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

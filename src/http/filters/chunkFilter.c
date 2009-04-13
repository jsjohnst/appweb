/*
 *  chunkFilter.c - Transfer chunk endociding filter.
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

#if BLD_FEATURE_CHUNK
/********************************** Forwards **********************************/

static void setChunkPrefix(MaQueue *q, MaPacket *packet);

/*********************************** Code *************************************/

static void openChunk(MaQueue *q)
{
    MaConn          *conn;
    MaResponse      *resp;

    conn = q->conn;
    resp = conn->response;

    if (resp->entityLength >= 0 && resp->chunkSize < 0) {
        maRemoveQueue(q);
    } else {
        q->packetSize = min(conn->http->limits.maxChunkSize, q->max);
    }
}


/*
 *  Apply chunks to dynamic outgoing data. 
 */
static void outgoingChunkService(MaQueue *q)
{
    MaConn      *conn;
    MaPacket    *packet;
    MaResponse  *resp;

    conn = q->conn;
    resp = conn->response;

    if (!(q->flags & MA_QUEUE_SERVICED)) {
        /*
         *  If the last packet is the end packet, we have all the data. Thus we know the actual content length 
         *  and can bypass the chunk handler.
         */
        if (q->last->flags & MA_PACKET_END) {
            if (resp->chunkSize < 0 && resp->length <= 0) {
                /*  
                 *  Set the response content length and thus disable chunking -- not needed as we know the entity length.
                 */
                resp->length = q->count;
            }

        } else {
            resp->chunkSize = min(conn->http->limits.maxChunkSize, q->max);
        }
    }

    if (resp->chunkSize <= 0) {
        maDefaultOutgoingServiceStage(q);
        
    } else {
        for (packet = maGet(q); packet; packet = maGet(q)) {
            if (!(packet->flags & MA_PACKET_HEADER)) {
                if (packet->count > resp->chunkSize) {
                    maResizePacket(q, packet, resp->chunkSize);
                }
                setChunkPrefix(q, packet);
            }
            if (!maWillNextQueueAccept(q, packet)) {
                maPutBack(q, packet);
                return;
            }
            maPutNext(q, packet);
        }
    }
}


static void setChunkPrefix(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;

    conn = q->conn;

    if (packet->prefix) {
        return;
    }
    packet->prefix = mprCreateBuf(packet, 32, 32);
    if (packet->count) {
        mprPutFmtToBuf(packet->prefix, "\r\n%x\r\n", packet->count);
    } else {
        mprPutStringToBuf(packet->prefix, "\r\n0\r\n\r\n");
    }
}


/*
 *  Loadable module initialization
 */
MprModule *maChunkFilterInit(MaHttp *http, cchar *path)
{
    MprModule   *module;
    MaStage     *filter;

    module = mprCreateModule(http, "chunkFilter", BLD_VERSION, NULL, NULL, NULL);
    if (module == 0) {
        return 0;
    }

    filter = maCreateFilter(http, "chunkFilter", MA_STAGE_ALL);
    if (filter == 0) {
        mprFree(module);
        return 0;
    }
    http->chunkFilter = filter;

    filter->open = openChunk; 
    filter->outgoingService = outgoingChunkService; 

    return module;
}


#else
void __mprChunkFilterDummy() {}
#endif /* BLD_FEATURE_CHUNK */

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

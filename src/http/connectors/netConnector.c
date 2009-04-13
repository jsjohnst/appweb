/*
 *  netConnector.c -- General network connector. 
 *
 *  The Network connector handles output data (only) from upstream handlers and filters. It uses vectored writes to
 *  aggregate output packets into fewer actual I/O requests to the O/S. 
 *
 *  Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

#if BLD_FEATURE_NET
/**************************** Forward Declarations ****************************/

static void addPacketForNet(MaQueue *q, MaPacket *packet);
static void adjustNetVec(MaQueue *q, int written);
static int  buildNetVec(MaQueue *q);

/*********************************** Code *************************************/

static void netOutgoingService(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    int         written, errCode;

    conn = q->conn;
    resp = conn->response;
    
    while (q->first || q->ioIndex) {

        if (q->ioIndex == 0 && buildNetVec(q) <= 0) {
            break;
        }

        /*
         *  Issue a single I/O request to write all the blocks in the I/O vector
         */
        mprAssert(q->ioIndex > 0);
        written = mprWriteSocketVector(conn->sock, q->iovec, q->ioIndex);
        mprLog(q, 5, "Net connector write %d", written);

        if (written < 0) {
            errCode = mprGetOsError(q);
            if (errCode == EAGAIN || errCode == EWOULDBLOCK) {
                break;
            }
#if UNUSED
            saveErrno = errno;
            written = mprWriteSocketVector(conn->sock, q->iovec, q->ioIndex);
            mprAssert(written == -1);
            if (written != -1) {
                mprLog(q, 5, "mprWriteSocketVector errno %d, then wrote %d", saveErrno, written);
            }
#endif
            if (errCode == EPIPE) {
                maFailConnection(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "Client closed connection");
            } else {
                maFailConnection(conn, MPR_HTTP_CODE_SERVICE_UNAVAILABLE, "mprVectorWriteSocket failed, error %d", errCode);
            }
            break;

        } else if (written == 0) {
            /* 
             * Socket full. Wait for an I/O event. Conn.c will setup listening for write events if the queue is non-empty
             */
            maRequestWriteBlocked(conn);
            break;

        } else if (written == q->ioCount && q->flags & MA_QUEUE_EOF) {
#if BLD_DEBUG
            /*
             *  Just to see the contents
             */
            adjustNetVec(q, written);
#else
            resp->bytesWritten += written;
#endif
            maCompleteRequest(conn);
            break;

        } else {
            adjustNetVec(q, written);
        }
    }
}


/*
 *  Build the IO vector. Return the count of bytes to be written. Return -1 for EOF.
 */
static int buildNetVec(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaPacket    *packet;

    conn = q->conn;
    resp = conn->response;

    /*
     *  Examine each packet and accumulate as many packets into the I/O vector as possible. Leave the packets on the queue 
     *  for now, they are removed after the IO is complete for the entire packet.
     */
    for (packet = q->first; packet; packet = packet->next) {
        
        /* 
         *  Must be room for 3 fragments in IO vector (could be prefix, content and suffix) 
         */
        if (q->ioIndex >= (MA_MAX_IOVEC - 2)) {
            break;
        }
        if (packet->flags & MA_PACKET_HEADER) {
            if (resp->chunkSize <= 0 && q->count > 0 && resp->length < 0) {
                /* Incase no chunking filter and we've not seen all the data yet */
                conn->keepAliveCount = 0;
            }
            maFillHeaders(conn, packet);
            q->count += packet->count;

        } else if (packet->count == 0) {
            q->flags |= MA_QUEUE_EOF;
            if (packet->prefix == NULL) {
                break;
            }
            
        } else if (resp->flags & MA_RESP_NO_BODY) {
            //  TODO - convert to maDiscardData and then remove maCleanQueue
            maCleanQueue(q);
            continue;
        }
        addPacketForNet(q, packet);
    }

    return q->ioCount;
}


/*
 *  Add one entry to the io vector
 */
static void addToNetVector(MaQueue *q, char *ptr, int bytes)
{
    mprAssert(bytes > 0);

    q->iovec[q->ioIndex].start = ptr;
    q->iovec[q->ioIndex].len = bytes;
    q->ioCount += bytes;
    q->ioIndex++;
}


/*
 *  Add a packet to the io vector. Return the number of bytes added to the vector.
 */
static void addPacketForNet(MaQueue *q, MaPacket *packet)
{
    MaResponse  *resp;
    MaConn      *conn;
    MprIOVec    *iovec;
    int         index;

    conn = q->conn;
    resp = conn->response;
    iovec = q->iovec;
    index = q->ioIndex;

    mprAssert(q->count >= 0);
    mprAssert(q->ioIndex < (MA_MAX_IOVEC - 2));

    if (packet->prefix) {
        addToNetVector(q, mprGetBufStart(packet->prefix), mprGetBufLength(packet->prefix));
    }

    if (packet->count > 0) {
        addToNetVector(q, mprGetBufStart(packet->content), mprGetBufLength(packet->content));
    }
    
#if FUTURE
    if (packet->suffix) {
        addToNetVector(q, mprGetBufStart(packet->suffix), mprGetBufLength(packet->suffix));
    }
#endif
}


/*
 *  Clear entries from the IO vector that have actually been transmitted. Support partial writes.
 */
static void adjustNetVec(MaQueue *q, int written)
{
    MprIOVec    *iovec;
    MaPacket    *packet, *last;
    MaResponse  *resp;
    int         i, j, bytes, len;

    mprAssert(q->first);

    resp = q->conn->response;
    resp->bytesWritten += written;

    /*
     *  Remove completed packets
     */
    bytes = written;
    while (bytes > 0 && (packet = q->first) != 0) {
        if (packet->prefix) {
            len = mprGetBufLength(packet->prefix);
            if (len > bytes) {
                mprAdjustBufStart(packet->prefix, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->prefix = 0;
        }

        if (packet->content) {
            len = mprGetBufLength(packet->content);
            if (len > bytes) {
                mprAdjustBufStart(packet->content, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->content = 0;
        }

#if FUTURE
        if (packet->suffix) {
            len = mprGetBufLength(packet->suffix);
            if (len > bytes) {
                mprAdjustBufStart(packet->suffix, bytes);
                bytes = 0;
                break;
            }
            bytes -= len;
            packet->suffix = 0;
        }
#endif
        last = packet;
        /*
         *  This will get the packet off the queue and will re-enable upstream disabled queues
         */
        maGet(q);
        mprFree(packet);
    }
    mprAssert(bytes == 0);

    /*
     *  Cleanup the IO vector
     */
    if (written == q->ioCount) {
        /*
         *  Entire vector written. Just reset.
         */
        q->ioIndex = 0;
        q->ioCount = 0;

    } else {
        /*
         *  Partial write of an vector entry. Need to copy down the unwritten vector entries.
         */
        q->ioCount -= written;
        mprAssert(q->ioCount >= 0);
        iovec = q->iovec;
        for (i = 0; i < q->ioIndex; i++) {
            len = (int) iovec[i].len;
            if (written < len) {
                iovec[i].start += written;
                iovec[i].len -= written;
                break;
            } else {
                written -= len;
            }
        }
        /*
         *  Compact
         */
        for (j = 0; i < q->ioIndex; j++) {
            iovec[j++] = iovec[i++];
        }
        q->ioIndex = j;
    }
}


/*
 *  Initialize the net connector
 */
int maOpenNetConnector(MaHttp *http)
{
    MaStage     *stage;

    stage = maCreateConnector(http, "netConnector", MA_STAGE_ALL);
    if (stage == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    stage->outgoingService = netOutgoingService;
    http->netConnector = stage;
    return 0;
}


#endif /* BLD_FEATURE_NET */

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

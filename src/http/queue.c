/*
 *  queue.c -- Queue support routines
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/*********************************** Code *************************************/
/*
 *  Createa a new queue for the given stage. If prev is given, then link the new queue after the previous queue.
 */

MaQueue *maCreateQueue(MaConn *conn, MaStage *stage, int direction, MaQueue *prev)
{
    MaQueue     *q;
    MaResponse  *resp;
    MaLimits    *limits;

    resp = conn->response;
    limits = &conn->http->limits;

    q = mprAllocObjZeroed(resp, MaQueue);
    if (q == 0) {
        return 0;
    }
    
    maInitQueue(conn->http, q, stage->name);
    maInitSchedulerQueue(q);

    q->conn = conn;
    q->stage = stage;
    q->close = stage->close;
    q->open = stage->open;
    q->direction = direction;

    q->max = limits->maxStageBuffer;
    q->packetSize = limits->maxStageBuffer;

    if (direction == MA_QUEUE_SEND) {
        q->put = stage->outgoingData;
        q->service = stage->outgoingService;
        
    } else {
        q->put = stage->incomingData;
        q->service = stage->incomingService;
    }
    
    if (prev) {
        maInsertQueue(prev, q);
    }
    return q;
}


/*
 *  Initialize a bare queue. Used for dummy heads.
 */
void maInitQueue(MaHttp *http, MaQueue *q, cchar *name)
{
    q->nextQ = q;
    q->prevQ = q;
    q->owner = name;

    q->max = http->limits.maxStageBuffer;
    q->low = q->max / 100 * 5;
}


/*
 *  Insert a queue after the previous element
 */
void maAppendQueue(MaQueue *head, MaQueue *q)
{
    q->nextQ = head;
    q->prevQ = head->prevQ;
    head->prevQ->nextQ = q;
    head->prevQ = q;
}


/*
 *  Insert a queue after the previous element
 */
void maInsertQueue(MaQueue *prev, MaQueue *q)
{
    q->nextQ = prev->nextQ;
    q->prevQ = prev;
    
    prev->nextQ->prevQ = q;
    prev->nextQ = q;
}


void maRemoveQueue(MaQueue *q)
{
    q->prevQ->nextQ = q->nextQ;
    q->nextQ->prevQ = q->prevQ;
    q->prevQ = q->nextQ = q;
}


MaQueue *findPreviousQueue(MaQueue *q)
{
    while (q->prevQ) {
        q = q->prevQ;
        if (q->service) {
            return q;
        }
    }
    return 0;
}


bool maIsQueueEmpty(MaQueue *q)
{
    return q->first == 0;
}


/*
 *  Get the next packet from the queue
 */
MaPacket *maGet(MaQueue *q)
{
    MaConn      *conn;
    MaQueue     *prev;
    MaPacket    *packet;

    conn = q->conn;
    while (q->first) {
        if ((packet = q->first) != 0) {
            if (packet->flags & MA_PACKET_DATA && conn->requestFailed) {
                q->first = packet->next;
                mprFree(packet);
                continue;
            }
            q->first = packet->next;
            packet->next = 0;
            q->count -= packet->count;
            mprAssert(q->count >= 0);
            if (packet == q->last) {
                q->last = 0;
                mprAssert(q->first == 0);
            }
        }
        if (q->flags & MA_QUEUE_FULL && q->count < q->low) {
            /*
             *  This queue was full and now is below the low water mark. Back-enable the previous queue.
             */
            q->flags &= ~MA_QUEUE_FULL;
            prev = findPreviousQueue(q);
            if (prev && prev->flags & MA_QUEUE_DISABLED) {
                maEnableQueue(prev);
            }
        }
        return packet;
    }
    return 0;
}


//  TODO - Make a macro
int maGetPacketLength(MaPacket *packet)
{
    return (packet->content) ? mprGetBufLength(packet->content) : 0;
}


/*
 *  Create a new packet. If size is -1, then also create a default growable buffer -- used for incoming body content. If 
 *  size > 0, then create a non-growable buffer of the requested size.
 */
MaPacket *maCreatePacket(MaConn *conn, int size)
{
    MaPacket    *packet;
    MprCtx      ctx;

#if UNUSED
    ctx = (conn->response) ? (MprCtx) conn->response : (MprCtx) conn;
#else
    ctx = conn;
#endif
    
    packet = mprAllocObjZeroed(ctx, MaPacket);
    if (packet == 0) {
        return 0;
    }
    packet->conn = conn;

    if (size != 0) {
        packet->content = mprCreateBuf(packet, size < 0 ? MA_BUFSIZE: size, -1);
        if (packet->content == 0) {
            mprFree(packet);
            return 0;
        }
    }
    return packet;
}


/*
 *  Create the response header packet
 */
MaPacket *maCreateHeaderPacket(MaConn *conn)
{
    MaPacket    *packet;

    packet = maCreatePacket(conn, MA_BUFSIZE);
    if (packet == 0) {
        return 0;
    }
    packet->flags = MA_PACKET_HEADER;
    return packet;
}


MaPacket *maCreateDataPacket(MaConn *conn, int size)
{
    MaPacket    *packet;

    packet = maCreatePacket(conn, size);
    if (packet == 0) {
        return 0;
    }
    packet->flags = MA_PACKET_DATA;
    return packet;
}


MaPacket *maCreateEndPacket(MaConn *conn)
{
    MaPacket    *packet;

    packet = maCreatePacket(conn, 0);
    if (packet == 0) {
        return 0;
    }
    packet->flags = MA_PACKET_END;
    return packet;
}


/*
 *  Put a packet on the service queue.
 */
void maPutForService(MaQueue *q, MaPacket *packet, bool serviceQ)
{
    mprAssert(packet);
    
    q->count += packet->count;
    packet->next = 0;
    
    if (q->first) {
        q->last->next = packet;
        q->last = packet;
        
    } else {
        q->first = packet;
        q->last = packet;
    }
    if (serviceQ && !(q->flags & MA_QUEUE_DISABLED))  {
        maScheduleQueue(q);
    }
}


/*
 *  Join a packet onto the service queue.
 */
void maJoinForService(MaQueue *q, MaPacket *packet, bool serviceQ)
{
    if (q->first == 0) {
        /*
         *  Just use the service queue as a holding queue while we aggregate the post data.
         */
        maPutForService(q, packet, 0);

    } else {
        /*
         *  Aggregate all data into one packet
         */
        maJoinPacket(q->first, packet);
    }
    if (serviceQ && !(q->flags & MA_QUEUE_DISABLED))  {
        maScheduleQueue(q);
    }
}


/*
 *  Pass to a queue
 */
void maPut(MaQueue *q, MaPacket *packet)
{
    mprAssert(packet);
    
    mprAssert(q->put);
    q->put(q, packet);
}


/*
 *  Pass to the next queue
 */
void maPutNext(MaQueue *q, MaPacket *packet)
{
    mprAssert(packet);
    
    mprAssert(q->nextQ->put);
    q->nextQ->put(q->nextQ, packet);
}


/*
 *  Put the packet back at the front of the queue
 */
void maPutBack(MaQueue *q, MaPacket *packet)
{
    mprAssert(packet);
    mprAssert(packet->next == 0);
    
    packet->next = q->first;

    if (q->first == 0) {
        q->last = packet;
    }
    q->first = packet;

    mprAssert(packet->count >= 0);
    q->count += packet->count;
    mprAssert(q->count >= 0);
}


/*
 *  Return true if the next queue will accept this packet. If not, then disable the queue's service procedure.
 *  This may split the packet if it exceeds the downstreams maximum packet size.
 */
bool maWillNextQueueAccept(MaQueue *q, MaPacket *packet)
{
    MaConn      *conn;
    MaQueue     *next;
    int         size;

    conn = q->conn;
    next = q->nextQ;

    size = maGetPacketLength(packet);
    if (size <= next->packetSize && (size + next->count) <= next->max) {
        return 1;
    }

    if (maResizePacket(q, packet, 0) < 0) {
        return 0;
    }
    size = maGetPacketLength(packet);
    if (size <= next->packetSize && (size + next->count) <= next->max) {
        return 1;
    }

    /*
     *  The downstream queue is full, so disable the queue and mark the downstream queue as full and service immediately. 
     */
    maDisableQueue(q);
    next->flags |= MA_QUEUE_FULL;
    return 0;
}


void maSendEndPacket(MaQueue *q)
{
    maPutNext(q, maCreateEndPacket(q->conn));
    q->flags |= MA_QUEUE_EOF;
}


void maSendPackets(MaQueue *q)
{
    MaPacket    *packet;

    for (packet = maGet(q); packet; packet = maGet(q)) {
        maPutNext(q, packet);
    }
}


void maDisableQueue(MaQueue *q)
{
    q->flags |= MA_QUEUE_DISABLED;
}


//  TODO - should this check if the queue is enabled?
void maScheduleQueue(MaQueue *q)
{
    MaQueue     *head;
    
    mprAssert(q->conn);
    head = &q->conn->serviceq;
    
    if (q->scheduleNext == q) {
        q->scheduleNext = head;
        q->schedulePrev = head->schedulePrev;
        head->schedulePrev->scheduleNext = q;
        head->schedulePrev = q;
    }
}


MaQueue *maGetNextQueueForService(MaQueue *q)
{
    MaQueue     *next;
    
    if (q->scheduleNext != q) {
        next = q->scheduleNext;
        next->schedulePrev->scheduleNext = next->scheduleNext;
        next->scheduleNext->schedulePrev = next->schedulePrev;
        next->schedulePrev = next->scheduleNext = next;
        return next;
    }
    return 0;
}


void maInitSchedulerQueue(MaQueue *q)
{
    q->scheduleNext = q;
    q->schedulePrev = q;
}


void maServiceQueue(MaQueue *q)
{
    /*
     *  Since we are servicing the queue, remove it from the service queue if it is at the front of the queue.
     */
    if (q->conn->serviceq.scheduleNext == q) {
        maGetNextQueueForService(&q->conn->serviceq);
    }
    q->service(q);
    q->flags |= MA_QUEUE_SERVICED;
}


void maEnableQueue(MaQueue *q)
{
    q->flags &= ~MA_QUEUE_DISABLED;
    maScheduleQueue(q);
}


/*
 *  Return the number of bytes the queue will accept. Always positive.
 */
int maGetQueueRoom(MaQueue *q)
{
    mprAssert(q->max > 0);
    mprAssert(q->count >= 0);
    
    if (q->count >= q->max) {
        return 0;
    }
    return q->max - q->count;
}


/*
 *  Return true if the packet is too large to be accepted by the downstream queue.
 */
bool maPacketTooBig(MaQueue *q, MaPacket *packet)
{
    int     size;
    
    size = mprGetBufLength(packet->content);
    return size > q->max || size > q->packetSize;
}


/*
 *  Split a packet if required so it fits in the downstream queue. Put back the 2nd portion of the split packet on the queue.
 *  Ensure that the packet is not larger than "size" if it is greater than zero.
 */
int maResizePacket(MaQueue *q, MaPacket *packet, int size)
{
    MaPacket    *tail;
    int         len;
    
    if (size <= 0) {
        size = MAXINT;
    }

    /*
     *  Calculate the size that will fit
     */
    len = packet->content ? mprGetBufLength(packet->content) : packet->count;
    size = min(size, len);
    size = min(size, q->nextQ->max);
    size = min(size, q->nextQ->packetSize);

    if (size == 0) {
        /* Can't fit anything downstream, no point splitting yet */
        return 0;
    }
    if (size == len) {
        return 0;
    }
    
    tail = maSplitPacket(q->conn, packet, size);
    if (tail == 0) {
        return MPR_ERR_NO_MEMORY;
    }
    maPutBack(q, tail);
    return 0;
}


/*
 *  Drain a service queue
 */
static bool drain(MaQueue *q, bool block)
{
    MaConn      *conn;

    conn = q->conn;
    
    q->pending = 0;

    /*
     *  Queue is full. Need to drain the service queue if possible.
     */
    if (block) {
        mprSetSocketBlockingMode(conn->sock, 1);
    }
    maScheduleQueue(q);
    maServiceQueues(conn);
    
    if (block) {
        mprSetSocketBlockingMode(conn->sock, 0);
    
    } else if (q->count >= q->max) {
        /*
         *  Still no room.
         */
        return 0;
    }
    return 1;
}


/*
 *  Write a block of data. This is the lowest level write routine for dynamic data. If block is true, this routine will 
 *  block until all the block is written. If block is false, then it may return without having written all the data.
 *  WARNING: This routine will block if the downstream queue is full. 
 */
int maWriteBlock(MaQueue *q, cchar *buf, int size, bool block)
{
    MaPacket    *packet;
    MaConn      *conn;
    MaResponse  *resp;
    int         bytes, written, packetSize;

    mprAssert(q->stage->flags & MA_STAGE_HANDLER);
               
    if (q->flags & MA_QUEUE_DISABLED) {
        return 0;
    }
    
    conn = q->conn;
    resp = conn->response;
    packetSize = (resp->chunkSize > 0) ? resp->chunkSize : q->max;

    for (written = 0; size > 0; ) {

        if (q->count >= q->max && !drain(q, block)) {
            break;
        }
        if (conn->abandonConnection) {
            return MPR_ERR_CANT_WRITE;
        }

        packet = q->pending;
        if (packet == 0 || mprGetBufSpace(packet->content) == 0) {
            if ((packet = maCreateDataPacket(q->conn, packetSize)) != 0) {
                q->pending = packet;
                maPutForService(q, packet, 1);
            }
        }

        bytes = mprPutBlockToBuf(packet->content, buf, size);

        buf += bytes;
        size -= bytes;
        q->count += bytes;
        packet->count += bytes;
        written += bytes;
    }
    return written;
}


int maWriteString(MaQueue *q, cchar *s)
{
    return maWriteBlock(q, s, (int) strlen(s), 1);
}


int maWrite(MaQueue *q, cchar *fmt, ...)
{
    va_list     vargs;
    char        *buf;
    int         len, rc;
    
    va_start(vargs, fmt);
    len = mprAllocVsprintf(q, &buf, -1, fmt, vargs);
    va_end(vargs);

    rc = maWriteBlock(q, buf, len, 1);
    mprFree(buf);

    return rc;
}


/*
 *  Join two packets by pulling the content from the second into the first.
 */
int maJoinPacket(MaPacket *packet, MaPacket *p)
{
    packet->count += maGetPacketLength(p);
    if (mprPutBlockToBuf(packet->content, mprGetBufStart(p->content), maGetPacketLength(p)) < 0) {
        return MPR_ERR_NO_MEMORY;
    }
    return 0;
}


/*
 *  Split a packet at a given offset and return a new packet containing the data after the offset.
 *  The suffix data migrates to the new packet. 
 */
MaPacket *maSplitPacket(MaConn *conn, MaPacket *orig, int offset)
{
    MaPacket    *packet;

    if (offset >= orig->count) {
        mprAssert(0);
        return 0;
    }

    packet = maCreatePacket(conn, 0);
    if (packet == 0) {
        return 0;
    }
    
    packet->flags = orig->flags;
    packet->count = orig->count - offset;
    orig->count = offset;

    /*
     *  Suffix migrates to the new packet
     */
    if (packet->suffix) {
        packet->suffix = orig->suffix;
        mprStealBlock(packet, packet->suffix);
        orig->suffix = 0;
    }

    if (orig->content && maGetPacketLength(orig) > 0) {
        packet->content = mprCreateBuf(packet, packet->count, -1);
        if (packet->content == 0) {
            mprFree(packet);
            return 0;
        }
        mprAdjustBufEnd(orig->content, -packet->count);
        mprPutBlockToBuf(packet->content, mprGetBufEnd(orig->content), packet->count);
    }
    
    return packet;
}


/*
 *  Remove packets from a queue which do not need to be processed.
 *  Remove data packets if no body is required (HEAD|TRACE|OPTIONS|PUT|DELETE method, not modifed content, or error)
 *  This actually removes and frees the data packets whereas maDiscardData will just flush the data packets.
 */
void maCleanQueue(MaQueue *q)
{
    MaConn      *conn;
    MaResponse  *resp;
    MaPacket    *packet, *next, *prev;

    conn = q->conn;
    resp = conn->response;

    if (!(resp->flags & MA_RESP_NO_BODY)) {
        return;
    }

    for (prev = 0, packet = q->first; packet; packet = next) {
        next = packet->next;
        if (packet->flags & (MA_PACKET_RANGE | MA_PACKET_DATA)) {
            if (prev) {
                prev->next = next;
            } else {
                q->first = next;
            }
            q->count -= packet->count;
            mprFree(packet);
            continue;
        }
        prev = packet;
    }
}


/*
 *  Remove all data from non-header packets in the queue. Don't worry about freeing. Will happen automatically at 
 *  the request end. See also maCleanQueue above.
 *  TODO - implement removePackets
 */
void maDiscardData(MaQueue *q, bool removePackets)
{
    MaPacket    *packet;

    if (q->first) {
        /*
         *  Skip the header packet
         */
        if (q->first->flags & MA_PACKET_HEADER) {
            packet = q->first->next;
        } else {
            packet = q->first;
        }

        /*
         *  Just flush each packet. Don't remove so the EOF packet is preserved
         */
        for (; packet; packet = packet->next) {
            q->conn->response->length -= packet->count;
            if (packet->content) {
                mprFlushBuf(packet->content);
            }
            packet->count = 0;
        }
    }
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

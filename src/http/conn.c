/*
 *  conn.c -- Connection module to handle individual HTTP connections.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "http.h"

/***************************** Forward Declarations ***************************/

static int  connectionDestructor(MaConn *conn);
static inline MaPacket *getPacket(MaConn *conn);
static void readEvent(MaConn *conn);
static void ioEvent(MaConn *conn, MprSocket *sock, int mask, bool isPoolThread);
static void setupConnIO(MaConn *conn);
static void setupHandler(MaConn *conn);

/*********************************** Code *************************************/
/*
 *  Create a new connection object.
 */

static MaConn *createConn(MprCtx ctx, MaHost *host, MprSocket *sock, cchar *ipAddr, int port, MaHostAddress *address)
{
    MaConn      *conn;

    conn = mprAllocObjWithDestructorZeroed(ctx, MaConn, connectionDestructor);

    if (conn == 0) {
        return 0;
    }

    if (host->keepAlive) {
        conn->keepAliveCount = host->maxKeepAlive;
    }

    conn->http = host->server->http;
    conn->sock = sock;
    mprStealBlock(conn, sock);

    conn->state = MPR_HTTP_STATE_BEGIN;
    conn->timeout = host->timeout;
    conn->remotePort = port;
    conn->remoteIpAddr = mprStrdup(conn, ipAddr);
    conn->address = address;
    conn->host = host;
    conn->originalHost = host;
    conn->input = 0;
    conn->expire = 1;

    maInitSchedulerQueue(&conn->serviceq);

    return conn;
}


/*
 *  Cleanup a connection. Invoked automatically whenever the connection is freed.
 */
static int connectionDestructor(MaConn *conn)
{
    maRemoveConn(conn->host, conn);
    mprAssert(conn->sock);

    if (conn->sock) {
        mprLog(conn, 4, "Closing connection");
        mprCloseSocket(conn->sock, MPR_SOCKET_GRACEFUL);
        mprFree(conn->sock);
    }
    return 0;
}


/*
 *  Close a connection
 */
void maCloseConn(MaConn *conn)
{
    //  TODO - locking issues
    //  TODO - need a parameter to say force a close.
    mprCloseSocket(conn->sock, MPR_SOCKET_GRACEFUL);
}


/*
 *  Reset a connection after completing a request. Connection may be kept-alive
 */
void maResetConn(MaConn *conn)
{
    conn->requestFailed = 0;
    conn->request = 0;
    conn->response = 0;
    conn->state =  MPR_HTTP_STATE_BEGIN;
    conn->flags &= ~MA_CONN_CLEAN_MASK;
    conn->expire = conn->time + conn->host->keepAliveTimeout;
}


/*
 *  Accept a new client connection. If multithreaded, this will come in on a pool thread dedicated to this connection.
 *  This is called from the listen wait handler.
 */
void maAcceptConn(MaServer *server, MprSocket *sock, cchar *ip, int port)
{
    MaHostAddress   *address;
    MaHost          *host;
    MaConn          *conn;
    MprSocket       *listenSock;
    MprHeap         *arena;

    listenSock = sock->listenSock;

    //  TODO OPT - how to compile this away
    //  TODO - want to report the current thread here (must compile single threaded)
    mprLog(server, 4, "New connection from %s:%d for %s:%d %s",
        ip, port, listenSock->ipAddr, listenSock->port, listenSock->sslSocket ? "(secure)" : "");

    /*
     *  Map the address onto a suitable host to initially serve the request initially until we can parse the Host header.
     */
    address = (MaHostAddress*) maLookupHostAddress(server, listenSock->ipAddr, listenSock->port);

    if (address == 0 || (host = mprGetFirstItem(address->vhosts)) == 0) {
        mprError(server, "No host configured for request %s:%d", listenSock->ipAddr, listenSock->port);
        mprFree(sock);
        return;
    }

    /*
     *  Create a connection memory arena. This optimizes memory allocations for this entire connection.
     *  Arenas are scalable, thread-safe virtual memory blocks that are freed in one chunk.
     */
    arena = mprAllocArena(host, "conn", 1, 0, NULL);
    if (arena == 0) {
        mprError(server, "Can't create connect arena object. Insufficient memory");
        return;
    }

    conn = createConn(arena, host, sock, ip, port, address);
    if (conn == 0) {
        mprError(server, "Can't create connect object. Insufficient memory");
        return;
    }
    conn->arena = arena;
    maAddConn(host, conn);

    if (listenSock->sslSocket) {
        /*
         *  SSL always needs the handler for non-blocking I/O (deep down in the SSL stack). Normal requests often don't
         *  require the handler as they can read the entire request in one I/O
         */
        setupHandler(conn);
    }

    ioEvent(conn, sock, MPR_READABLE, 1);

    /* WARNING the connection object may be destroyed here */
    
#if BLD_FEATURE_MULTITHREAD
    //  TODO - should this not be enabling wiat events on sock?
    //  TODO - must only do this is the queue is enabled.
    mprEnableWaitEvents(listenSock->handler, 1);
#endif
}


/*
 *  IO event handler. Called in response to accept and when single-threaded, I/O events. If multithreaded, this will be 
 *  run by a pool thread. NOTE: a request is not permanently assigned to a pool thread. Each io event may be serviced by a
 *  different pool thread.
 */
static void ioEvent(MaConn *conn, MprSocket *sock, int mask, bool isPoolThread)
{
    conn->time = mprGetTime(conn);

    if (mask & MPR_WRITEABLE) {
        maProcessWriteEvent(conn);
    }
    if (mask & MPR_READABLE) {
        readEvent(conn);
    }
    setupConnIO(conn);
}


static void setupHandler(MaConn *conn) 
{
    if (conn->sock->handler == 0) {
        mprSetSocketCallback(conn->sock, (MprSocketProc) ioEvent, conn, NULL, conn->socketEventMask, MPR_NORMAL_PRIORITY);
    } else {
        mprSetSocketEventMask(conn->sock, conn->socketEventMask);
    }
}


/*
 *  Control the connection's I/O events
 */
static void setupConnIO(MaConn *conn)
{
    conn->socketEventMask = 0;
    
    if (conn->request) {
        if (conn->response->queue[MA_QUEUE_SEND].prevQ->count > 0) {
            /*
             *  Connector has more data to send in its queue. Request a write event to service it.
             */
            conn->socketEventMask |= MPR_WRITEABLE;
        }
        if (conn->state <= MPR_HTTP_STATE_CHUNK) {
            conn->socketEventMask |= MPR_READABLE;
            
        } else if (MPR_HTTP_STATE_COMPLETE == conn->state) {
            maProcessReadEvent(conn, 0);
        }

    } else {
        if (mprGetSocketEof(conn->sock) || conn->keepAliveCount < 0) {
            /*
             *  This will close the connection and free all connection resources
             */
            mprFree(conn->arena);
            /* mprPrintAllocReport(mprGetMpr(0), "After closing connection"); */
            return;

        } else {
            conn->socketEventMask |= MPR_READABLE;
        }
    }

    /*
     *  Not end of file so enable an I/O handler to listen for either more data or another request on this connection
     */
    setupHandler(conn);

    //  TODO Locking??
    conn->expire = conn->time + conn->host->timeout;
}


/*
 *  TODO MULTITHREAD - race. This is called from cgiCallback on another thread.
 */
void maAwakenConn(MaConn *conn)
{
    if (conn->keepAliveCount <= 0) {
        maCloseConn(conn);
    } else {
        conn->socketEventMask |= MPR_READABLE;
        setupHandler(conn);
    }
}


/*
 *  Process a socket readable event and keep reading while there is data.
 */
static void readEvent(MaConn *conn)
{
    MaPacket    *packet;
    MprBuf      *content;
    int         nbytes, len;

    while (1) {

        if ((packet = getPacket(conn)) == 0) {
            return;
        }
        content = packet->content;
        len = mprGetBufSpace(content);
        if (conn->request) {
            len = min(conn->request->remainingContent, len);
            if (len == 0) {
                len = mprGetBufSpace(content);
            }
        }
        mprAssert(len > 0);

        nbytes = mprReadSocket(conn->sock, mprGetBufEnd(content), len);
    
        mprLog(conn, 5, "readEvent: read nbytes %d, bufsize %d", nbytes, len);
            
        if (nbytes < 0) {
            if (conn->request) {
                maProcessReadEvent(conn, 0);
            } else {
                conn->keepAliveCount = 0;
            }
            break;
    
        } else if (nbytes > 0) {
            mprAdjustBufEnd(content, nbytes);
            packet->count += nbytes;
            maProcessReadEvent(conn, packet);
    
        } else {
            if (mprGetSocketEof(conn->sock) && 
                    (MPR_HTTP_STATE_CONTENT <= conn->state && conn->state <= MPR_HTTP_STATE_CHUNK)) {
                maProcessReadEvent(conn, 0);
            } 
            break;
        }
    }
}


static inline MaPacket *getPacket(MaConn *conn)
{
    MaPacket        *packet;
    MprBuf          *content;

    if ((packet = conn->input) == NULL) {
        conn->input = packet = maCreatePacket(conn, MA_BUFSIZE);
    }
    if (packet) {
        content = packet->content;
        mprResetBufIfEmpty(content);
        if (mprGetBufSpace(content) < MPR_BUFSIZE) {
            mprGrowBuf(content, MPR_BUFSIZE);
        }
    }
    return packet;
}


void *maGetHandlerQueueData(MaConn *conn)
{
    MaQueue     *q;

    q = &conn->response->queue[MA_QUEUE_SEND];
    return q->nextQ->queueData;
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

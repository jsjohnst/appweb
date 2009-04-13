/*
 *	simpleHandler.c - Create a simple AppWeb request handler
 *
 *	This sample demonstrates creating a request handler to process requests.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */
 
/******************************* Includes *****************************/

#include	"appweb.h"

/********************************* Code *******************************/
/*
 *  Run the handler. This is called when all input data has been received.
 */
static void runSimple(MaQueue *q)
{
    MaConn          *conn;
    MaRequest       *req;

    conn = q->conn;
    req = conn->request;
    
    maSetHeader(conn, 0, "Last-Modified", req->host->currentDate);
    maDontCacheResponse(conn);

	/*
	 *	Create the empty header packet. This will be filled in by the downstream network connector stage.
	 */
    maPutForService(q, maCreateHeaderPacket(conn), 0);

	/*
	 *	Generate some dynamic data. If you generate a lot, this will buffer up to a configured maximum. 
	 *	If that limit is exceeded, the packet will be sent downstream and the response headers will be created.
 	 */
	maWrite(q, "Hello World");

	/*
	 *	Send an end of data packet
 	 */
    maPutForService(q, maCreateEndPacket(conn), 1);
}



static void incomingSimpleData(MaQueue *q, MaPacket *packet)
{
	/*
	 *	Do something with the incoming data in packet and then free the packet.
 	 */
	mprLog(q, 0, "Data in packet is %s", mprGetBufStart(packet->content));
    mprFree(packet);
}


int maOpenSimpleHandler(MaHttp *http)
{
    MaStage     *stage;

    stage = maCreateHandler(http, "simpleHandler", MA_STAGE_ALL | MA_STAGE_VIRTUAL);
    if (stage == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    stage->run = runSimple;
    stage->incomingData = incomingSimpleData;

    return 0;
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

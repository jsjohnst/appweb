/*
 *	simpleEjs.c - Simple use of Ejscript and a native function.
 *
 *  Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 *
 *	See the typicalEjs sample for a more extensive sample.
 */
 
/******************************* Includes *****************************/

#include	"appweb.h"
#include	"ejs.h"

/********************************* Code *******************************/
/*
 *	Native C Function that is invoked when the corresponding ejs function is called.
 *	The ejs parameter is the Ejscript interpreter handle. argv contains the arguments 
 *	passed to the ejs function defined in the web page. This function is defined via 
 *	ejsDefineGlobalFunction in main() below.
 */

static EjsVar *myEjs(Ejs *ejs, EjsVar *thisObj, int argc, EjsVar **argv)
{
	char	*s;
	int		i;

	/*
	 *	Write this data back to the client
 	 */
	ejsWrite(ejs, "<h1>Hello World</h1><p>Args: ");

	mprAssert(argv);
	for (i = 0; i < argc; ) {
		s = ejsGetString(argv[i]);
		ejsWrite(ejs, s);
		if (++i < argc) {
			ejsWrite(ejs, " ");
		}
	}
	ejsWrite(ejs, "</p>");

	/*
	 *	Functions can return a result
	 */
	return (EjsVar*) ejsCreateString(ejs, "sunny day");
}


/*
 *	Create a simple stand-alone web server
 */
int main(int argc, char **argv)
{
	MaHttp		*http;
	Ejs			*ejs;

	if ((http = maCreateWebServer("simpleEjs.conf")) == 0) {
        return MPR_ERR_CANT_CREATE;
    }

	/*
	 *	Define our ejs function in the master interpreter (shared definitions for all requests)
	 *	See the typicalEjs sample for how to create functions in classes.
	 */
	ejs = ejsGetMaster(http);
	ejsDefineGlobalFunction(ejs, "helloWorld", myEjs);
	
	if (maServiceWebServer(http) < 0) {
        return MPR_ERR_CANT_CREATE;
	}

	mprFree(http);
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

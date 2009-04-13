#!/bin/bash
#
#	connectionLoad.tst -- 

#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

HTTP_ITERATIONS=50
HTTP=${BLD_BIN_DIR}/http

#
#	Run url lists via http. This is used for load testing.
#
doTest()
{
	if [ "$VERBOSE" != 0 ]
	then
		SW=--verbose
		echo -e "http $SW --noout --host $TEST_HOST --iterations $HTTP_ITERATIONS --threads $CLIENT_THREADS /index.html"
	fi

	$HTTP $SW --noout --host $TEST_HOST --iterations $HTTP_ITERATIONS --threads $CLIENT_THREADS /index.html
	status=$?
	if [ "$VERBOSE" != 0 ]
	then
		echo "# PASSED test \"$TEST_NAME\""
	else
		echo -e "# PASSED all tests for \"$TEST_NAME\"\n"
	fi
	return $status
}

################################################################################
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#	The latest version of this code is available at http://www.embedthis.com
#
#	This software is open source; you can redistribute it and/or modify it 
#	under the terms of the GNU General Public License as published by the 
#	Free Software Foundation; either version 2 of the License, or (at your 
#	option) any later version.
#
#	This program is distributed WITHOUT ANY WARRANTY; without even the 
#	implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
#	See the GNU General Public License for more details at:
#	http://www.embedthis.com/downloads/gplLicense.html
#	
#	This General Public License does NOT permit incorporating this software 
#	into proprietary programs. If you are unable to comply with the GPL, a 
#	commercial license for this software and support services are available
#	from Embedthis Software at http://www.embedthis.com
#
################################################################################

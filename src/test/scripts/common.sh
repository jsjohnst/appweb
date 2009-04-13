#!/bin/bash
#
#	common.sh -- Common test support routines
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.

: ${BLD_TOP:=../../..}

. ${BLD_TOP}/buildConfig.sh

APPWEB=$BLD_BIN_DIR/appweb
APPWEB_PID=0
BASE_NAME=appweb
CLIENT_THREADS=1
[ $BLD_HOST_OS = WIN ] && KILL_SW=-f
HOST=127.0.0.1
HTTP=${BLD_BIN_DIR}/http
LISTEN=$HOST:4010
SERVER_THREADS=10
STARTUP=0
TEST_CLIENT=${BLD_BIN_DIR}/testAppweb
TIMEOUT=3600
TIMEOUT_PID=0
VERBOSE=0

export LD_LIBRARY_PATH=${BLD_LIB_DIR}:${BLD_LIB_DIR}/modules:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=${BLD_LIB_DIR}:${BLD_LIB_DIR}/modules:$DYLD_LIBRARY_PATH

################################################################################

cleanup()
{
	APPWEB_PID=0
	if [ ${TIMEOUT_PID} != 0 ] ; then
		/bin/kill ${KILL_SW} -15 $TIMEOUT_PID 2>/dev/null	
		TIMEOUT_PID=0
	fi
}


setTimeout()
{
	local topShell=$$
	(
		trap "exit 0" SIGTERM
		nap=5
		count=$TIMEOUT
		while [ $count -gt 0 ]
		do
			sleep $nap
			count=`expr $count - $nap`
		done

		echo
		echo "Timeout. Killing top level shell."
		/bin/kill ${KILL_SW} -2 $topShell 2>/dev/null	# SIGALRM
		sleep 1
		/bin/kill ${KILL_SW} -9 $APPWEB_PID 2>/dev/null	# SIGKILL
	) &
	TIMEOUT_PID=$!
}


#
#	Wait for Appweb to become ready to serve requests
#
waitForServer() 
{
	local status wait

	sleep 1
	wait=20
	while [ $wait -gt 0 ]
	do
		sleep 1
		$HTTP --noout --retries 0 http://$TEST_HOST/index.html ;#>/dev/null
		status=$?
		[ $status = 0 ] && break
		wait=`expr $wait - 1`
		echo "  # Waiting for $APPWEB to start ..."
	done
	if [ $status != 0 ]
	then
		echo "FAILED test $BASE_NAME" 1>&2
		echo "$APPWEB did not startup correctly." 1>&2
		exit 1
	fi
}


#
#	Kill Appweb
#
killAppweb() 
{
	local	psline

	signo=${1:=15}
	wait=${2:=1}

	[ $APPWEB_PID = 0 ] && return

	#
	#	Force kill
	#
	/bin/kill ${KILL_SW} -${signo} $APPWEB_PID 2>/dev/null
	sleep 1

	psline=`ps | awk '{print $1}' | grep $APPWEB_PID`
	while [ "$psline" != "" -a $wait -gt 0 ]
	do
		[ $wait -lt 10 ] && /bin/kill ${KILL_SW} -${signo} $psline 2>/dev/null
		sleep 1
		wait=`expr $wait - 1`
		psline=`ps | awk '{print $1}' | grep $APPWEB_PID`
		[ "$psline" != "" ] && echo "Waiting for ${APPWEB}, PID ${APPWEB_PID} to die"
		signo=9
	done

	if [ "$psline" != "" ]
	then
		echo "Error: $BASENAME: FAILED test $BASE_NAME" 1>&2
		echo "$APPWEB pid $APPWEB_PID Did not shutdown cleanly." 1>&2
		exit 1
	fi
}


getListenAddress()
{
	TEST_HOST=`grep Listen $CONF | head -1 | sed 's/Listen[ 	]*//'`
	if ! echo $TEST_HOST | grep '\.' >/dev/null
	then
		TEST_HOST="${HOST}:${TEST_HOST}"
	fi
	if ! echo $TEST_HOST | grep ':' >/dev/null
	then
		TEST_HOST="${TEST_HOST}:80"
	fi
}


#
#	Start the server
#
startServer()
{
	[ "$STARTUP" != "1" ] && return 0
	echo ""
	echo "`basename ${APPWEB}` --config $CONF --threads $SERVER_THREADS --log trace.out:0 &"
	${APPWEB} --config $CONF --threads $SERVER_THREADS --log trace.out:0 & APPWEB_PID=$!
	waitForServer
	echo "# $APPWEB ready for requests"
	echo ""
}


#
#	Instruct Appweb to exit gracefully
#
stopServer() 
{
	local status

	[ "$STARTUP" != "1" ] && return 0

	killAppweb 15 2000
}


#
#	Exit handler
#
trap "
	if [ \${TIMEOUT_PID} != 0 ] 
	then 
		/bin/kill ${KILL_SW} -15 \$TIMEOUT_PID 2>/dev/null	# SIGINT
		TIMEOUT_PID=0
	fi
	if [ \${APPWEB_PID} != 0 ] 
	then 
		killAppweb 9 2
		APPWEB_PID=0
		exit 2
	fi
" EXIT SIGINT SIGTERM

#
#	Timeout handler
#
trap "
	echo \"Error: $BASE_NAME: FAILED. Test did not complete in the allocated time.\" 2>&1
	if [ \${APPWEB_PID} != 0 ] 
	then 
		killAppweb 9 2
		exit 2
	fi
" SIGALRM SIGINT


#
#	pidof command for windows (return the windows pid, not the cygwin pid)
#
if [ $BLD_HOST_OS = WIN -o ! "`type pidof 2>/dev/null`" ]
then
	pidof() {
		[ $BLD_HOST_OS = WIN ] && winSw=-W
		name=`basename $1`
		echo `ps $winSw -e | grep $name | egrep -v 'unitTest|grep|benchmark' | awk '{ print $1 }'`
	}
fi

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


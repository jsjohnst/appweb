#!/bin/bash
#
#	appwebTest.sh -- Harness for Appweb to run various tests
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.

. scripts/common.sh

USAGE="appwebTest [--config file] [--iterations count] [--startServer] [--clientThreads N] [--serverThreads N]"

while [ "$1" != "" ]
do
	case "$1" in
	--config)
		CONF=$2
		shift ; shift ;;
	--iterations)
		ITERATIONS="$2"
		shift ; shift ;;
	--clientThreads)
		CLIENT_THREADS="$2"
		shift ; shift ;;
	--name)
		BASE_NAME="$2"
		shift ; shift ;;
	--serverThreads)
		SERVER_THREADS="$2"
		shift ; shift ;;
	--startServer)
		STARTUP=1
		shift ;;
	--timeout)
		TIMEOUT="$2"
		shift ; shift ;;
	--verbose)
		VERBOSE=1
		shift ;;
	*)
		echo "$USAGE"
		exit 255
	esac
done


executeTests()
{
	count=$ITERATIONS
	while [ $count -gt 0 ]
	do
		count=`expr $count - 1`

		for f in scripts/*.tst
		do
			TEST_NAME=$BASE_NAME.`basename ${f%.tst}`
			if [ "${f/\/*}" = "regression" ]
			then
				echo "# Running regression tests for \"$TEST_NAME\""
			else
				echo "# Running unit tests for \"$TEST_NAME\""
			fi

			. $f
			doTest
			status=$?
			if [ $status != 0 ]
			then
				# echo "ERROR: Appweb $f FAILED." 1>&2
				exit $status
			fi
		done
		echo -e "\n# Completed iteration `expr $ITERATIONS - $count` at `date +%T`\n"
	done
}


getListenAddress
startServer
setTimeout
executeTests
stopServer
cleanup

echo "# PASSED unit tests for \"$BASE_NAME\""

exit 0

################################################################################
#
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

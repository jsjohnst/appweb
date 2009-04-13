#!/bin/bash
#
#   benchmark.sh -- Benchmark the Appweb HTTP server
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.

. scripts/common.sh

USAGE="benchmark [--config file] [--iterations count] [--startServer] [--clientThreads N] [--serverThreads N] urlFile"

while [ "$1" != "" ]
do
	if [ "${1#--}" != ${1} ] ; then
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
	else
		break
	fi

	URLS=$1
done

executeTests()
{
	local results perSec

	results=/tmp/results.$$

	echo "${HTTP_CLIENT} -q -b -i $ITERATIONS -t $CLIENT_THREADS -T $CLIENT_THREADS -f $URLS -h $TEST_HOST"
	echo
	echo -e "Group\tBenchmark                     \t     Microsec\t Elapsed-sec\t Count"
	${HTTP_CLIENT} -q -b -i $ITERATIONS -t $CLIENT_THREADS -T $CLIENT_THREADS http://$TEST_HOST/bench.html
	echo -e "\n# Completed benchmark  at `date +%T`\n"

	if type ab >/dev/null 2>/dev/null ; then
		ab -k -c $CLIENT_THREADS -n $ITERATIONS http://$TEST_HOST/bench.html > $results 2>/dev/null
		if [ $? != 0 ] ; then
			echo "Benchmark failed >&2"
			exit 255
		fi
		perSec=`grep "Requests per second:" < $results | awk '{ print $4 }'`

		echo "Requests per second $perSec"
		echo
		echo "Full details:"
		cat $results
	else
		echo "WARNING: apache bench is not installed, can't find ab"
	fi
	rm -f $results
}


getListenAddress
startServer
setTimeout
executeTests
stopServer
cleanup

echo "# Benchmark complete for \"$BASE_NAME\""

exit 0

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


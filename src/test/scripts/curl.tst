#!/bin/bash
#
#	curl.tst -- Included by appwebTest.ksh
#

TEST_URLS="scripts/data/curl.urls"
TMP=/tmp/tmp$$

#
#	Run test urls using curl
#
doTest()
{
	cat $TEST_URLS | while read cmd 
	do
		uploadForm=0
        if [ "${cmd/#//}" != "${cmd}" ] ; then
            continue
        fi
        if [ "${cmd/@//}" != "${cmd}" ] ; then
			uploadForm=1
			uploadedFile="${cmd/*@/}"
			uploadedFile="${uploadedFile/\" */}"
		fi
        if [ "${cmd/upload-file//}" != "${cmd}" ] ; then
			uploadFile=1
			uploadedFile="${cmd/--upload-file /}"
			uploadedFile="${uploadedFile/ */}"
			file="${cmd/* /}"
			file="${file/?TEST_HOST/}"
			file=web${file}
		fi

		[ "$VERBOSE" = 1 ] && echo -ne "."
		[ "$VERBOSE" -ge 2 ] && echo "  curl --silent --show-error `eval echo $cmd`"

		rm -f ${TMP}.err ${TMP}.out

		sleep 1
		# echo curl --silent --show-error -o ${TMP}.trace $cmd 
		eval curl --silent --show-error -o ${TMP}.trace $cmd 2> ${TMP}.err 1> ${TMP}.out
		status=$?
        if [ $status != 0 ] ; then
			(
				echo "FAILED test $TEST_NAME"
				echo "When executing: "
				echo "curl --silent --show-error $cmd"
				echo "Status: $status"
				echo 
				[ -f ${TMP}.err ] && cat ${TMP}.err
			) 1>&2
			rm -f ${TMP}.err ${TMP}.out ${TMP}.trace
			return $status
		fi

        if [ "$uploadForm" = "1" ] ; then
			file=`cat ${TMP}.trace | grep 'Renamed File' | awk '{ print $3 }'`
			[ $BLD_BUILD_OS = WIN ] && file=c:$file
			diff $file $uploadedFile >/dev/null
			status=$?
			if [ $status != 0 ]
			then
				(
					echo "FAILED test $TEST_NAME"
					echo "Appweb file upload test failed."
					echo "File compare failed for $uploadedFile $file: $status"
					echo "When executing: "
					echo "curl --silent --show-error $cmd"
					cat ${TMP}.trace
				) 1>&2
				rm -f ${TMP}.err ${TMP}.out ${TMP}.trace $file
				return $status
			fi
			rm -f $file
		fi
        if [ "$uploadFile" = "1" ] ; then
			diff $file $uploadedFile >/dev/null
			status=$?
			if [ $status != 0 ]
			then
				(
					echo "FAILED test $TEST_NAME"
					echo "Appweb file upload test failed."
					echo "File compare failed for $uploadedFile $file: $status"
					echo "When executing: "
					echo "curl --silent --show-error $cmd"
					cat ${TMP}.trace
				) 1>&2
				rm -f ${TMP}.err ${TMP}.out ${TMP}.trace $file
				return $status
			fi
			rm -f $file
		fi
	done
	if [ "$VERBOSE" != 0 ]
	then
		echo "# PASSED test \"$TEST_NAME\""
	else
		echo -e "# PASSED all tests for \"$TEST_NAME\"\n"
	fi
	rm -f ${TMP}.err ${TMP}.out ${TMP}.trace
	return 0
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

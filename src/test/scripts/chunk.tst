#!/bin/bash
#
#	chunk.tst -- Do chunked transfers tests (use curl)
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

TEST_URLS="scripts/data/chunk.urls"
TMP=/tmp/tmp$$

#
#	Run test urls using curl
#
doTest()
{
	cat $TEST_URLS | while read cmd 
	do
		downloadedFile="${cmd/*?TEST_HOST/web}"

		[ "$VERBOSE" = 1 ] && echo -ne "."
		[ "$VERBOSE" -ge 2 ] && echo "  curl --silent --show-error `eval echo $cmd`"

		rm -f ${TMP}.err

		eval curl --silent --show-error -o ${TMP}.out $cmd 2>${TMP}.err
		status=$?
		if [ $status != 0 ]
		then
			(
				echo "FAILED test $TEST_NAME"
				echo "When executing: "
				echo "curl --silent --show-error $cmd"
				echo "Status: $status"
				echo 
				[ -f ${TMP}.err ] && cat ${TMP}.err
			) 1>&2
			rm -f ${TMP}.err ${TMP}.out
			return $status
		fi

#		diff $downloadedFile ${TMP}.out >/dev/null
#		status=$?
#		if [ $status != 0 ]
#		then
#			(
#				echo "FAILED test $TEST_NAME"
#				echo "Appweb file upload test failed."
#				echo "File compare failed for $file ${TMP}.out: $status"
#				echo "When executing: "
#				echo "curl --silent --show-error $cmd"
#				echo
#			) 1>&2
#			rm -f ${TMP}.err 
#			return $status
#		fi
#		rm -f $file

	done
	if [ "$VERBOSE" != 0 ]
	then
		echo "# PASSED test \"$TEST_NAME\""
	else
		echo -e "# PASSED all tests for \"$TEST_NAME\"\n"
	fi
	rm -f ${TMP}.err ${TMP}.out
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

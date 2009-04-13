#
#	Makefile -- Man page makefile
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include		.makedep

compileExtra: \
	appweb.txt  appwebAngel.txt  appwebMonitor.txt  http.txt  makerom.txt  httpPassword.txt \
	appweb.html appwebAngel.html appwebMonitor.html http.html makerom.html httpPassword.html \
	ejsc.txt  ejsweb.txt  ejs.txt  ejsmod.txt  ejsvm.txt \
	ejsc.html ejsweb.html ejs.html ejsmod.html ejsvm.html

%.txt: %.1
	man ./$*.1 | sed 's/.\//g' >$*.txt

%.html: %.1
	man ./$*.1 | man2html -u >$*.html

cleanExtra:
	rm -f *.txt *.html

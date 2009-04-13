#
#	Makefile for the Appweb unit tests utilities
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

EXPORT_OBJECTS	:= 0
BUILD_NATIVE	:= 0

include 		.makedep

TARGETS			+= $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE)

#
#	Targets to build
#
compileExtra: $(TARGETS) ../cgi-bin/testScript

#
#	Update the shbang path in testScript. Must be an absolute path on some systems and for valgrind.
#
../cgi-bin/testScript: $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE) Makefile $(BLD_TOP)/buildConfig.h
	echo "#!$(BLD_ABS_BIN_DIR)/cgiProgram$(BLD_EXE)" >../cgi-bin/testScript
	chmod +x ../cgi-bin/testScript

#
#	cgiProgram
#
$(BLD_BIN_DIR)/cgiProgram$(BLD_EXE): $(BLD_OBJ_DIR)/cgiProgram$(BLD_OBJ)
	@bld --omitstdlibs --exe $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE) $(BLD_OBJ_DIR)/cgiProgram$(BLD_OBJ) 
	cp $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE) '../../server/cgi-bin'
	chmod +x '../../server/cgi-bin/cgiProgram$(BLD_EXE)'
	cp $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE) '../cgi-bin'
	chmod +x '../cgi-bin/cgiProgram$(BLD_EXE)'
	cp $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE) '../web/cgiProgram.cgi'
	chmod +x '../web/cgiProgram.cgi'
	cp $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE) '../cgi-bin/nph-cgiProgram$(BLD_EXE)'
	chmod +x '../cgi-bin/nph-cgiProgram$(BLD_EXE)'
	cp $(BLD_BIN_DIR)/cgiProgram$(BLD_EXE) '../cgi-bin/cgi Program$(BLD_EXE)'
	chmod +x '../cgi-bin/cgi Program$(BLD_EXE)'
	@if [ -f '$(BLD_BIN_DIR)/cgiProgram$(BLD_EXE).manifest' ] ; \
	then \
		m=$(BLD_BIN_DIR)/cgiProgram$(BLD_EXE).manifest ; \
		cp "$$m" '../../server/cgi-bin/cgiProgram$(BLD_EXE).manifest' ; \
		cp "$$m" '../cgi-bin/cgiProgram$(BLD_EXE).manifest' ; \
		cp "$$m" '../web/cgiProgram.cgi.manifest' ; \
		cp "$$m" '../cgi-bin/nph-cgiProgram$(BLD_EXE).manifest' ; \
		cp "$$m" '../cgi-bin/cgi Program$(BLD_EXE).manifest' ; \
	fi

cleanExtra:
	@rm -f ../cgi-bin/cgiProgram$(BLD_EXE) '../cgi-bin/cgi Program$(BLD_EXE)' 
	@rm -f ../cgi-bin/cgiProgram '../cgi-bin/cgi Program'
	@rm -f ../cgi-bin/nph-cgiProgram$(BLD_EXE) ../web/cgiProgram.cgi
	@rm -f ../cgi-bin/nph-cgiProgram ../web/cgiProgram.cgi

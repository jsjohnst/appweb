#
# 	Makefile for utilities for the http web server 
#
# 	opyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 		.makedep

compileExtra: $(BLD_BIN_DIR)/httpPassword$(BLD_EXE)

$(BLD_BIN_DIR)/httpPassword$(BLD_EXE): $(OBJECTS)
	@bld --exe $(BLD_BIN_DIR)/httpPassword$(BLD_EXE) --search "$(BLD_MPR_LIBPATHS)" --libs "$(BLD_MPR_LIBS)" $(OBJECTS)

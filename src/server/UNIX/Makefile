#
# Makefile for the UNIX Appweb server components
#
# Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 		.makedep 

MAKE_IFLAGS		:= -I$(BLD_TOP)/include

TARGETS			+= $(BLD_BIN_DIR)/$(BLD_PRODUCT)Angel$(BLD_EXE)

compileExtra: 	$(TARGETS) 

$(BLD_BIN_DIR)/$(BLD_PRODUCT)Angel$(BLD_EXE): $(OBJECTS) $(BLD_MPR_MAKEDEP)
	@bld --exe $(BLD_BIN_DIR)/$(BLD_PRODUCT)Angel$(BLD_EXE) --search "$(BLD_MPR_LIBPATHS)" --libs "$(BLD_MPR_LIBS)" \
		$(OBJECTS)

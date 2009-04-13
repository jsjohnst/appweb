#
# Makefile for the Windows Appweb server components
#
# Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 	.makedep 

#
#	Extra libraries
#
ifeq	($(BLD_HOST_OS),WIN)
	SYSLIBS	+= shell32.lib
	MONITOR_RES = --resources appwebMonitor.rc
endif

#
#	Executables to build
#
TARGETS		:= $(BLD_PRODUCT)Angel$(BLD_EXE) $(BLD_PRODUCT)Monitor$(BLD_EXE)

compileExtra: $(TARGETS) 

#
#	Link the service angel
#
$(BLD_PRODUCT)Angel$(BLD_EXE): $(BLD_LIB_DIR)/libmpr$(BLD_LIB) $(OBJECTS)
	@bld --graphical --exe $(BLD_PRODUCT)Angel$(BLD_EXE) --rpath "$(BLD_PREFIX)/bin" --rpath "../bin" \
		--search "$(LIBPATH)" $(MODE) --libs "mpr" --syslibs "$(SYSLIBS)" \
		 $(BLD_OBJ_DIR)/$(BLD_PRODUCT)Angel$(BLD_OBJ) 
	cp $(BLD_PRODUCT)Angel$(BLD_EXE) $(BLD_BIN_DIR)/$(BLD_PRODUCT)Angel$(BLD_EXE) 

#
#	Link the admin monitor. 
#
$(BLD_PRODUCT)Monitor$(BLD_EXE): $(BLD_LIB_DIR)/libmpr$(BLD_LIB) $(OBJECTS)
	@bld --graphical --exe $(BLD_PRODUCT)Monitor$(BLD_EXE) --rpath "$(BLD_PREFIX)/bin" --rpath "../bin" \
		--search "$(LIBPATH)" $(MODE) --libs "mpr" $(MONITOR_RES) --syslibs "$(SYSLIBS)" \
		$(BLD_OBJ_DIR)/$(BLD_PRODUCT)Monitor$(BLD_OBJ) 
	cp $(BLD_PRODUCT)Monitor$(BLD_EXE) $(BLD_BIN_DIR)/$(BLD_PRODUCT)Monitor$(BLD_EXE) 
	cp $(BLD_PRODUCT)Monitor.ico $(BLD_BIN_DIR)

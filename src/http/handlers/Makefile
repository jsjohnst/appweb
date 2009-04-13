#
#	Makefile for the http handlers
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 		.makedep

MAKE_IFLAGS		+= $(BLD_PHP_IFLAGS) 

ifeq	($(BLD_FEATURE_PHP),1)
	IMPORTS 	+= $(BLD_OPENSSL_IMPORTS) $(BLD_PHP_IMPORTS)
	IMPORTED	+= $(patsubst %,$(BLD_LIB_DIR)/%, $(notdir $(IMPORTS)))
endif

compileExtra: 	$(IMPORTED)

$(IMPORTED): $(IMPORTS)
	getlib $^

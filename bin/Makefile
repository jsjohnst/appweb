#
#  Makefile -- Binaries Makefile
#
# Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include 	.makedep

dependExtra:
	@[ "$(BLD_CROSS)" = 1 ] && mkdir -p $(BLD_HOST_SYSTEM) ; true

cleanExtra:
	@rm -f ./$(BLD_HOST_SYSTEM)/*
	@rm -f tmp*.exe appweb* cgiProgram* charGen* httpClient* httpComp*
	@rm -f httpPassword* testAppweb* testClient* *.mod
	@rm -f *.lib *.dll ejscgi-debug

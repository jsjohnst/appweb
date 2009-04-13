#
#  Makefile -- Libraries Makefile
#
# Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

PRE_DIRS	:= modules

include 	.makedep

dependExtra:
	@mkdir -p modules
	@[ "$(BLD_CROSS)" = 1 ] && mkdir -p $(BLD_HOST_SYSTEM)/modules ; true

cleanExtra: cleanFiles

cleanFiles:
	@rm -f httpClient httpComp httpPassword charGen
	@rm -f dsiPatch* genLicense* genDepend* httpServer* dumpext*
	@rm -f testClient* testAppweb*
	@rm -f cgiProgram appweb winAppweb http testHttp staticAppweb
	@rm -f httpComp* httpPassword*
	@rm -f libcrypto.* ssleay32.* libssl.* libappweb*
	@rm -f libejs.so libmatrixssl.so libmpr.so libmprTest.so libphp.so
	@rm -f libeay32.* libejs.* libejsStatic.*
	@rm -f libmatrixssl.* libmpr.* libmprStatic.* libmprTest.*
	@rm -f libmprTestStatic.* libespStatic.* php* libec.* libphp*
	@rm -f lib*.dylib lib*.so lib*.a ejsweb.es
	@[ $(BLD_HOST_OS) = WIN -a -f bldout$(BLD_BUILD_EXE) ] && \
		rm -f bldout$(BLD_BUILD_EXE) || true
	@rm -f ./$(BLD_HOST_SYSTEM)/*$(BLD_LIB)
	@rm -f modules/*.mod modules/*.so modules/*.dylib
	@rm -rf default-web
	@rm -f mime.types appweb.conf

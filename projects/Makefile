#
#	Makefile for the projects
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

include     .makedep

MPR_HEADERS = $(shell find ../src/include -name 'mpr*.h')
EJS_HEADERS = $(shell find ../src/include -name 'ejs*.h')
EC_HEADERS  = $(shell find ../src/include -name 'ec*.h')
APP_HEADERS = ../src/include/appweb.h ../src/include/http.h
MPR_SOURCES = $(shell find ../src/mpr -name '*.c' | egrep -v 'http.c|makerom.c')
EJS_SOURCES = ../src/ejs/ejsLib.c ../src/ejs/sqlite3.c
EC_SOURCES = ../src/ejs/ecLib.c
APP_SOURCES = $(shell find ../src/http -name '*.c' | egrep -v 'XXXXX.c')

projectsExtra: appweb.vcproj appwebAngel.vcproj appwebMonitor.vcproj ejsc.vcproj ejs.vcproj ejssql.vcproj \
	ejsweb.vcproj http.vcproj httpPassword.vcproj

appweb.vcproj: dummy
	@makeVsProj --project appweb.vcproj --exe appweb.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(EJS_HEADERS) $(APP_HEADERS) $(EJS_SOURCES) $(MPR_SOURCES) $(APP_SOURCES) \
		../src/server/appweb.c ../src/ejs/ejsAppweb.c

appwebAngel.vcproj: dummy
	@makeVsProj --project appwebAngel.vcproj --graphical --exe appwebAngel.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(APP_HEADERS) $(MPR_SOURCES) ../src/server/WIN/appwebAngel.c

appwebMonitor.vcproj: dummy
	@makeVsProj --project appwebMonitor.vcproj --graphical --exe appwebMonitor.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(APP_HEADERS) $(MPR_SOURCES) \
        ../src/include/appwebMonitor.h ../src/server/WIN/appwebMonitor.c

ejs.vcproj: dummy
	@makeVsProj --project ejs.vcproj --exe ejs.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(EJS_HEADERS) $(EJS_SOURCES) $(MPR_SOURCES) ../src/ejs/ejs.c

ejsc.vcproj: dummy
	@makeVsProj --project ejsc.vcproj --exe ec.exe --search "$(BLD_EJS_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(EJS_HEADERS) $(EC_HEADERS) $(EC_SOURCES) $(EJS_SOURCES) $(MPR_SOURCES) \
		../src/ejs/ejsc.c

ejssql.vcproj: dummy
	@makeVsProj --project ejssql.vcproj --exe ejssql.exe --search "$(BLD_MPR_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(MPR_SOURCES) ../src/ejs/ejssql.c ../src/ejs/sqlite3.c

ejsweb.vcproj: dummy
	@makeVsProj --project ejsweb.vcproj --exe ejsweb.exe --search "$(BLD_MPR_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(MPR_SOURCES) ../src/ejs/ejsweb.c

http.vcproj: dummy
	@makeVsProj --project http.vcproj --exe http.exe --search "$(BLD_MPR_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(MPR_SOURCES) ../src/mpr/http.c

httpPassword.vcproj: dummy
	@makeVsProj --project httpPassword.vcproj --exe httpPassword.exe --search "$(BLD_MPR_LIBPATHS)" \
		--libs "ws2_32.lib" $(MPR_HEADERS) $(MPR_SOURCES) ../src/utils/httpPassword.c

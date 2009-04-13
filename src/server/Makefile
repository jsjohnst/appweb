#
# 	Makefile for the Appweb server 
#
# 	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

BUILD_NATIVE		:= 0
POST_DIRS			:= package

include 			.makedep 

ifeq 		($(BLD_UNIX_LIKE),1)
	PRE_DIRS		+= UNIX
else
	PRE_DIRS		+= $(BLD_HOST_OS)
	endif
ifeq	($(BLD_FEATURE_TEST),1)
	PRE_DIRS		+= test
endif

#
#	Extra libraries
#
ifeq	($(BLD_HOST_OS),WIN)
	SYSLIBS			+= shell32.lib
	RES				= --resources appweb.rc
endif
ifeq	($(BLD_FEATURE_AUTH_PAM),1)
	LIBS			+= pam
endif

LIBS				+= mpr

#
#	Executables to build
#
TARGETS				+= $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)

compileExtra: $(TARGETS) appweb.conf

#
#	Create the appweb program
#
$(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE): $(BLD_LIB_DIR)/libappweb$(BLD_LIB) $(OBJECTS) $(LIBDEP)
	@bld --exe $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE) --search "$(BLD_APPWEB_LIBPATHS)" \
	--libs "$(BLD_APPWEB_LIBS) $(LIBS)" --syslibs "$(SYSLIBS)" $(RES) $(OBJECTS)

#
#	Patch the appweb.conf configuration file
#
appweb.conf: masterConf/appweb.conf masterConf/*/*.conf ../../buildConfig.make Makefile
	@echo
	cp -r masterConf/* conf 
	mv conf/appweb.conf appweb.conf
	BLD_PREFIX=. BLD_WEB_PREFIX=./web BLD_MOD_PREFIX=$(BLD_MOD_DIR) BLD_DOC_PREFIX=.. BLD_SERVER=`hostname` \
	BLD_LOG_PREFIX=./logs BLD_HTTP_PORT=$(BLD_HTTP_PORT) BLD_SSL_PORT=$(BLD_SSL_PORT) patchAppwebConf appweb.conf \
	conf/hosts/ssl-default.conf conf/log.conf conf/misc.conf conf/extras/doc.conf

run:
	$(call setlibpath) ; $(BLD_BIN_DIR)/$(BLD_PRODUCT) --log stdout:2 --config appweb.conf

x:
	$(call setlibpath) ; $(BLD_BIN_DIR)/$(BLD_PRODUCT)

cleanExtra:
	@rm -f $(TARGETS) $(BLD_PRODUCT)$(BLD_EXE)
	@rm -f trace.txt access.log error.log leak.log
	@rm -f *.obj *.pdb
	@rm -f logs/*.log logs/*.old
	@rm -fr appweb.conf target conf/appweb.conf conf/hosts conf/modules
	@rm -f web/*.mod web/*.es

rom:
	find web -print | \
	egrep -v 'CVS|\.svn|\.cvsignore|\.ignore|\.tmp|\.sav' >rom.files
	echo -e ".\nappweb.conf\nmime.types\nusers.db\ngroups.db" >>rom.files
	$(call setlibpath) ; $(BLD_BIN_DIR)/httpComp rom.files >romFiles.cpp

#
#	VxWorks Quick deploy target to copy files to ./target
#
deploy:
	@echo "Copy files to $(BLD_TOP)/appweb/target"
	@[ ! -d target ] && mkdir target ; true
	@[ ! -d target/bin ] && mkdir target/bin ; true
	@[ ! -d target/put ] && mkdir target/put ; true
	@cp appweb.conf mime.types server.crt server.key.pem target
	@cp ../bin/appweb.out target
	@[ ! -d target/web ] && mkdir target/web ; true
	@[ ! -d target/web/icons ] && mkdir target/web/icons ; true
	@cp web/*.ejs web/*.html web/*.ico web/*.js target/web
	@cp web/icons/* target/web/icons
	@echo "Sizes for appweb.out"
	@size$(BLD_BUILD_CPU_VX) target/appweb.out 
	cp C:/tornado/target/proj/appweb/default/vxWorks target
	echo OLD cp C:/tornado/target/proj/mob/default/vxWorks target
	@mkdir -p target/sub1/sub2
	@date >target/sub1/a.txt
	@date >target/sub1/sub2/b.txt

vxSetup:
	mkdir -p target
	subst v: c:\usr\svn\appweb\trunk\appweb\target

#	subst /d v:

size:
	@echo -e "    #\n    #	Size of $(BLD_PRODUCT)\n    #"
	@size $(BLD_PRODUCT) | sed -e 's/^/    # /'


curl:
	curl --trace trace.tmp --dump-header header.tmp https://127.0.0.1:4443/index.html

val:
	(valgrind -v --tool=memcheck --suppressions=d.supp --leak-check=yes --num-callers=12 --leak-resolution=high \
		--show-reachable=yes --track-fds=yes ../bin/appweb -r . -f appweb.conf -l stdout:1  2>&1) | tee val.out

mem:
	@echo -e "\nImage and code sizes:"
	@ls -hs  $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)
	@size $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)
	pid=`ps -ef | grep 'appweb.*conf' | grep -v grep | awk '{ print \$$2 }'` ; \
	echo -e "\nMemory usage for $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE), pid $$pid:" ; \
	cat /proc/$$pid/status | egrep 'VmSize|VmPeak|VmRSS|VmData|VmStk|VmExe'

dependExtra:
	@mkdir -p logs

do:
	cd ../..  ; make
	$(call setlibpath) ; $(BLD_BIN_DIR)/$(BLD_PRODUCT) --log stdout:4 --config appweb.conf

#
# Makefile for the Appweb unit tests
#
# Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

BUILD_NATIVE	:= 0
TEST_SYSTEM		:= 127.0.0.1:7010
POST_DIRS		:= utils

include 		.makedep

#
#	Compute the targets to build
#
TARGETS	:= $(BLD_BIN_DIR)/testAppweb$(BLD_EXE) 

#
#	Targets to build
#
compileExtra: $(TARGETS) appweb.conf

#
#	Build testAppweb
#
$(BLD_BIN_DIR)/testAppweb$(BLD_EXE): $(OBJECTS) $(BLD_LIB_DIR)/libmpr$(BLD_LIB)
	@bld --exe $(BLD_BIN_DIR)/testAppweb$(BLD_EXE) --libs "$(BLD_MPR_LIBS) $(BLD_SSL_LIBS)" $(OBJECTS)

run:
	$(call setlibpath) ; $(BLD_BIN_DIR)/appweb --log stdout:1 --config appweb.conf

users.db:
	@echo "# Make authorization databases"
	@rm -f users.db groups.db
	@set -e ; \
	httpPassword -c -p pass1 users.db "Acme Inc" joshua ; \
	httpPassword -p pass2 users.db "Acme Inc" mary ; \
	httpPassword -p pass3 users.db "Coyote Corp" peter ; \
	httpPassword -p pass4 users.db "Coyote Corp" julie ; \
	echo "1: 1023: coyoteUsers: peter julie" >groups.db ; \
	echo "1: FFFF: acmeUsers: joshua mary" >>groups.db ; \
	echo "1: FFFF: acmeExec: mary" >>groups.db

appweb.conf: Makefile $(BLD_TOP)/buildConfig.make $(BLD_BIN_DIR)/appweb$(BLD_EXE)
ifeq ($(BLD_OS),WIN)
	BLD_PREFIX=. BLD_WEB_PREFIX=./web BLD_MOD_PREFIX=$(BLD_MOD_DIR) BLD_SERVER=`hostname` BLD_LOG_PREFIX=logs \
	   	BLD_DOC_PREFIX=../.. BLD_HTTP_PORT=4010 BLD_SSL_PORT=4011 \
		patchAppwebConf appweb.conf conf/hosts/ssl-default.conf conf/log.conf conf/misc.conf conf/extras/doc.conf 
else
	BLD_PREFIX=. BLD_WEB_PREFIX=./web BLD_MOD_PREFIX=$(BLD_MOD_DIR) BLD_SERVER=`hostname` BLD_LOG_PREFIX=logs \
	   	BLD_DOC_PREFIX=../.. BLD_HTTP_PORT=4010 BLD_SSL_PORT=4011 \
		patchAppwebConf appweb.conf conf/hosts/ssl-default.conf conf/log.conf conf/misc.conf conf/extras/doc.conf 
endif

ifeq	($(BLD_FEATURE_TEST),1)
#
#	Build farm test
#
buildFarmTestExtra: users.db appweb.conf
	@echo -e "# Running unit tests for $(BLD_PRODUCT).server"
	@$(call setlibpath) ; scripts/appwebTest.sh --startServer --config appweb.conf --iterations 1 \
		--name $(BLD_PRODUCT).server --clientThreads 1 --serverThreads 10 --timeout 300

#
#	Unit tests
#
testExtra: users.db
	@echo -e "# Running unit tests for $(BLD_PRODUCT).server"
	@$(call setlibpath) ; scripts/appwebTest.sh --startServer --config appweb.conf --iterations 1 \
		--name $(BLD_PRODUCT).server --clientThreads 1 --serverThreads 10 --timeout 300

leakTestExtra:
	@echo -e "# Running leak tests for $(BLD_PRODUCT).server "
	if type valgrind >/dev/null 2>/dev/null ; then \
		leakTest --name $(BLD_PRODUCT).server \
		--cmd "testAppweb -r . -f appweb.conf" --timeout 300 \
		--stressCmd "testAppweb -v -h 127.0.0.1:4010" --stressDelay 25 \
		--timeoutCmd "httpAppweb http://127.0.0.1:4010/admin/exit" ; \
	fi

benchExtra: 
	@echo -e "#\n# Benchmarking $(BLD_NAME)\n#\n"
	if [ `hostname` != "redhat9" ] ; then \
	./benchmark.sh --startServer --config appweb.conf --iterations 10000 \
		--name $(BLD_PRODUCT).server --clientThreads 8 --serverThreads 10 \
		--timeout 300 bench.urls ; \
	fi 

apacheBench:
	ab -k -c 10 -n 1000 http://127.0.0.1:7010/bench.html 

#
#	Load (longevity) tests
#
loadTestExtra: users.db
	@echo -e "# Running load tests for $(BLD_PRODUCT).server"
	@scripts/appwebTest.sh --startServer --config appweb.conf --iterations 100 \
		--name $(BLD_PRODUCT).server --clientThreads 2 --serverThreads 40 --timeout 36000
endif

#
#	Manual testing. Start a test server. Use this with testAppweb1, 2 and testOtherClient
#
testServer: 
	$(call setlibpath) ; $(BLD_BIN_DIR)/appweb --log appweb.out:1.999

testClient:
	@$(call setlibpath) ; \
	iterations=100 ; \
	while : ; \
	do \
		echo $(BLD_BIN_DIR)/testAppweb -i $$iterations -h $$TEST_SYSTEM -t 1 -v ; \
		$(BLD_BIN_DIR)/testAppweb -i $$iterations -h $$TEST_SYSTEM -t 1 -v ; \
	done

#
#	Start a client test with multiple threads. This runs all unit tests.
#
testClient1:
	@set -e ; \
	iterations=1000 ; \
	$(call setlibpath) ; \
	while : ; \
	do \
		set -x ; \
		$(BLD_BIN_DIR)/testAppweb -h $(TEST_SYSTEM) -i $$iterations -t 1 -v ; \
		$(BLD_BIN_DIR)/testAppweb -h $(TEST_SYSTEM) -i $$iterations -t 2 -v ; \
		$(BLD_BIN_DIR)/testAppweb -h $(TEST_SYSTEM) -i $$iterations -t 4 -v ; \
		$(BLD_BIN_DIR)/testAppweb -h $(TEST_SYSTEM) -i $$iterations -t 8 -v ; \
		set +x ; \
	done

#
#	Test using the http program
#
testClient2:
	@set -e ; \
	$(call setlibpath) ; \
	while : ; \
	do \
		iterations=4000 ; \
		set -x ; \
		$(BLD_BIN_DIR)/http -v -T 1 -t 1 -i $$iterations -n -h $(TEST_SYSTEM) /index.html ; \
		$(BLD_BIN_DIR)/http -v -T 2 -t 2 -i $$iterations -n -h $(TEST_SYSTEM) /index.html ; \
		$(BLD_BIN_DIR)/http -v -T 4 -t 4 -i $$iterations -n -h $(TEST_SYSTEM) /index.html ; \
		$(BLD_BIN_DIR)/http -v -T 8 -t 8 -i $$iterations -n -h $(TEST_SYSTEM) /index.html ; \
		set +x ; \
	done

testOtherClient:
	@echo -n "Enter IP:PORT : " ; read SYSTEM ; \
	set -e ; \
	$(call setlibpath) ; \
	iterations=2000 ; \
	while : ; \
	do \
		$(BLD_BIN_DIR)/testAppweb -i $$iterations -h $$SYSTEM -t 2 -v ; \
	done

mem:
	@echo -e "\nImage and code sizes:"
	@ls -hs  $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)
	@size $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE)
	pid=`ps -ef | grep 'appweb.*conf' | grep -v grep | awk '{ print \$$2 }'` ; \
	echo -e "\nMemory usage for $(BLD_BIN_DIR)/$(BLD_PRODUCT)$(BLD_EXE), pid $$pid:" ; \
	cat /proc/$$pid/status | egrep 'VmSize|VmPeak|VmRSS|VmData|VmStk|VmExe'

cleanExtra:
	@rm -f *.obj *.pdb
	@rm -f access.log error.log leak.log
	@rm -f $(MODULES) $(BLD_BIN_DIR)/testAppweb$(BLD_EXE) testAppweb$(BLD_EXE)
	@rm -f *.o *.lo *.obj *.out */*.mod */*/*.mod

dependExtra:
	@cd scripts >/dev/null ; [ "`echo *.sh *.tst`" != "*.sh *.tst" ] && chmod +x *.sh *.tst; true
	@chmod -R 777 vhostWeb web 

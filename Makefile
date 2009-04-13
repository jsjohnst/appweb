# 
#	Makefile -- Top level Makefile for Appweb
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#
#
#	Standard Make targets supported are:
#	
#		make 						# Does a "make compile"
#		make clean					# Removes generated objects
#		make compile				# Compiles the source
#		make depend					# Generates the make dependencies
#		make test 					# Runs unit tests
#		make leakTest 				# Runs memory leak tests
#		make loadTest 				# Runs load tests
#		make benchmark 				# Runs benchmarks
#		make package				# Creates an installable package
#		make startService			# Starts an installed instance of the app
#		make stopService			# Stops an installed instance of the app
#
#	Additional targets for this makefile:
#
#		make newbuild				# Increment the build number and rebuild
#
#	Installation targets. Use "make DESTDIR=myDir" to do a custom local
#		install:
#
#		make install				# Call install-binary
#		make install-release		# Install release files (README.TXT etc)
#		make install-binary			# Install binary files
#		make install-dev			# Install development libraries and headers
#		make install-doc			# Install documentation
#		make install-samples		# Install samples
#		make install-src			# Install source code
#		make install-all			# Install everything except source code
#		make install-package		# Install a previously built installation package
#
#	To remove, use make uninstall-ITEM, where ITEM is a component above.
#

include		build/make/Makefile.top

ifeq ($(shell [ -f buildConfig.make ] && echo found),found)
uclinuxCheck: 
	@if [ "$(UCLINUX_BUILD_USER)" = 1 ] ; \
	then \
		rm -f build/buildConfig.defaults ; \
		BLD_PRODUCT=appweb ; \
		echo "    ln -s $$BLD_PRODUCT/uclinux.defaults build/buildConfig.defaults" ;\
		ln -s $$BLD_PRODUCT/uclinux.defaults build/buildConfig.defaults ; \
		if [ ! -f build/buildConfig.cache -o ../../.config -nt buildConfig.make ] ; \
		then \
			if [ "$$CONFIG_USER_APPWEB_DYNAMIC" = "y" ] ; \
			then \
				SW="$$SW" ; \
			else \
				SW="$$SW --static" ; \
			fi ; \
			if [ "$$CONFIG_USER_APPWEB_MULTITHREAD" = "y" ] ; \
			then \
				SW="$$SW --enable-multi-thread" ; \
			else SW="$$SW --disable-multi-thread" ; \
			fi ; \
			if [ "$$CONFIG_USER_APPWEB_SSL" = "y" ] ; \
			then \
				SW="$$SW --with-openssl=../../lib/libssl" ; \
			elif [ "$$CONFIG_USER_APPWEB_MATRIXSSL" = "y" ] ; \
			then \
				SW="$$SW --with-matrixssl=../../lib/matrixssl" ; \
			else SW="$$SW --without-ssl" ; \
			fi ; \
			if [ "$$CONFIG_USER_APPWEB_CGI" = "y" ] ; \
			then SW="$$SW --enable-cgi" ; \
			else SW="$$SW --disable-cgi" ; \
			fi ; \
			echo "    ./configure $$SW " ; \
			./configure $$SW; \
			echo "  $(MAKE) -S $(MAKEF)" ; \
			$(MAKE) -S $(MAKEF) ; \
		fi ; \
	else \
		echo "Must run configure first" ; \
		exit 2 ; \
	fi
endif

startService:

stopService:

diff import sync:
	@if [ ! -x $(BLD_TOOLS_DIR)/edep$(BLD_BUILD_EXE) -a "$(BUILDING_CROSS)" != 1 ] ; then \
		$(MAKE) -S --no-print-directory _RECURSIVE_=1 -C $(BLD_TOP)/build/src compile ; \
	fi
	@rm -fr doc/ejs
	@mkdir -p doc/ejs
	@cd ../ejs/doc ; find . -type f | \
		egrep -v '/xml/|/html/|/dsi/|.makedep|.DS_Store|.pptx|\/Archive' | cpio -pdum ../../appweb/doc/ejs
	@chmod +w doc/man/*.1
	@cp doc/ejs/man/*.1 doc/man
	@chmod -R +w doc/ejs
	@rm -f doc/api/mprBare.html doc/api/ejsBare.html
	@import.ksh --$@ --src ../build --dir . ../build/build/export/export.gen
	@import.ksh --$@ --src ../mpr --dir . ../mpr/build/export/export.gen
	@import.ksh --$@ --src ../mpr --dir ./src/include --strip ./src/all/ ../mpr/build/export/export.h
	@import.ksh --$@ --src ../mpr --dir ./src/mpr --strip ./src/all/ ../mpr/build/export/export.c
	@import.ksh --$@ --src ../ejs --dir . ../ejs/build/export/export.gen
	@import.ksh --$@ --src ../ejs --dir ./src/include --strip ./src/all/ ../ejs/build/export/export.h
	@import.ksh --$@ --src ../ejs --dir ./src/ejs --strip ./src/all/ ../ejs/build/export/export.c
	@cp doc/ejs/man/*.1 doc/man
	@echo

#
#	Convenient configure targets
#
config:
	./configure --static --without-ssl --without-php
	@make depend clean >/dev/null

config64:
	./configure --host x86_64-apple-darwin --without-ssl --without-php

release:
	./configure --defaults=release

vx5:
	unset WIND_HOME WIND_BASE ; \
	SEARCH_PATH=/tornado ./configure --host=i386-wrs-vxworks --enable-all --without-ssl --without-php

vx6:
	unset WIND_HOME WIND_BASE ; \
	./configure --host=pentium-wrs-vxworks --enable-all --without-ssl --without-php


#
#	Samples for cross compilation
#	
vx5env:
	ARCH=386 ; \
	WIND_HOME=c:/tornado ; \
	WIND_BASE=$$WIND_HOME ; \
	WIND_GNU_PATH=$$WIND_BASE/host ; \
	AR=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ar$${ARCH}.exe \
	CC=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/cc$${ARCH}.exe \
	LD=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ld$${ARCH}.exe \
	NM=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/nm$${ARCH}.exe \
	RANLIB=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ranlib$${ARCH}.exe \
	STRIP=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/strip$${ARCH}.exe \
	IFLAGS="-I$$WIND_BASE/target/h -I$$WIND_BASE/target/h/wrn/coreip" \
	SEARCH_PATH=/tornado ./configure --host=i386-wrs-vxworks --enable-all --without-ssl --without-php

vx6env:
	ARCH=pentium ; \
	WIND_HOME=c:/WindRiver ; \
	VXWORKS=vxworks-6.3 ; \
	WIND_BASE=$$WIND_HOME/$$VXWORKS ; \
	PLATFORM=i586-wrs-vxworks ; \
	WIND_GNU_PATH=$$WIND_HOME/gnu/3.4.4-vxworks-6.3 ; \
	AR=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ar$${ARCH}.exe \
	CC=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/cc$${ARCH}.exe \
	LD=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/cc$${ARCH}.exe \
	NM=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/$${PLATFORM}/bin/nm.exe \
	RANLIB=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/$${PLATFORM}/bin/ranlib.exe \
	STRIP=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/$${PLATFORM}/bin/strip.exe \
	CFLAGS="-I$$WIND_BASE/target/h -I$$WIND_BASE/target/h/wrn/coreip" \
	./configure --host=i386-wrs-vxworks --enable-all --without-ssl --without-php

vxenv:
	wrenv -p vxworks-6.3 -f sh -o print_env

cygwin:
	./configure --cygwin --defaults=dev --enable-test --disable-samples \
	--without-php --without-matrixssl --without-openssl --without-gacompat

freebsd:
	./configure --defaults=dev --enable-test --disable-samples --without-php --without-matrixssl \
		--without-openssl --without-gacompat --disable-multithread

php:
	DIR=/Users/mob/svn/packages/php/php-5.2.0 ; \
	CC=arm-linux-gcc AR=arm-linux-ar LD=arm-linux-ld
	./configure -with-php=builtin --with-php-dir=$$DIR \
		--with-php-iflags="-I$$DIR -I$$DIR/main -I$$DIR/Zend -I$$DIR/TSRM" \
		--with-php-libpath="$$DIR/libs" --with-php-libs="libphp crypt resolv db z"

#
#	Using ubuntu packages: uclibc-toolchain, libuclibc-dev
#	Use dpkg -L package to see installed files. Installed under /usr/i386-uclibc-linux
#
uclibc:
	PREFIX=i386-uclibc-linux; \
	DIR=/usr/i386-uclibc-linux/bin ; \
	AR=$${DIR}/$${PREFIX}-ar \
	CC=$${DIR}/$${PREFIX}-gcc \
	LD=$${DIR}/$${PREFIX}-gcc \
	NM=$${DIR}/$${PREFIX}-nm \
	RANLIB=$${DIR}/$${PREFIX}-ranlib \
	STRIP=$${DIR}/$${PREFIX}-strip \
	CFLAGS="-fno-stack-protector" \
	CXXFLAGS="-fno-rtti -fno-exceptions" \
	BUILD_CC=/usr/bin/cc \
	BUILD_LD=/usr/bin/cc \
	./configure --host=i386-pc-linux --enable-all

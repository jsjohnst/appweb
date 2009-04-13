#
#	RPM spec file for the Embedthis Appweb HTTP web server
#
Summary: !!BLD_NAME!! -- Embeddable HTTP Web Server
Name: !!BLD_PRODUCT!!
Version: !!BLD_VERSION!!
Release: !!BLD_NUMBER!!
License: Dual GPL/commercial
Group: Applications/Internet
URL: http://www.appwebserver.org
Distribution: Embedthis
Vendor: Embedthis Software
BuildRoot: !!ROOT_DIR!!/rpmDist
AutoReqProv: no

%description
Embedthis Appweb is an embeddable HTTP Web Server

%prep

%build
    mkdir -p !!ROOT_DIR!!/rpmDist
    for dir in BIN DEV SRC ; do
        cp -r !!ROOT_DIR!!/${dir}/*  !!ROOT_DIR!!/rpmDist
    done

%install

%clean

%files -f binFiles.txt

%post
if [ -x /usr/bin/chcon ] ; then 
	sestatus | grep enabled >/dev/null 2>&1
	if [ $? = 0 ] ; then
		for f in !!BLD_LIB_PREFIX!!/*.so !!BLD_MOD_PREFIX!!/*.so ; do
			chcon /usr/bin/chcon -t texrel_shlib_t $f
		done
	fi
fi

ldconfig -n !!BLD_LIB_PREFIX!!

%preun

%postun

#
#	Dev package
#
%package dev
Summary: Embedthis Appweb -- Development headers for !!BLD_NAME!!
Group: Applications/Internet
Prefix: !!BLD_INC_PREFIX!!

%description dev
Development headers for !!BLD_NAME!!

%files dev -f devFiles.txt

#
#	Source package
#
%package src
Summary: Embedthis Appweb -- Source code for !!BLD_NAME!!
Group: Applications/Internet
Prefix: !!BLD_SRC_PREFIX!!

%description src
Source code for !!BLD_NAME!!

%files src -f srcFiles.txt

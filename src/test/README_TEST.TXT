README.TXT for the Embedthis HTTP Server.
--------------------------------------------------------------------------------

The http/test directory contains an extensive suite of unit tests to verify
Appweb. A test server called "testAppweb" is built and is exercised by running 
unit tests from the "testClient" client program.

Use the "make run" target to run the http server. 
Use the "make test" target to run the unit tests.

The key directories and files are:

Directory             Purpose
-----------------------------
certs                - empty
conf                 - Http.conf files for various build configurations
sslWeb               - Web content to test SSL access
vhostWeb             - Web content to test virtual hosts
web                  - Web content for unit tests

File                 Purpose
----------------------------
ca.crt               - Certificate for test Certificate Authority
ca.key               - Test Certificate Authority key
ca.key.pem           - Decrypted Test Certificate Authority key
cgiPostBuild.bat     - Visual Studio post build step for cgiProgram.c
cgiProgram.c         - Test CGI program
error.log            - Error log output created by http
files                - List of object files to link with testClient
groups.db            - Test group authorization database
http.binary          - Test Appweb configuration file for binary distributions
http.conf            - Test Appweb configuration file for http
httpStaticLink.c     - File to link against builtin modules
leak.urls            - File of URLs used by httpClient to test for mem leaks
httpMain.c           - Main program for http
make.dep             - Makefile dependencies
Makefile             - Makefile for the test area with test targets
mime.types           - Mime types for http
openssl.ca           - Certificate Authority definition file 
openssl.conf         - OpenSSL configuration file
php.ini              - PHP initialization file for use by the phpHandler
post.urls            - File of URLs used by httpClient to test for mem leaks
response.client      - Test response file for building client certificates
response.server      - Test response file for building server certificates
rom.files            - List of files to be compiled into ROM
romFiles.c           - Generated ROM of test web content by httpComp
server.crt           - Test server certificate
server.key           - Encrypted test server key
server.key.pem       - Decrypted test server key 
sourceFiles          - List of files for Visual Studio to build testClient
testAuth.c           - Unit tests for the authorization feature
testCgi.c            - Unit tests for the 
testCompat.h         - Compat unit test header
testCompatModule.c   - Unit tests for GoAhead WebServer compatibility module
testEgi.c            - Unit tests for the 
testEgi.h            - EGI unit test header
testEgiModule.c      - Loadable module for unit tests for EGI
testEsp.c            - Unit tests for Embedded Server Pages (ESP)
testEsp.h            - ESP unit test header
testEspModule.c      - Unit tests for the 
testFiles            - List of object files for Visual Studio to build
testGet.c            - Unit tests for the GET HTTP method
testClient.c         - Unit tests for appweb
testClient.h         - Unit test control header
testHttpLib.c        - Unit tests for the HTTP/Appweb library
testModule.c         - Test dynamically loadable module
testPhp.c            - Unit tests for the PHP handler
testPost.c           - Unit tests for the POST HTTP method
testVhost.c          - Unit tests for virtual hosts 
unitTest.sh          - Script to invoke the unit tests
users.db             - Test user authentication database

--------------------------------------------------------------------------------

Copyright (c) 2003-2009 Embedthis Software, LLC. All Rights Reserved.
Embedthis and Appweb are trademarks of Embedthis Software, LLC. Other 
brands and their products are trademarks of their respective holders.

See LICENSE.TXT for software license details.

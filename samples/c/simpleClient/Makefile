#
#	Makefile for the simpleClient sample
#
#	Copyright (c) Embedthis Software LLC, 2003-2009. All Rights Reserved.
#

NAME			:= simpleClient

include 		.makedep

TARGETS			+= $(NAME)$(BLD_EXE)

compileExtra: 	$(TARGETS)

$(NAME)$(BLD_EXE): $(OBJECTS)
	@bld --exe $(NAME)$(BLD_EXE) --libs "$(BLD_MPR_LIBS)" $(OBJECTS)

run:
	./$(NAME)$(BLD_EXE)

projects:
	@makeVsProj --project client.vcproj --exe client.exe --libs "$(BLD_MPR_LIBS)" *.c

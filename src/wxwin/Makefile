#
# File:		Makefile for samples
# Author:	Robert Roebling
# Created:	1999
# Updated:	
# Copyright:	(c) 1998 Robert Roebling
#
# This makefile requires a Unix version of wxWindows
# to be installed on your system. This is most often
# done typing "make install" when using the complete
# sources of wxWindows or by installing the two
# RPM packages wxGTK.XXX.rpm and wxGTK-devel.XXX.rpm
# under Linux.
#

CXX_FLAGS = -Wall

CXX = $(shell wx-config --cxx) $(CXX_FLAGS)

PROGRAM = CPEditorTest

SOURCES = CPEditorTest.cpp CPImageCtrl.cpp CPEditor.cpp TestPanel.cpp ImgScrollWindow.cpp
HEADERS = CPEditorTest.h CPImageCtrl.h CPEditor.h TestPanel.h ImgScrollWindow.h

OBJECTS = $(PROGRAM).o CPImageCtrl.o TestPanel.o CPEditor.o ImgScrollWindow.o

# implementation

.SUFFIXES:	.o .cpp

.cpp.o :
	$(CXX) -c `wx-config --cxxflags` -o $@ $<

all:    $(PROGRAM) Makefile.depend

$(PROGRAM):	$(OBJECTS)
	$(CXX) -o $(PROGRAM) $(OBJECTS) `wx-config --libs`

clean:
	rm -f *.o $(PROGRAM)

DEPEND_FILES = $(HEADERS) $(SOURCES)
Makefile.depend: $(DEPEND_FILES) Makefile
	makedepend -DDEPEND -f- $(DEPEND_FILES) > Makefile.depend 2>/dev/null

-include Makefile.depend

# This Makefile can be used to build the library on its own.
# It is integrated in the hugin cmake build system, so for use with
# hugin, you shouldn't need that.

LIBOBJ = Comment.o MakefileItem.o  Variable.o VariableRef.o Makefile.o \
Rule.o VariableDef.o AutoVariable.o Conditional.o
CXXFLAGS += -fPIC -Wall -g -O2
LIB = libmakefilelib.so
BOOSTLIBS = -lboost_thread-mt -lboost_regex-mt -lboost_filesystem-mt -lboost_iostreams-mt
LDLIBS = $(BOOSTLIBS) 
LDFLAGS += -Wl,-rpath,.

TESTERS = test_simple test_filenames test_makefilelib example

ifdef WIN32
TESTUTIL = test_util_win32.o
else
TESTUTIL = test_util.o
endif

all: $(TESTERS)

$(LIB): $(LIBOBJ)
	$(CXX) $(CFLAGS) -shared -Wl,-soname,$(LIB) -o $(LIB) $(LIBOBJ) $(BOOSTLIBS)
	

$(TESTERS): $(LIB) $(TESTUTIL)

clean:
	-$(RM) $(LIBOBJ) $(LIB) $(TESTUTIL) $(TESTERS)

.PHONY: all clean testers
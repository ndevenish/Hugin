# ========================================================================
#
#  Makefile.template
#
#  Author: Patric Jensfelt
#
#  Changes by Pablo d'Angelo
#   - removed CORBA stuff
#   - added LIBS, APPS and TESTS expansion (written by Boris Kluge)
#   - make static instead of shared libraries
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public
#  License along with this library; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#
# ========================================================================

# ====================================================================
# relative path to the src directory
# ====================================================================

PREFIX = .

# ====================================================================
# include config
# ====================================================================
include $(PREFIX)/makefiles/config.mk

# ====================================================================
# If you wish to add an extra directory to this makefile
# this is the only place you have to change. Just add it
# to the SUBDIRS variable. NOTE: they will be make:ed in
# the order they are written here.
# ====================================================================

SUBDIRS = Panorama hugin

# ====================================================================
# Files that should be considered while creating the dependencies
# ====================================================================

DEPEND_FILES = 

# ====================================================================
# Some of the object in this directory you might want to put in a
# library to make them easily available from other locations. The
# library name should be without the leading lib and trailing
# .a. Those are added automatically.
# ====================================================================

LIBS = 

# Defaults
LIB_CFLAGS = $(CFLAGS)
LIB_LFLAGS = $(LFLAGS)

# Library 1
LIB_1_BIN = somelib
LIB_1_OBJ = somelib.o other.o
LIB_1_CFLAGS =  $(LIB_CFLAGS)
LIB_1_LFLAGS =  $(LIB_LFLAGS)


# ====================================================================
# The source files from which the applications should be built. The
# applications will be given the same name as the source file but
# without the suffix such as .cc
# ====================================================================


APPS = 

# Defaults

APP_CFLAGS =   $(CFLAGS)
APP_LFLAGS =   $(LFLAGS)

# App 1

APP_1_BIN = bla
APP_1_OBJ = bla.o bla2.o
APP_1_CFLAGS = $(TEST_CFLAGS)
APP_1_LFLAGS = $(TEST_LFLAGS)

# App 2

APP_1_BIN = alb
APP_1_OBJ = alb.o alb2.o
APP_1_CFLAGS = $(TEST_CFLAGS)
APP_1_LFLAGS = $(TEST_LFLAGS)


# ====================================================================
# Some applications are only for testing. To make these compile you
# must explicitly type "make tests"
# ====================================================================


TESTS =

# Defaults

TEST_CFLAGS =   $(CFLAGS)
TEST_LFLAGS =   $(LFLAGS)

# Test 1

TEST_1_BIN =
TEST_1_OBJ =
TEST_1_CFLAGS = $(TEST_CFLAGS)
TEST_1_LFLAGS = $(TEST_LFLAGS)


# ====================================================================
# Do not edit anything below this
# ====================================================================

include $(PREFIX)/makefiles/rules.mk
-include $(DEPEND)

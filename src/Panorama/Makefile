# ========================================================================
#
#  Makefile.template
#
#  Author: Patric Jensfelt
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

PREFIX = ..

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

SUBDIRS =

# ====================================================================
# Files that should be considered while creating the dependencies
# ====================================================================

DEPEND_FILES = *.cpp

# ====================================================================
# Some of the object in this directory you might want to put in a
# library to make them easily available from other locations. The
# library name should be without the leading lib and trailing
# .a. Those are added automatically.
# ====================================================================

LIBS = 1

# Defaults
LIB_CFLAGS = $(CFLAGS)
LIB_LFLAGS = $(LFLAGS)

# Library 1
LIB_1_BIN = panorama
LIB_1_OBJ = Panorama.o PanoramaMemento.o PanoImage.o utils.o Process.o
LIB_1_CFLAGS =  $(LIB_CFLAGS)
LIB_1_LFLAGS =  $(LIB_LFLAGS)


APP = 1 2 3 4 5 6

# Defaults

APP_CFLAGS =   $(CFLAGS)
APP_LFLAGS =   $(LFLAGS)

# Test 1

APP_1_BIN =
APP_1_OBJ =
APP_1_CFLAGS = $(TEST_CFLAGS)
APP_1_LFLAGS = $(TEST_LFLAGS)


# ====================================================================
# Some applications are only for testing. To make these compile you
# must explicitly type "make tests"
# ====================================================================


TEST =

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

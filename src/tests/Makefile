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
# special compiler and linker flags for this directory
# ====================================================================

CFLAGS += $(shell wx-config --cxxflags)
LFLAGS += $(shell wx-config --libs) -lwximageproc -lpanorama -ljhead -lboost_unit_test_framework

# ====================================================================
# Some applications are only for testing. To make these compile you
# must explicitly type "make tests"
# ====================================================================


TESTS = 1 2

# Defaults

TEST_CFLAGS =   $(CFLAGS)
TEST_LFLAGS =   $(LFLAGS)

# Test 1

TEST_1_BIN = imageproc
TEST_1_OBJ = imageproc.o
TEST_1_CFLAGS = $(TEST_CFLAGS)
TEST_1_LFLAGS = $(TEST_LFLAGS)

TEST_2_BIN = panorama
TEST_2_OBJ = panorama.o
TEST_2_CFLAGS = $(TEST_CFLAGS)
TEST_2_LFLAGS = $(TEST_LFLAGS)


# ====================================================================
# Do not edit anything below this
# ====================================================================

include $(PREFIX)/makefiles/rules.mk
-include $(DEPEND)


run_tests: panorama
	./panorama
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

DEPEND_FILES =

# ====================================================================
# special compiler and linker flags for this directory
# ====================================================================

CFLAGS += $(VIGRA_CFLAGS)
LFLAGS += -lpanorama -ljhead -lcommon -lpano12 $(VIGRA_LFLAGS)

# ====================================================================
# The applications that should be build in this directory.
#
# ====================================================================

ifeq ($(HAS_FULL_VIGRA),1)

APPS = 4

# App 1

APP_1_BIN = impex_mask
APP_1_OBJ = impex_mask.o
APP_1_LFLAGS = -lvigra_impex2 $(LFLAGS)

APP_3_BIN = 8to16bit
APP_3_OBJ = convert_types.o
APP_3_LFLAGS = -l vigra_impex2 $(LFLAGS)

APP_2_BIN = phase_correlate
APP_2_OBJ = phasecorrelation.o

APP_4_BIN = test_multilayer
APP_4_OBJ = test_multilayer.o

APP_2_LFLAGS = $(LFLAGS) -lfftw
endif


# ====================================================================
# Some applications are only for testing. To make these compile you
# must explicitly type "make tests"
# ====================================================================


TESTS = 6 7

# Defaults

TEST_CFLAGS =   $(CFLAGS)
TEST_LFLAGS =   $(LFLAGS)  -lboost_unit_test_framework

# Test 1

TEST_1_BIN = imageproc
TEST_1_OBJ = imageproc.o
TEST_1_CFLAGS = $(TEST_CFLAGS)
TEST_1_LFLAGS = $(TEST_LFLAGS)

TEST_2_BIN = panorama
TEST_2_OBJ = panorama.o
TEST_2_CFLAGS = $(TEST_CFLAGS)
TEST_2_LFLAGS = $(TEST_LFLAGS)

TEST_3_BIN = panotools
TEST_3_OBJ = panotools.o
TEST_3_CFLAGS = $(TEST_CFLAGS)
TEST_3_LFLAGS = $(TEST_LFLAGS)

TEST_4_BIN = transforms
TEST_4_OBJ = transforms.o
TEST_4_CFLAGS = $(TEST_CFLAGS)
TEST_4_LFLAGS = $(TEST_LFLAGS)

TEST_5_BIN = interpolator
TEST_5_OBJ = interpolator.o
TEST_5_CFLAGS = $(TEST_CFLAGS)
TEST_5_LFLAGS = $(TEST_LFLAGS)

TEST_6_BIN = vigra_ext
TEST_6_OBJ = vigra_ext.o
TEST_6_CFLAGS = $(TEST_CFLAGS)
TEST_6_LFLAGS = $(TEST_LFLAGS)

TEST_7_BIN = common
TEST_7_OBJ = common.o
TEST_7_CFLAGS = $(TEST_CFLAGS)
TEST_7_LFLAGS = $(TEST_LFLAGS)


# ====================================================================
# Do not edit anything below this
# ====================================================================

include $(PREFIX)/makefiles/rules.mk
-include $(DEPEND)


#run_tests: panorama
#	./panorama

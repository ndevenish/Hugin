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

CFLAGS += $(VIGRA_CFLAGS)
LFLAGS +=  -lpanorama $(VIGRA_LFLAGS) -ljhead -lcommon -lvigra_ext \
           -lpano12

# ====================================================================
# The applications that should be build in this directory.
#
# ====================================================================

ifeq ($(HAS_FULL_VIGRA),1)

APPS = 1 2 3 4 5 7

# App 1

APP_1_BIN = nona
APP_1_OBJ = nona.o

APP_1_LFLAGS = $(LFLAGS)

APP_2_BIN = autopano_old
APP_2_OBJ = autopano.o

APP_2_LFLAGS = $(LFLAGS) -lklt -lfftw

APP_3_BIN = automatch
APP_3_OBJ = automatch.o

APP_3_LFLAGS = $(LFLAGS) -lklt -lfftw

APP_4_BIN = panosifter
APP_4_OBJ = panosifter.o

APP_4_LFLAGS = $(LFLAGS)

APP_5_BIN = autooptimiser
APP_5_OBJ = autooptimiser.o

APP_5_LFLAGS = $(LFLAGS)

APP_6_BIN = estimateCA
APP_6_OBJ = estimate_CA_shift.o

APP_6_LFLAGS = $(LFLAGS) -lklt

APP_7_BIN = zhang_undistort
APP_7_OBJ = zhang_undistort.o

APP_7_LFLAGS = $(LFLAGS)


endif

# ====================================================================
# Do not edit anything below this
# ====================================================================

include $(PREFIX)/makefiles/rules.mk
-include $(DEPEND)



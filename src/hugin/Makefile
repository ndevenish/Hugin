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

SUBDIRS = xrc po

# ====================================================================
# Files that should be considered while creating the dependencies
# ====================================================================

DEPEND_FILES = *.cpp

# ====================================================================
# special compiler and linker flags for this directory
# ====================================================================

CFLAGS += $(WXWIN_CFLAGS) -I../ $(INCL_RESOURCES_FLAG) $(VIGRA_CFLAGS)
LFLAGS += $(XRC_LFLAGS) $(WXWIN_LFLAGS)  -lpanorama \
           -ljhead -lcommon -lvigra_ext ${VIGRA_LFLAGS}

# ====================================================================
# Some of the object in this directory you might want to put in a
# library to make them easily available from other locations. The
# library name should be without the leading lib and trailing
# .a. Those are added automatically.
# ====================================================================

LIBS = 1

# our image processing functions. separarte lib, to test them
# with a programm in tests/
LIB_1_BIN = wximageproc
LIB_1_OBJ = ImageProcessing.o \
	    wxVigraImage.o \
            ImageCache.o
LIB_1_LFLAGS =  $(LFLAGS)

# ====================================================================
# The applications that should be build in this directory.
#
# ====================================================================


APPS = 1

# App 1

APP_1_BIN = hugin
APP_1_OBJ = \
            huginApp.o \
	    ImagesList.o \
            Server.o \
            MainFrame.o \
            CPImageCtrl.o \
            CPEditorPanel.o \
            LensPanel.o \
            ImagesPanel.o \
            ImageCache.o \
            CommandHistory.o \
	    ImageProcessing.o \
	    wxVigraImage.o \
            PanoPanel.o \
	    PreviewFrame.o \
	    PreviewPanel.o \
	    ImageCenter.o \
	    OptimizePanel.o \
	    RunOptimizerFrame.o \
	    RunStitcherFrame.o \
	    CPListFrame.o \
	    TextKillFocusHandler.o \
	    ImageOrientationPanel.o \
	    ImageOrientationFrame.o \
	    PanoDruid.o \
            $(RES_O)

ifeq ($(PLATFORM),win32)
   APP_1_OBJ += hugin_rc.o
endif

APP_1_DESKTOP = hugin.desktop

APP_1_LFLAGS = $(LFLAGS) -lpano12

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
# Code generation for the ui
# For distributions we include pixmaps and ui layouts in the code.
# comment INCL_RESOURCES_FLAG out for including all resources
# ====================================================================

#INCL_RESOURCES_FLAG = -D_INCLUDE_UI_RESOURCES
RESOURCES = xrc/*.xrc \
            xrc/data/about.htm

ifdef INCL_RESOURCES_FLAG
RES_O = resource.o
resource.cpp: $(RESOURCES)
	wxrc -c xrc/*.xrc ; test -f resource.cpp
endif


# ====================================================================
# Do not edit anything below this
# ====================================================================

include $(PREFIX)/makefiles/rules.mk
-include $(DEPEND)



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

PREFIX = ../..

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

SUBDIRS = data

# ====================================================================
# Files that should be considered while creating the dependencies
# ====================================================================

DEPEND_FILES = 

# ====================================================================
# special compiler/linker flags needed in this directory
# ====================================================================

CFLAGS +=
LFLAGS +=

# ====================================================================
# Some of the object in this directory you might want to put in a
# library to make them easily available from other locations. The
# library name should be without the leading lib and trailing
# .a. Those are added automatically.
# This is useful for code that is needed from other directories.
#
# Multiple libraries can be defined here, by adding them to the
# LIBS list and specifying details in LIB_x_BIN LIB_x_OBJ
# LIB_x_LFLAGS
# ====================================================================
LIBS =
#LIBS = 1

# Defaults
LIB_LFLAGS = $(LFLAGS)

# Library 1
LIB_1_BIN = somelib
LIB_1_OBJ = somelib.o other.o
LIB_1_LFLAGS =  $(LIB_LFLAGS)


# ====================================================================
# Applications
#
# Note: All the files referenced here should be in the local directory
# This also applies to XRC and XRC_DATA, they should have separate
# Makefile's in their directory.
# ====================================================================

APPS = 1
#APPS = 1 2

# Defaults

APP_LFLAGS =   $(LFLAGS)

# App 1

APP_1_BIN = hugin
# for XRC files (in the current directory)
APP_1_XRC = about.xrc cp_editor_panel.xrc edit_text.xrc main_menu.xrc \
            main_tool.xrc
# for XRC data files, icons, images etc.
APP_1_XRC_DATA =

# App 2

APP_2_BIN = alb
APP_2_OBJ = alb.o alb2.o
APP_2_LFLAGS = $(TEST_LFLAGS)


# ====================================================================
# Some applications are only for testing. To make these compile you
# must explicitly type "make tests"
# ====================================================================


TESTS =

# Defaults

TEST_LFLAGS =   $(LFLAGS)

# Test 1

TEST_1_BIN =
TEST_1_OBJ =
TEST_1_LFLAGS = $(TEST_LFLAGS)


# ====================================================================
# include our rules here
# ====================================================================

include $(PREFIX)/makefiles/rules.mk

# ====================================================================
# add custom rules here if needed
# ====================================================================


# ====================================================================
# do not edit stuff below here.
# ====================================================================

-include $(DEPEND)

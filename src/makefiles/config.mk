# ========================================================================
#
#  config.mk
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

# this is set by the individual makefiles. it is not the install prefix!
# do not change
CODE_ROOT = $(PREFIX)

# ========================================================================
# install locations
# ========================================================================

INSTALL_PREFIX=/tmp/l
INSTALL_ETC_DIR=__ETC_PREFIX__
INSTALL_BIN_DIR=$(INSTALL_PREFIX)/bin
INSTALL_DOC_DIR=$(INSTALL_PREFIX)/share/doc/hugin
INSTALL_DATA_DIR=$(INSTALL_PREFIX)/share/hugin
INSTALL_XRC_DIR=$(INSTALL_DATA_DIR)/xrc
INSTALL_XRC_DATA_DIR=$(INSTALL_XRC_DIR)/data
INSTALL_LOCALE_DIR=$(INSTALL_PREFIX)/locale


# ========================================================================
# General settings
# ========================================================================

# the programs we use (TODO: use configure to detect them)
CC            = gcc
CXX           = g++
CPP	      = gcc -E
PERL	      = perl
AR            = ar r
RANLIB        = ranlib
ECHO          = @echo
RM	      = rm -rf
MKDIR         = install -d

INSTALL       = install -c -p
INSTALL_PROGRAM= ${INSTALL} $(INSTALL_STRIP_FLAG)
INSTALL_DATA  = ${INSTALL} -m 644
INSTALL_SCRIPT= ${INSTALL}
INSTALL_HEADER= $(INSTALL_DATA)

MSGFMT        = msgfmt -v
MSGMERGE      = msgmerge
XGETTEXT      = xgettext
XARGS         = xargs

# common xgettext args: C++ syntax, use the specified macro names as markers
XGETTEXT_ARGS=-C -k_ -s -j


OBJ_DIR       = .obj

DEPEND	      = Makefile.depend

# ========================================================================
# Define path to different types of files
# ========================================================================

INC_DIR   = $(CODE_ROOT)/include
LIB_DIR   = $(CODE_ROOT)/lib
BIN_DIR   = $(CODE_ROOT)/bin
TST_DIR   = $(CODE_ROOT)/test-bin

# ========================================================================
# Command line argument and flags for make
# ========================================================================

ARFLAGS		= rv

CXXFLAGS   +=
LFLAGS     += -L$(LIB_DIR)

CFLAGS	   += -I. -I$(INC_DIR)
CFLAGS     += -Wall -g
CFLAGS     += -D_REENTRANT -D_POSIX_THREADS -D_POSIX_THREAD_SAFE_FUNCTIONS


# ========================================================================
# set to @ if compile commands shouldn't be printed
# ========================================================================

ifeq ($(QUIET),1)
SILENT=@
else
SILENT=
endif


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

CODE_ROOT = $(PREFIX)

# ========================================================================
# General settings
# ========================================================================

CC            = gcc
CXX           = g++
CPP	      = gcc -E
PERL	      = perl
AR            = ar r
RANLIB        = ranlib
ECHO          = @echo
RM	      = rm -rf
MKDIR         = install -d

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

SILENT=
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

SUBDIRS =

# ====================================================================
# Applications
# ====================================================================

APPS = 1
#APPS = 1 2

# Defaults

APP_LFLAGS =   $(LFLAGS)

# App 1

APP_1_BIN = hugin
APP_1_LINGUAS = de


# ====================================================================
# include our rules here
# ====================================================================

include $(PREFIX)/makefiles/rules.mk

# ====================================================================
# add custom rules here if needed
# FIXME hugin.pot creation should consider xrc files also
# ====================================================================

LINGUAS = $(APP_1_LINGUAS)
POTFILE = $(APP_1_BIN).pot

updatepo: $(POTFILE)
	linguas='$(LINGUAS)'; for lang in $$linguas ; do \
	    pofile="$$lang.po"; \
	    if [ -f $$pofile ]; then \
		    echo "Updating catalog: $$pofile"; \
		    $(MSGMERGE) $$pofile $(POTFILE) > $$pofile.new && mv $$pofile.new $$pofile; \
	    else \
		    echo "Creating new catalog: $$pofile"; \
		    cp $(POTFILE) $$pofile; \
	    fi; \
	done

$(POTFILE):
	touch $@
	find .. -name "*.cpp" -or -name "*.h" \
		| $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $(POTFILE)


# ====================================================================
# do not edit stuff below here.
# ====================================================================

-include $(DEPEND)

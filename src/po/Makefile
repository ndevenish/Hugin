# locale/Makefile
#
# Matt Brubeck 2002-01-12
# based on the locale makefile from wxWindows 2.2.9
#  
# This is the makefile for generating the message catalog file and
# building lang.mo files from the translated lang.po catalogs.
# This makefile may be invoked to build either hugin.pot or any lang.mo

PROGNAME=hugin
LINGUAS=de

prefix=/usr/local/
# this defaults to /usr/share
DATADIR=${prefix}/share
top_srcdir=..
INSTALL=/usr/bin/install -c
ifdef DESTDIR
   INSTALL_PATH=$(top_srcdir)/$(DESTDIR)
else
   INSTALL_PATH=
endif

all: allmo
po: all

# Merge POT file with all existing message catalogs
allpo: $(LINGUAS:%=%.po) FORCE
allmo: $(LINGUAS:%=%/$(PROGNAME).mo) FORCE

# Update the POT template file.
POTFILE=$(PROGNAME).pot 
update: force-update $(POTFILE) updatepo FORCE

# the programs we use (TODO: use configure to detect them)
MSGFMT=msgfmt -v
MSGMERGE=msgmerge
XGETTEXT=xgettext
XARGS=xargs

# common xgettext args: C++ syntax, use the specified macro names as markers
XGETTEXT_ARGS=-C -k_ -s -j

# implicit rules

%/$(PROGNAME).mo: %.po
	@test -d $* || mkdir $*
	$(MSGFMT) -o $@ $<

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
	find ../src -name "*.cpp" -or -name "*.h" \
		| $(XARGS) $(XGETTEXT) $(XGETTEXT_ARGS) -o $(POTFILE)

force-update: FORCE
	rm -f $(POTFILE)

FORCE:

install: allmo
	linguas='$(LINGUAS)'; for lang in $$linguas ; do \
	   $(INSTALL) -d $(INSTALL_PATH)$(DATADIR)/locale/$$lang/LC_MESSAGES ; \
	   $(INSTALL) -m 644 $$lang/hugin.mo $(INSTALL_PATH)$(DATADIR)/locale/$$lang/LC_MESSAGES/$(PROGNAME).mo ; \
	done

uninstall:
	linguas='$(LINGUAS)'; for lang in $$linguas ; do \
	   rm -f $(DATADIR)/locale/$$lang/LC_MESSAGES/$(PROGNAME).mo ; \
	done

clean:
	-linguas='$(LINGUAS)'; for lang in $$linguas ; do \
	   rm -rf $$lang ; \
	   rm -f hugin.pot ; \
	done

distclean: clean
	rm -f Makefile


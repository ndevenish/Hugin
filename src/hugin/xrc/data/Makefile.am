# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

SUBDIRS=help_common help_en_EN help_fr_FR

HTM_FILES = $(wildcard $(srcdir)/*.htm)
HTML_FILES = $(wildcard $(srcdir)/*.html)
ICO_FILES = $(wildcard $(srcdir)/*.ico)
JPG_FILES = $(wildcard $(srcdir)/*.jpg)
PNG_FILES = $(wildcard $(srcdir)/*.png)
XPM_FILES = $(wildcard $(srcdir)/*.xpm)
TXT_FILES = $(wildcard $(srcdir)/*.txt)
OTHER_FILES = $(wildcard $(srcdir)/*.mk)

if HAVE_MINGW
xrcdatadir = $(bindir)/xrc/data
else
xrcdatadir = $(datadir)/hugin/xrc/data
endif

all-local: stamp-data

stamp-data: $(HTM_FILES) $(HTML_FILES) $(ICO_FILES) $(JPG_FILES) $(PNG_FILES) $(XPM_FILES) $(TXT_FILES) $(OTHER_FILES)
	for p in $?; do \
	    if test -e `basename $$p` ; then :; else \
		( $(LN_S) $$p . ) ; \
	    fi; \
	done
	@echo "touch stamp-data"
	@echo timestamp > stamp-dataT
	@mv stamp-dataT stamp-data

dist_xrcdata_DATA = $(HTM_FILES) $(HTML_FILES) $(ICO_FILES) $(JPG_FILES) $(PNG_FILES) $(XPM_FILES) $(TXT_FILES) $(OTHER_FILES)

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = hugin.png
hugin.png: $(srcdir)/icon.png
	cp $(srcdir)/icon.png hugin.png

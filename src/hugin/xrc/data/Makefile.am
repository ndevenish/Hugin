# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

HTM_FILES = $(wildcard $(srcdir)/*.htm)
HTML_FILES = $(wildcard $(srcdir)/*.html)
ICO_FILES = $(wildcard $(srcdir)/*.ico)
JPG_FILES = $(wildcard $(srcdir)/*.jpg)
PNG_FILES = $(wildcard $(srcdir)/*.png)
XPM_FILES = $(wildcard $(srcdir)/*.xpm)
TXT_FILES = $(wildcard $(srcdir)/*.txt)
HELP_DIRS = $(wildcard $(srcdir)/help_*)
HELP_FILES = help_en_EN.htb help_fr_FR.htb

if HAVE_MINGW
xrcdatadir = $(bindir)/xrc/data
else
xrcdatadir = $(datadir)/hugin/xrc/data
endif

all-local: stamp-data $(HELP_FILES)

stamp-data: $(HTM_FILES) $(HTML_FILES) $(ICO_FILES) $(JPG_FILES) $(PNG_FILES) $(XPM_FILES) $(TXT_FILES) $(HELP_DIRS)
	for p in $?; do \
	    if test -e `basename $$p` ; then :; else \
		( $(LN_S) $$p . ) ; \
	    fi; \
	done
	@echo "touch stamp-data"
	@echo timestamp > stamp-dataT
	@mv stamp-dataT stamp-data

dist_xrcdata_DATA = $(HTM_FILES) $(HTML_FILES) $(ICO_FILES) $(JPG_FILES) $(PNG_FILES) $(XPM_FILES) $(TXT_FILES) $(HELP_FILES)

# dangelo: probably not the best way, but I'm too lazy to read the make manual now.
HELP_EN_FILES = $(wildcard $(srcdir)/help_en_EN/*.h*) $(wildcard $(srcdir)/help_en_EN/images/*.png) $(wildcard $(srcdir)/help_en_EN/images/*.jpg)
help_en_EN.htb: $(HELP_EN_FILES)
	rm -f help_en_EN.htb
	cd help_en_EN && $(ZIP) ../help_en_EN.htb *.hhc *.html *.hhk *.hhp images/*.png images/*.jpg

HELP_FR_FILES = $(wildcard $(srcdir)/help_fr_FR/*.h*) $(wildcard $(srcdir)/help_fr_FR/images/*.png)
help_fr_FR.htb: $(HELP_FR_FILES)
	rm -f help_fr_FR.htb
	cd help_fr_FR && $(ZIP) ../help_fr_FR.htb *.hhc *.html *.hhk *.hhp images/*.png images/*.jpg

pixmapsdir = $(datadir)/pixmaps
pixmaps_DATA = hugin.png
hugin.png: $(srcdir)/icon.png
	cp $(srcdir)/icon.png hugin.png

dist-hook:
	rm -rf `find $(distdir)/help_en_EN $(distdir)/help_fr_FR -name CVS`
	rm -rf `find $(distdir)/help_en_EN $(distdir)/help_fr_FR -name .cvsignore`

EXTRA_DIST = help_en_EN help_fr_FR


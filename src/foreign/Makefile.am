SUBDIRS= vigra_impex

dist-hook:
	rm -rf `find $(distdir)/vigra $(distdir)/getopt -name CVS`
	rm -rf `find $(distdir)/vigra $(distdir)/getopt -name .cvsignore`

EXTRA_DIST = getopt vigra VIGRA_LICENSE
    
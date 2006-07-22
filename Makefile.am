SUBDIRS=src doc
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	rm -rf `find $(distdir)/admin $(distdir)/doc $(distdir)/m4 $(distdir)/mac $(distdir)/utils -name CVS`
	rm -rf `find $(distdir)/admin $(distdir)/doc $(distdir)/m4 $(distdir)/mac $(distdir)/utils -name .cvsignore`

EXTRA_DIST = admin doc m4 mac utils bootstrap BUGS HACKING LICENCE LICENCE_VIGRA LICENCE_JHEAD README_WINDOWS windows_DOT_NET_compile.txt


SUBDIRS=src
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	rm -rf `find $(distdir)/admin $(distdir)/utils $(distdir)/m4 $(distdir)/mac -name CVS`
	rm -rf `find $(distdir)/admin $(distdir)/utils $(distdir)/m4 $(distdir)/mac -name .cvsignore`

EXTRA_DIST = admin m4 mac utils bootstrap BUGS LICENCE LICENCE_SIFT windows_compile.txt windows_DOT_NET_compile.txt


SUBDIRS=src m4
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	rm -rf `find $(distdir)/admin $(distdir)/utils -name CVS`
	rm -rf `find $(distdir)/admin $(distdir)/utils -name .cvsignore`

EXTRA_DIST = admin utils bootstrap BUGS LICENCE LICENCE_SIFT windows_compile.txt windows_DOT_NET_compile.txt


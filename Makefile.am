SUBDIRS=src
ACLOCAL_AMFLAGS = -I m4

dist-hook:
	rm -rf `find $(distdir)/admin $(distdir)/utils $(distdir)/m4 -name CVS`
	rm -rf `find $(distdir)/admin $(distdir)/utils $(distdir)/m4 -name .cvsignore`

EXTRA_DIST = admin utils m4 bootstrap BUGS LICENCE LICENCE_SIFT windows_compile.txt windows_DOT_NET_compile.txt

# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

SUBDIRS=jhead common foreign Panorama hugin tools nona_gui

dist-hook:
	rm -rf `find $(distdir)/include $(distdir)/dox $(distdir)/tests $(distdir)/build_msvc -name CVS`
	rm -rf `find $(distdir)/include $(distdir)/dox $(distdir)/tests $(distdir)/build_msvc -name .cvsignore`

EXTRA_DIST = include dox Doxyfile tests hugin.rc build_msvc compile-mingw.sh makefile.g95

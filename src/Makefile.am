# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

SUBDIRS=jhead klt common vigra_ext foreign Panorama hugin tools nona_gui keypoints

dist-hook:
	rm -rf `find $(distdir)/PanoImage $(distdir)/include $(distdir)/dox $(distdir)/Doxyfile $(distdir)/tests -name CVS`
	rm -rf `find $(distdir)/PanoImage $(distdir)/include $(distdir)/dox $(distdir)/Doxyfile $(distdir)/tests -name .cvsignore`

EXTRA_DIST = include dox Doxyfile tests hugin.rc Hugin.sln compile-mingw.sh makefile.g95

# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

if BuildGUI
  MAYBE_HUGIN = hugin
  MAYBE_NONA_GUI = nona_gui
endif
SUBDIRS=jhead common foreign vigra_ext Panorama tools $(MAYBE_HUGIN) $(MAYBE_NONA_GUI)

dist-hook:
	rm -rf `find $(distdir)/include $(distdir)/dox $(distdir)/tests $(distdir)/build_msvc -name CVS`
	rm -rf `find $(distdir)/include $(distdir)/dox $(distdir)/tests $(distdir)/build_msvc -name .cvsignore`

EXTRA_DIST = include dox Doxyfile tests hugin.rc build_msvc compile-mingw.sh makefile.g95

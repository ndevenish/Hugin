# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."
INCLUDES    = -I$(top_builddir)/src/include -I${top_srcdir}/src/include -I${top_srcdir}/src/foreign $(WX_CXXFLAGS)
AM_CXXFLAGS = $(JPEG_FLAGS) $(PNG_FLAGS) $(TIFF_FLAGS) $(ZLIB_FLAGS) $(PANO_FLAGS) $(BOOST_CPPFLAGS) -DINSTALL_XRC_DIR=\"$(datadir)/hugin/xrc\" -DINSTALL_LOCALE_DIR=\"${datadir}/locale\"
AM_LDFLAGS  = $(WX_LIBS) $(LIB_JPEG) $(LIB_PNG) $(LIB_TIFF) $(LIB_ZLIB) $(LIB_PANO) $(BOOST_THREAD_LIB)

SUFFIXES  = .rc

RC        = @WINDRES@
RCFLAGS   = --use-temp-file --verbose -O COFF

RCCOMPILE = $(RC) $(RCFLAGS) \
	    `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) |\
	    sed -e 's/-I/--include-dir /g;s/-D/--define /g;s/-m.*//g'`


SUBDIRS   = po

if HAVE_MINGW
WIN_SRC   = nona_gui_rc.rc
else
WIN_SRC   =
endif

bin_PROGRAMS     = nona_gui
nona_gui_SOURCES = nona_gui.cpp $(WIN_SRC)

nona_gui_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
                   ${top_builddir}/src/jhead/libjhead.la \
		   ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
                   ${top_builddir}/src/hugin/libhuginwxutils.la \
                   ${top_builddir}/src/vigra_ext/libvigra_ext.la

EXTRA_DIST       = nona_gui_rc.rc nona_gui.vcproj

.rc.o:
	$(RCCOMPILE) -i $< -o $@


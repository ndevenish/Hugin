# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

SUFFIXES  = .rc

RC        = @WINDRES@
RCFLAGS   = --use-temp-file --verbose -O COFF

RCCOMPILE = $(RC) $(RCFLAGS) \
	    `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) |\
	    sed -e 's/-I/--include-dir /g;s/-D/--define /g'`
					  
SUBDIRS   = po

if HAVE_WIN32
WIN_SRC   = nona_gui_rc.rc
else
WIN_SRC   =
endif

INCLUDES    = -I${top_srcdir}/src/include -I${top_srcdir}/src/foreign
AM_CXXFLAGS = $(WX_CXXFLAGS)
AM_LDFLAGS  = $(WX_LIBS) $(LIB_JPEG) $(LIB_PNG) $(LIB_TIFF) $(LIB_ZLIB) $(LIB_PANO)

bin_PROGRAMS     = nona_gui
nona_gui_SOURCES = nona_gui.cpp $(WIN_SRC)

nona_gui_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
                   ${top_builddir}/src/jhead/libjhead.la ${top_builddir}/src/vigra_ext/libvigra_ext.la \
		   ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/hugin/libhuginwxutils.la

EXTRA_DIST       = nona_gui_rc.rc

.rc.o:
	$(RCCOMPILE) -i $< -o $@


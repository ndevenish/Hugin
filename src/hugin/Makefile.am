# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."
INCLUDES    = -I${top_srcdir}/src/foreign -I${top_srcdir}/src/include -I.. $(FFTW_INCLUDES) $(WX_CXXFLAGS) 
AM_CXXFLAGS = $(GTK_CFLAGS) $(PANO_FLAGS) $(BOOST_CPPFLAGS)

SUFFIXES    = .rc

RC          = @WINDRES@
RCFLAGS     = --use-temp-file --verbose -O COFF

RCCOMPILE   = $(RC) $(RCFLAGS) \
	      `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) |\
	      sed -e 's/-I/--include-dir /g;s/-D/--define /g;s/-m.*//g'`

STD_SRC     = huginApp.cpp ImagesList.cpp AutoCtrlPointCreator.cpp MainFrame.cpp CPImageCtrl.cpp CPEditorPanel.cpp LensPanel.cpp \
	      ImagesPanel.cpp CommandHistory.cpp PanoPanel.cpp NonaStitcherPanel.cpp PTStitcherPanel.cpp PreviewFrame.cpp \
	      PreviewPanel.cpp ImageCenter.cpp OptimizePanel.cpp RunOptimizerFrame.cpp RunStitcherFrame.cpp CPListFrame.cpp \
	      TextKillFocusHandler.cpp ImageOrientationPanel.cpp ImageOrientationFrame.cpp PanoDruid.cpp \
	      UniversalCursor.cpp CPZoomDisplayPanel.cpp CPFineTuneFrame.cpp \
	      PreferencesDialog.cpp

if HAVE_WIN32
WIN_SRC    = hugin_rc.rc
else
WIN_SRC    =
endif

SUBDIRS    = xrc po

noinst_LTLIBRARIES         = libhuginwxutils.la
libhuginwxutils_la_SOURCES = ImageCache.cpp MyProgressDialog.cpp

bin_PROGRAMS   = hugin
hugin_SOURCES  = $(STD_SRC) $(WIN_SRC)
hugin_LDFLAGS  = $(WX_LIBS) $(GTK_LIBS) $(FFTW_LDFLAGS) $(LIB_FFTW) $(LIB_PANO) $(LIB_TIFF) $(LIB_PNG) $(LIB_JPEG) $(LIB_ZLIB)
hugin_LDADD    = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
                 ${top_builddir}/src/jhead/libjhead.la ${top_builddir}/src/common/libcommon.la  \
                 ${top_builddir}/src/vigra_ext/libvigra_ext.la libhuginwxutils.la
noinst_HEADERS = win_prep.h

EXTRA_DIST = hugin.vcproj hugin_rc.rc hugin.desktop.tmpl

.rc.o:
	$(RCCOMPILE) -i $< -o $@


# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I${top_srcdir}/src/foreign -I${top_srcdir}/src/include -I.. $(FFTW_INCLUDES)
AM_CXXFLAGS = $(WX_CXXFLAGS) $(GTK_CFLAGS) 
AM_LDFLAGS = $(WX_LIBS) $(GTK_LIBS) $(FFTW_LDFLAGS) $(LIB_FFTW) $(LIB_PANO)

SUBDIRS = xrc po

noinst_LTLIBRARIES = libhuginwxutils.la
libhuginwxutils_la_SOURCES = ImageCache.cpp MyProgressDialog.cpp

bin_PROGRAMS   = hugin
hugin_SOURCES  = huginApp.cpp ImagesList.cpp AutoCtrlPointCreator.cpp MainFrame.cpp CPImageCtrl.cpp CPEditorPanel.cpp LensPanel.cpp \
                 ImagesPanel.cpp CommandHistory.cpp PanoPanel.cpp NonaStitcherPanel.cpp PTStitcherPanel.cpp PreviewFrame.cpp \
                 PreviewPanel.cpp ImageCenter.cpp OptimizePanel.cpp RunOptimizerFrame.cpp RunStitcherFrame.cpp CPListFrame.cpp \
                 TextKillFocusHandler.cpp ImageOrientationPanel.cpp ImageOrientationFrame.cpp PanoDruid.cpp \
                 UniversalCursor.cpp CPZoomDisplayPanel.cpp CPFineTuneFrame.cpp
hugin_LDADD    = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
                 ${top_builddir}/src/jhead/libjhead.la ${top_builddir}/src/common/libcommon.la  \
                 ${top_builddir}/src/vigra_ext/libvigra_ext.la libhuginwxutils.la
noinst_HEADERS = win_prep.h

EXTRA_DIST = hugin.vcproj hugin_rc.rc hugin.desktop.tmpl

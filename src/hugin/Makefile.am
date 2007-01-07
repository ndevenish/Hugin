# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."
INCLUDES    = -I$(top_builddir)/src/include -I${top_srcdir}/src/foreign -I${top_srcdir}/src/include -I.. $(WX_CXXFLAGS)
AM_CXXFLAGS = $(GTK_CFLAGS) $(PANO_FLAGS) $(BOOST_CPPFLAGS) -DINSTALL_XRC_DIR=\"$(datadir)/hugin/xrc\" -DINSTALL_LOCALE_DIR=\"${datadir}/locale\"

SUFFIXES    = .rc

RC          = @WINDRES@
RCFLAGS     = --use-temp-file --verbose -O COFF

RCCOMPILE   = $(RC) $(RCFLAGS) \
	      `echo $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) |\
	      sed -e 's/-I/--include-dir /g;s/-D/--define /g;s/-m.*//g'`

STD_SRC     = huginApp.cpp ImagesList.cpp AutoCtrlPointCreator.cpp MainFrame.cpp CPImageCtrl.cpp CPEditorPanel.cpp LensPanel.cpp \
	      ImagesPanel.cpp CommandHistory.cpp PanoPanel.cpp NonaStitcherPanel.cpp PTStitcherPanel.cpp PreviewFrame.cpp \
	      PreviewPanel.cpp CropPanel.cpp OptimizePanel.cpp RunStitcherFrame.cpp CPListFrame.cpp \
	      TextKillFocusHandler.cpp PanoDruid.cpp \
	      CPZoomDisplayPanel.cpp CPFineTuneFrame.cpp \
	      PreferencesDialog.cpp LocalizedFileTipProvider.cpp VigCorrDialog.cpp \
	      wxPanoCommand.cpp MyExternalCmdExecDialog.cpp UniversalCursor.cpp \
	      AssistantPanel.cpp HFOVDialog.cpp

if HAVE_MINGW
WIN_SRC    = hugin_rc.rc
else
WIN_SRC    =
endif

SUBDIRS    = xrc po

noinst_LTLIBRARIES         = libhuginwxutils.la
libhuginwxutils_la_SOURCES = ImageCache.cpp MyProgressDialog.cpp Plot2D.cpp \
	PTWXDlg.cpp

bin_PROGRAMS   = hugin
hugin_SOURCES  = $(STD_SRC) $(WIN_SRC)
hugin_LDFLAGS  = $(WX_LIBS) $(GTK_LIBS) $(LIB_TIFF) $(LIB_PNG) $(LIB_JPEG) $(LIB_ZLIB) $(LIB_PANO) $(BOOST_THREAD_LIB)
hugin_LDADD    = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
	             ${top_builddir}/src/jhead/libjhead.la ${top_builddir}/src/common/libcommon.la  \
	             libhuginwxutils.la ${top_builddir}/src/vigra_ext/libvigra_ext.la
noinst_HEADERS = win_prep.h

desktopdir = $(datadir)/applications
desktop_DATA = hugin.desktop

sharedmimedir = $(datadir)/mime
sharedmimepackagedir = $(sharedmimedir)/packages
sharedmimepackage_DATA = hugin.xml

gnomemimeicondir = $(datadir)/icons/gnome/48x48/mimetypes
gnomemimeicon_DATA = gnome-mime-application-x-ptoptimizer-script.png

EXTRA_DIST = hugin.vcproj hugin_rc.rc hugin.desktop hugin.xml gnome-mime-application-x-ptoptimizer-script.png

.rc.o:
	$(RCCOMPILE) -i $< -o $@

install-data-hook:
	- if test -n "$(UPDATE_MIME_DATABASE)"; then \
	  if test -z "${DESTDIR}" ; then \
		echo "*** calling $(UPDATE_MIME_DATABASE) \"${sharedmimedir}\""; \
		$(UPDATE_MIME_DATABASE) "${sharedmimedir}"; \
	  else \
		echo "*** After installation call $(UPDATE_MIME_DATABASE) \"${sharedmimedir}\""; \
	  fi; \
	fi
	if test -n "$(UPDATE_DESKTOP_DATABASE)"; then \
	  if test -z "${DESTDIR}" ; then \
		echo "*** calling $(UPDATE_DESKTOP_DATABASE)"; \
		if "$(UPDATE_DESKTOP_DATABASE)"; then \
			echo "Update desktop database ok"; \
		else \
			echo "Update desktop database returned an error" ;\
		fi\
	  else \
		echo "*** After installation call $(UPDATE_DESKTOP_DATABASE)"; \
	  fi; \
	fi


uninstall-local: uninstall-sharedmimepackageDATA uninstall-desktopDATA
	- if test -n "$(UPDATE_MIME_DATABASE)"; then \
	  if test -z "${DESTDIR}" ; then \
		$(UPDATE_MIME_DATABASE) "${sharedmimedir}"; \
	  else \
		echo "*** After uninstallation call $(UPDATE_MIME_DATABASE) \"${sharedmimedir}\""; \
	  fi; \
	fi
	if test -n "$(UPDATE_DESKTOP_DATABASE)"; then \
	  if test -z "${DESTDIR}" ; then \
		$(UPDATE_DESKTOP_DATABASE); \
	  else \
		echo "*** After uninstallation call $(UPDATE_DESKTOP_DATABASE)"; \
	  fi; \
	fi


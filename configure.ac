dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.5)
AC_INIT([hugin], [0.6], [pablo.dangelo@web.de])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_TARGET
AC_CONFIG_HEADER([src/include/config.h])
AM_INIT_AUTOMAKE([no-define dist-bzip2 dist-zip])

dnl By default, do not create shared libraries
AC_DISABLE_SHARED

dnl
dnl  Check the operating system
dnl
case "${target_os}" in
  "")
    HSYS=unknown
    ;;
  linux*)
    HSYS=linux
	dnl
	dnl Check the cpu to allow lib/lib64 decisions
	dnl
	case "${target_cpu}" in
	  "")
	    HCPU=unknown
	    ;;
	  x86_64*)
	    HCPU=amd64
	    ;;
	  *)
	    HCPU="${target_cpu}"
	    ;;
	esac
    ;;
  darwin*)
    HSYS=darwin
	HCPU="${target_cpu}"
    ;;
  *mingw32*)
    AC_ARG_VAR(WINDRES, [Windows Resource compiler tool path])
    AC_PATH_TOOL(WINDRES,windres,
	             [AC_MSG_ERROR(Could not find a windres tool in your PATH. Will not be able to compile hugin or nona_gui:)]
		     )
    AC_SUBST(WINDRES)

    HSYS=mingw32
	HCPU="${target_cpu}"

    ;;
  *)
    HSYS="${target_os}"
	HCPU="${target_cpu}"
    ;;
esac
AM_CONDITIONAL(HAVE_LINUX, test "${HSYS}" = "linux")
AM_CONDITIONAL(HAVE_DARWIN, test "${HSYS}" = "darwin")
AM_CONDITIONAL(HAVE_MINGW, test "${HSYS}" = "mingw32")

AM_CONDITIONAL(HAVE_AMD64, test "${HCPU}" = "amd64")

AC_ARG_ENABLE(gui,
          AC_HELP_STRING([--disable-gui],
		         [disable gui (compile only nona and fulla)]))


dnl Whether GCJ_PROPERTIES is used depends on the target.
if test "x$enable_gui" = 'xno'; then
	with_gui='no'
else
	with_gui='yes'
fi

AM_CONDITIONAL(BuildGUI, test "x$with_gui" = 'xyes')


dnl --- look for "ZIP", required for building the help file
AC_PATH_PROG(ZIP, zip, no)
if [[ $ZIP = "no" ]] ; then
        AC_MSG_ERROR("Could not find zip - needed to create the help file");
        dnl TODO
        dnl make non-fatal - i.e. just don't build help
fi



AC_ARG_WITH([unicode],
            AC_HELP_STRING([--with-unicode=yes/no],
	    [set to 'yes' if you are using unicode wxWidgets library. (default=no)]),
	    [with_unicode=$withval],
	    [with_unicode='no'])

dnl debugging options
AC_ARG_ENABLE([debug],
          AC_HELP_STRING([--enable-debug],
	    			     [enable debugging of the hugin applictions(default=no).]),
	      [enable_debug=$enableval],
	      [enable_debug=no])

if test "x$enable_debug" = xyes; then
	CFLAGS="$CFLAGS -g -DDEBUG"
	CXXFLAGS="$CXXFLAGS -g -DDEBUG"
fi

dnl  create local defines in config.h.in
AH_TEMPLATE([HUGIN_VERSION],[Current Version of Hugin])
AH_TEMPLATE([INSTALL_LOCALE_DIR],[Install Location of Hugin])
AH_TEMPLATE([INSTALL_XRC_DIR],[Location for XRC information])

dnl I18n support
AM_GNU_GETTEXT([external])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
dnl AC_PROG_LIBTOOL is going to check for this anyway 
dnl so keep it to a minimum
AC_PROG_F77(g77)
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S

dnl desktop integration options
AC_ARG_ENABLE([desktop],
          AC_HELP_STRING([--enable-desktop],
	    			     [enable desktop integration using freedesktop standards(default=yes).]),
	      [enable_desktop=$enableval],
	      [enable_desktop=yes])

if test "x$enable_desktop" = xyes; then
	AC_PATH_PROG(UPDATE_MIME_DATABASE, update-mime-database)
	AC_PATH_PROG(UPDATE_DESKTOP_DATABASE, update-desktop-database)
fi

dnl Checks for libraries.
AM_OPTIONS_WXCONFIG

AX_CHECK_ZLIB
if test "x$have_zlib" != xyes; then
    AC_MSG_ERROR([
	the zlib library must be installed on your system
 	but configure could not find it. Use --with-zlib to
	specify the location of the zlib library
    ])
fi

AX_CHECK_PNG
if test "x$have_png" != xyes; then
    AC_MSG_ERROR([
	the png library must be installed on your system
 	but configure could not find it. Use --with-png to
	specify the location of the PNG library
    ])
fi

AX_CHECK_JPEG
if test "x$have_jpeg" != xyes; then
    AC_MSG_ERROR([
	the jpeg library must be installed on your system
 	but configure could not find it. Use --with-jpeg to
	specify the location of the JPEG library
    ])
fi

AX_CHECK_TIFF
if test "x$have_tiff" != xyes; then
    AC_MSG_ERROR([
	the tiff library must be installed on your system
 	but configure could not find it. Use --with-tiff to
	specify the location of the TIFF library
    ])
fi

AX_CHECK_PANO
if test "x$have_pano" != xyes; then
    AC_MSG_ERROR([
	the panorama tools library must be installed on your system
 	but configure could not find it. Use --with-pano to
	specify the location of the panotools library
    ])
fi

dnl dangelo: we need to force the boost check somehow, since it is a requirement
dnl not an options. 
want_boost_thread=yes
AX_BOOST([1.31],
[echo "boost found"],
[AC_MSG_ERROR([
	the boost headers must be installed on your system
 	but configure could not find it. Use --with-boost to
	specify the location of the boost library and 
	--with-boost-version to specify your version
    ])  ])

if test "x$ax_cv_boost_thread" != xyes; then
    AC_MSG_ERROR([
	the boost thread library must be installed on your system
	but configure could not find it. Use --with-boost to
	specify the location of the boost library and 
	--with-boost-version to specify your version.
	On debian based systems, install libboost-thread-dev
    ])
fi

dnl AX_CHECK_BOOST_THREAD
dnl if test "x$have_boost" != xyes; then
dnl     AC_MSG_ERROR([
dnl 	the boost headers must be installed on your system
dnl 	but configure could not find it. Use --with-boost to
dnl 	specify the location of the boost library and 
dnl 	--with-boost-version to specify your version
dnl     ])
dnl fi

dnl if test "x$ac_have_boost_thread" != xyes; then
dnl     AC_MSG_ERROR([
dnl 	the boost thread library must be installed on your system
dnl 	but configure could not find it. Use --with-boost to
dnl 	specify the location of the boost library and 
dnl 	--with-boost-version to specify your version
dnl     ])
dnl fi

if test "x$with_gui" = 'xyes' ; then

if test "x$with_unicode" = 'xyes' ; then
  AM_PATH_WXCONFIG(2.4.2, wxWin=1,,, --unicode=yes)
else
  AM_PATH_WXCONFIG(2.4.2, wxWin=1,,, --unicode=no)
fi
if test "x$wxWin" != x1; then
    AC_MSG_ERROR([
	wxWidgets must be installed on your system
 	but wx-config script couldn't be found.

 	Please check that wx-config is in path, the directory
 	where wxWidgets libraries are installed (returned by
 	'wx-config --libs' command) is in LD_LIBRARY_PATH or
 	equivalent variable and wxWidgets version is 2.4.2 or above.
    ])
fi

CHECK_WX_BUILT_WITH_GTK2

if test "x$GTK_USEDVERSION" = x2 ; then
  AM_PATH_GTK_2_0(2.0.3, havegtk2=yes, havegtk2=no, gthread)
  GTK_DEFS="-D__GTK2__"
else
  if test "x$GTK_USEDVERSION" = x1 ; then
    AM_PATH_GTK(1.2.0, havegtk=yes, havegtk=no, gthread)
    GTK_DEFS=""
  fi
fi

fi

AC_DEFINE_UNQUOTED(HUGIN_VERSION, $PACKAGE_VERSION)

case "$HSYS" in
  mingw32)
    AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "./locale")
    AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "./xrc")
    ;;
  *)
    if test "x$prefix" != 'xNONE' ; then
      if test "$datadir" != '${prefix}/share' ; then
        AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$datadir/locale")
        AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$datadir/hugin/xrc")
      else
        AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$prefix/share/locale")
        AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$prefix/share/hugin/xrc")
      fi
    else
      if test "$datadir" != '${prefix}/share' ; then
        AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$datadir/locale")
        AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$datadir/hugin/xrc")
      else
        AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$ac_default_prefix/share/locale")
        AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$ac_default_prefix/share/hugin/xrc")
      fi
    fi  
    ;;
esac
						
dnl
dnl Remove extraneous spaces from output variables (aesthetic)
dnl
CC=`echo $CC | sed -e 's/  */ /g'`
CFLAGS=`echo $CFLAGS | sed -e 's/  */ /g'`
CPPFLAGS=`echo $CPPFLAGS | sed -e 's/  */ /g'`
CXXFLAGS=`echo $CXXFLAGS | sed -e 's/  */ /g'`
LDFLAGS=`echo $LDFLAGS | sed -e 's/  */ /g'`
WX_CXXFLAGS=`echo $WX_CXXFLAGS | sed -e 's/  */ /g'`
WX_LIBS=`echo $WX_LIBS | sed -e 's/  */ /g'`
GTK_CFLAGS=`echo $GTK_CFLAGS | sed -e 's/  */ /g'`
GTK_LIBS=`echo $GTK_LIBS | sed -e 's/  */ /g'`

dnl Over-ride LIBS so that libraries are supplied by Makefiles instead
LIBS=""

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/Panorama/Makefile
                 src/common/Makefile
                 src/foreign/Makefile
                 src/foreign/vigra_impex/Makefile
                 src/hugin/Makefile
		 src/hugin/po/Makefile.in
                 src/hugin/xrc/Makefile
                 src/hugin/xrc/data/Makefile
                 src/hugin/xrc/data/help_common/Makefile
                 src/hugin/xrc/data/help_fr_FR/Makefile
                 src/hugin/xrc/data/help_en_EN/Makefile
                 src/jhead/Makefile
                 src/nona_gui/Makefile
		 src/nona_gui/po/Makefile.in
		 src/vigra_ext/Makefile
                 src/tools/Makefile
		 doc/Makefile])

AC_SUBST(ac_aux_dir)

AC_OUTPUT

echo ""
echo "hugin is configured as follows. Please verify that this configuration"
echo "matches your expectations."
echo ""
echo "Host system type : $host"
echo ""
echo "                  Option                        Value"
echo "-------------------------------------------------------------------------"
echo "Shared libraries --enable-shared=$enable_shared		$libtool_build_shared_libs"
echo "Static libraries --enable-static=$enable_static		$libtool_build_static_libs"
echo "JPEG             --with-jpeg=$with_jpeg		have_jpeg  = $have_jpeg"
echo "PNG              --with-png=$with_png		have_png   = $have_png"
echo "TIFF             --with-tiff=$with_tiff		have_tiff  = $have_tiff"
echo "ZLIB             --with-zlib=$with_zlib		have_zlib  = $have_zlib"
echo "PANO             --with-pano=$with_pano		have_pano  = $have_pano"
echo "BOOST            --with-boost=$with_boost		have_boost = $have_boost"
echo "Unicode          --with-unicode=$with_unicode"
echo "Debug enabled?   $enable_debug"
echo "compile GUI programs hugin and nona_gui:    $with_gui"
echo ""
echo "Options used to compile and link:"
echo "  PREFIX           = $prefix"
echo "  VERSION          = $PACKAGE_VERSION"
echo "  CC               = $CC"
echo "  CFLAGS           = $CFLAGS"
echo "  CPPFLAGS         = $CPPFLAGS"
echo "  DEFS             = $DEFS"
echo "  LDFLAGS          = $LDFLAGS"
echo "  LIBS             = $LIBS"
echo "  CXX              = $CXX"
echo "  CXXFLAGS         = $CXXFLAGS"
echo "  JPEG_FLAGS       = $JPEG_FLAGS"
echo "  LIB_JPEG         = $LIB_JPEG"
echo "  PNG_FLAGS        = $PNG_FLAGS"
echo "  LIB_PNG          = $LIB_PNG"
echo "  TIFF_FLAGS       = $TIFF_FLAGS"
echo "  LIB_TIFF         = $LIB_TIFF"
echo "  ZLIB_FLAGS       = $ZLIB_FLAGS"
echo "  LIB_ZLIB         = $LIB_ZLIB"
echo "  PANO_FLAGS       = $PANO_FLAGS"
echo "  LIB_PANO         = $LIB_PANO"
echo "  WX_CXXFLAGS      = $WX_CXXFLAGS"
echo "  WX_LIBS          = $WX_LIBS"
echo "  GTK_CFLAGS       = $GTK_CFLAGS"
echo "  GTK_LIBS         = $GTK_LIBS"
echo "  boost_version    = $ac_boost_version"
echo "  BOOST_CPPFLAGS   = $BOOST_CPPFLAGS"
echo "  BOOST_LIBS       = $BOOST_LIBS"
echo "  BOOST_THREAD_LIB = $BOOST_THREAD_LIB"
echo "  BUILD            = $HSYS"
echo "  CPU              = $HCPU"
echo ""


dnl                                               -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.5)
AC_INIT([hugin], [0.4], [pablo.dangelo@web.de])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_TARGET
AC_CONFIG_HEADER([src/hugin/config.h])
AM_INIT_AUTOMAKE([no-define dist-bzip2 dist-zip])

dnl By default, do not create shared libraries
AC_DISABLE_SHARED

dnl
dnl  Check the operating system
dnl
case "${target_os}" in
  "")
    SYS=unknown
    ;;
  linux*)
    SYS=linux
    ;;
  darwin*)
    SYS=darwin
    ;;
  *mingw32*)
    AC_ARG_VAR(WINDRES, [Windows Resource compiler tool path])
    AC_PATH_TOOL(WINDRES,windres,
	             [AC_MSG_WARN(Could not find a windres tool in your PATH. Will not be able to compile hugin or nona_gui:)]
		     )
    AC_SUBST(WINDRES)

    SYS=mingw32

    ;;
  *)
    SYS="${target_os}"
    ;;
esac
AM_CONDITIONAL(HAVE_LINUX, test "${SYS}" = "linux")
AM_CONDITIONAL(HAVE_DARWIN, test "${SYS}" = "darwin")
AM_CONDITIONAL(HAVE_WIN32, test "${SYS}" = "mingw32")

dnl debugging options
AC_ARG_ENABLE([debug],
              AC_HELP_STRING([--enable-debug],
	                     [enable debugging of the hugin applictions(default=no). Note: -g is removed from flags if this is not defined]),
	      [enable_debug=$enableval
	      CFLAGS="$CFLAGS -g -DDEBUG"
	      CXXFLAGS="$CXXFLAGS -g -DDEBUG"
	      ],
	      [enable_debug=no])

dnl  create local defines in config.h.in
AH_TEMPLATE([HUGIN_VERSION],[Current Version of Hugin])
AH_TEMPLATE([INSTALL_LOCALE_DIR],[Install Location of Hugin])
AH_TEMPLATE([INSTALL_XRC_DIR],[Location for XRC information])

dnl I18n support
AM_GNU_GETTEXT([external])

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S

dnl Checks for libraries.
AM_OPTIONS_WXCONFIG

AX_CHECK_ZLIB
if test "x$have_zlib" != xyes; then
    AC_MSG_WARN([
	the zlib library must be installed on your system
 	but configure could not find it.
    ])
fi

AX_CHECK_PNG
if test "x$have_png" != xyes; then
    AC_MSG_WARN([
	the png library must be installed on your system
 	but configure could not find it.
    ])
fi

AX_CHECK_JPEG
if test "x$have_jpeg" != xyes; then
    AC_MSG_WARN([
	the jpeg library must be installed on your system
 	but configure could not find it.
    ])
fi

AX_CHECK_TIFF
if test "x$have_tiff" != xyes; then
    AC_MSG_WARN([
	the tiff library must be installed on your system
 	but configure could not find it.
    ])
fi

AX_CHECK_PANO
if test "x$have_pano" != xyes; then
    AC_MSG_ERROR([
	the panorama tools library must be installed on your system
 	but configure could not find it.
    ])
fi

AC_PATH_FFTW

RS_BOOST([1.30.0],[],[AC_MSG_NOTICE([Package requires the Boost C++ libraries only for test programs])])
RS_BOOST_UNIT_TEST_FRAMEWORK

AM_PATH_WXCONFIG(2.4.2, wxWin=1)
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

dnl Checks for header files.
dnl AC_HEADER_DIRENT
dnl AC_HEADER_STDC
dnl AC_HEADER_SYS_WAIT
dnl AC_CHECK_HEADERS([float.h limits.h locale.h memory.h paths.h stdlib.h string.h sys/time.h unistd.h utime.h])

dnl Checks for typedefs, structures, and compiler characteristics.
dnl AC_HEADER_STDBOOL
dnl AC_C_CONST
dnl AC_C_INLINE
dnl AC_C_RESTRICT
dnl AC_TYPE_SIZE_T
dnl AC_HEADER_TIME
dnl AC_STRUCT_TM
dnl AC_CHECK_TYPES([ptrdiff_t])

dnl Checks for library functions.
dnl AC_FUNC_CLOSEDIR_VOID
dnl dnl AC_FUNC_ERROR_AT_LINE
dnl AC_FUNC_FORK
dnl AC_FUNC_MALLOC
dnl AC_FUNC_MEMCMP
dnl AC_FUNC_MKTIME
dnl AC_FUNC_SELECT_ARGTYPES
dnl AC_FUNC_SETVBUF_REVERSED
dnl AC_FUNC_STRFTIME
dnl AC_FUNC_STRTOD
dnl AC_FUNC_VPRINTF
dnl AC_CHECK_FUNCS([dup2 floor gettimeofday localtime_r memmove memset pow select setlocale sqrt])

AC_DEFINE_UNQUOTED(HUGIN_VERSION, $PACKAGE_VERSION)
if test "x$prefix" != xNONE; then
    AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$prefix/share/locale")
    AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$prefix/share/hugin/xrc")
else
    AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$ac_default_prefix/share/locale")
    AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$ac_default_prefix/share/hugin/xrc")
fi	

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
		 m4/Makefile
                 src/Makefile
                 src/Panorama/Makefile
                 src/common/Makefile
                 src/foreign/Makefile
                 src/foreign/vigra_impex/Makefile
                 src/hugin/Makefile
		 src/hugin/po/Makefile.in
                 src/hugin/xrc/Makefile
                 src/hugin/xrc/data/Makefile
                 src/jhead/Makefile
                 src/keypoints/Makefile
                 src/klt/Makefile
                 src/nona_gui/Makefile
		 src/nona_gui/po/Makefile.in
                 src/tools/Makefile
                 src/vigra_ext/Makefile])

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
echo "Shared libraries  --enable-shared=$enable_shared		$libtool_build_shared_libs"
echo "Static libraries  --enable-static=$enable_static		$libtool_build_static_libs"
echo "JPEG              --with-jpeg=$with_jpeg		have_jpeg = $have_jpeg"
echo "PNG               --with-png=$with_png		have_png  = $have_png"
echo "TIFF              --with-tiff=$with_tiff		have_tiff = $have_tiff"
echo "ZLIB              --with-zlib=$with_zlib		have_zlib = $have_zlib"
echo "PANO              --with-pano=$with_pano		have_pano = $have_pano"
echo ""
echo "Options used to compile and link:"
echo "  PREFIX         = $prefix"
echo "  VERSION        = $PACKAGE_VERSION"
echo "  CC             = $CC"
echo "  CFLAGS         = $CFLAGS"
echo "  CPPFLAGS       = $CPPFLAGS"
echo "  DEFS           = $DEFS"
echo "  LDFLAGS        = $LDFLAGS"
echo "  LIBS           = $LIBS"
echo "  CXX            = $CXX"
echo "  CXXFLAGS       = $CXXFLAGS"
echo "  JPEG_FLAGS     = $JPEG_FLAGS"
echo "  LIB_JPEG       = $LIB_JPEG"
echo "  PNG_FLAGS      = $PNG_FLAGS"
echo "  LIB_PNG        = $LIB_PNG"
echo "  TIFF_FLAGS     = $TIFF_FLAGS"
echo "  LIB_TIFF       = $LIB_TIFF"
echo "  ZLIB_FLAGS     = $ZLIB_FLAGS"
echo "  LIB_ZLIB       = $LIB_ZLIB"
echo "  PANO_FLAGS     = $PANO_FLAGS"
echo "  LIB_PANO       = $LIB_PANO"
echo "  WX_CXXFLAGS    = $WX_CXXFLAGS"
echo "  WX_LIBS        = $WX_LIBS"
echo "  GTK_CFLAGS     = $GTK_CFLAGS"
echo "  GTK_LIBS       = $GTK_LIBS"
echo "  FFTW_INCLUDES  = $FFTW_INCLUDES"
echo "  FFTW_LDFLAGS   = $FFTW_LDFLAGS"
echo "  BOOST_CPPFLAGS = $BOOST_CPPFLAGS"
echo "  BOOST_LIBS     = $BOOST_LIBS"
echo "  BUILD          = $SYS"
echo ""


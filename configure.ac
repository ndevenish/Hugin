#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.5)
AC_INIT([hugin], [0.4], [pablo.dangelo@web.de])
AC_CONFIG_AUX_DIR(config)
AC_CANONICAL_HOST
AC_CONFIG_HEADER([src/hugin/config.h])
AM_INIT_AUTOMAKE([no-define dist-bzip2 dist-zip])

# By default, do not create shared libraries
AC_DISABLE_SHARED

# debugging options
AC_ARG_ENABLE([debug],
              AC_HELP_STRING([--enable-debug],
	                     [enable debugging of the hugin applictions(default=no). Note: -g is removed from flags if this is not defined]),
	      [enable_debug=$enableval
	      CFLAGS="$CFLAGS -g -DDEBUG"
	      CXXFLAGS="$CXXFLAGS -g -DDEBUG"
	      ],
	      [enable_debug=no])

#create local defines in config.h.in
AH_TEMPLATE([HUGIN_VERSION],[Current Version of Hugin])
AH_TEMPLATE([INSTALL_LOCALE_DIR],[Install Location of Hugin])
AH_TEMPLATE([INSTALL_XRC_DIR],[Location for XRC information])

# I18n support
AM_GNU_GETTEXT([external])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_LIBTOOL
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_LN_S

# Checks for libraries.
AM_OPTIONS_WXCONFIG
AM_OPTIONS_GRAPHICS

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

# xrc ressources are used
WX_LIBS="${WX_LIBS} -l`$WX_CONFIG_WITH_ARGS --basename`_xrc-`$WX_CONFIG_WITH_ARGS --release`"

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

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([float.h limits.h locale.h memory.h paths.h stdlib.h string.h sys/time.h unistd.h utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_C_RESTRICT
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETVBUF_REVERSED
AC_FUNC_STRFTIME
AC_FUNC_STRTOD
AC_FUNC_VPRINTF
AC_CHECK_FUNCS([dup2 floor gettimeofday localtime_r memmove memset pow select setlocale sqrt])

AC_DEFINE_UNQUOTED(HUGIN_VERSION, $PACKAGE_VERSION)
if test "x$prefix" != xNONE; then
    AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$prefix/share/locale")
    AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$prefix/share/hugin/xrc")
else
    AC_DEFINE_UNQUOTED(INSTALL_LOCALE_DIR, "$ac_default_prefix/share/locale")
    AC_DEFINE_UNQUOTED(INSTALL_XRC_DIR, "$ac_default_prefix/share/hugin/xrc")
fi	

#
# Remove extraneous spaces from output variables (aesthetic)
#
CC=`echo $CC | sed -e 's/  */ /g'`
CFLAGS=`echo $CFLAGS | sed -e 's/  */ /g'`
CPPFLAGS=`echo $CPPFLAGS | sed -e 's/  */ /g'`
CXXFLAGS=`echo $CXXFLAGS | sed -e 's/  */ /g'`
LDFLAGS=`echo $LDFLAGS | sed -e 's/  */ /g'`
WX_CXXFLAGS=`echo $WX_CXXFLAGS | sed -e 's/  */ /g'`
WX_LIBS=`echo $WX_LIBS | sed -e 's/  */ /g'`
GTK_CFLAGS=`echo $GTK_CFLAGS | sed -e 's/  */ /g'`
GTK_LIBS=`echo $GTK_LIBS | sed -e 's/  */ /g'`

# Over-ride LIBS so that libraries are supplied by Makefiles instead
LIBS=""

AC_CONFIG_FILES([Makefile
		 src/hugin/po/Makefile.in
		 src/nona_gui/po/Makefile.in
                 src/Makefile
                 src/Panorama/Makefile
                 src/common/Makefile
                 src/foreign/Makefile
                 src/foreign/vigra_impex/Makefile
                 src/hugin/Makefile
                 src/hugin/xrc/Makefile
                 src/hugin/xrc/data/Makefile
                 src/jhead/Makefile
                 src/keypoints/Makefile
                 src/klt/Makefile
                 src/nona_gui/Makefile
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
echo "JPEG              --with-jpeg=$with_jpeg		$have_jpeg"
echo "PNG               --with-png=$with_png		$have_png"
echo "TIFF              --with-tiff=$with_tiff		$have_tiff"
echo "ZLIB              --with-zlib=$with_zlib		$have_zlib"
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
echo "  LIB_JPEG       = $LIB_JPEG"
echo "  LIB_PNG        = $LIB_PNG"
echo "  LIB_TIFF       = $LIB_TIFF"
echo "  LIB_ZLIB       = $LIB_ZLIB"
echo "  WX_CXXFLAGS    = $WX_CXXFLAGS"
echo "  WX_LIBS        = $WX_LIBS"
echo "  GTK_CFLAGS     = $GTK_CFLAGS"
echo "  GTK_LIBS       = $GTK_LIBS"
echo "  FFTW_INCLUDES  = $FFTW_INCLUDES"
echo "  FFTW_LDFLAGS   = $FFTW_LDFLAGS"
echo "  BOOST_CPPFLAGS = $BOOST_CPPFLAGS"
echo "  BOOST_LIBS     = $BOOST_LIBS"
echo ""


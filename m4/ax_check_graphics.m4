#
# Check for ZLIB
#
AC_DEFUN([AX_CHECK_ZLIB],
[
AC_ARG_WITH([zlib],
            AC_HELP_STRING([--with-zlib=PATH],
                           [Use version of zlib in PATH]),
            [with_zlib=$withval],
            [with_zlib=''])
have_zlib='no'
LIB_ZLIB=''
ZLIB_FLAGS=''
ZLIB_HOME=''
dnl PNG and TIFF require zlib so enable zlib check if PNG or TIFF is requested
if test "x$with_zlib" != 'xno' || test "x$with_png" != 'xno' || test "x$with_tiff" != 'xno' ; then
  AC_MSG_CHECKING(for ZLIB support )
  AC_MSG_RESULT()
  if test "x$with_zlib" != 'x' ; then
    if test -d "$with_zlib" ; then
      ZLIB_HOME="$with_zlib"
    else
      AC_MSG_WARN([Sorry, $with_zlib does not exist, checking usual places])
      with_zlib=''
    fi
  fi
  if test "x$ZLIB_HOME" = 'x' ; then
    zlib_dirs="/usr /usr/local /opt /mingw"
    for i in $zlib_dirs;
    do
      if test -r "$i/include/zlib.h"; then
        ZLIB_HOME="$i"
        break
      fi
    done
    if test "x$ZLIB_HOME" != 'x' ; then
      AC_MSG_NOTICE([zlib home set to $ZLIB_HOME])
    else
      AC_MSG_NOTICE([cannot find the zlib directory, assuming it is specified in CFLAGS])
    fi
  fi
  failed=0;
  passed=0;
  ZLIB_OLD_LDFLAGS=$LDFLAGS
  ZLIB_OLD_CPPFLAGS=$CPPFLAGS
  if test "x$ZLIB_HOME" != 'x' ; then
    if test "x$HCPU" = 'xamd64' ; then
      LDFLAGS="$LDFLAGS -L$ZLIB_HOME/lib64"
    else
      LDFLAGS="$LDFLAGS -L$ZLIB_HOME/lib"
    fi
    CPPFLAGS="$CPPFLAGS -I$ZLIB_HOME/include"
  fi
  AC_LANG_SAVE
  AC_LANG_C
  AC_CHECK_HEADER(zlib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_CHECK_LIB(z,inflate,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_LANG_RESTORE
  LDFLAGS="$ZLIB_OLD_LDFLAGS"
  CPPFLAGS="$ZLIB_OLD_CPPFLAGS"

  AC_MSG_CHECKING(if ZLIB package is complete)
  if test $passed -gt 0 ; then
    if test $failed -gt 0 ; then
      AC_MSG_RESULT(no -- some components failed test)
      have_zlib='no (failed tests)'
    else
      if test "x$ZLIB_HOME" = 'x' || test "x$ZLIB_HOME" = 'x/usr' ; then
        LIB_ZLIB="-lz"
        ZLIB_FLAGS="-DHasZLIB"
      else
        if test "x$HCPU" = 'xamd64' ; then
          LIB_ZLIB="-L$ZLIB_HOME/lib64 -lz"
        else
          LIB_ZLIB="-L$ZLIB_HOME/lib -lz"
        fi
        ZLIB_FLAGS="-I$ZLIB_HOME/include -DHasZLIB"
      fi
      AC_MSG_RESULT(yes)
      have_zlib='yes'
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi
AM_CONDITIONAL(HasZLIB, test "x$have_zlib" = 'xyes')
AC_SUBST(LIB_ZLIB)
AC_SUBST(ZLIB_FLAGS)
])

#
# Check for PNG
#
AC_DEFUN([AX_CHECK_PNG],
[
AC_ARG_WITH([png],
            AC_HELP_STRING([--with-png=PATH],
                           [Use version of png in PATH]),
            [with_png=$withval],
            [with_png=''])
have_png='no'
LIB_PNG=''
PNG_FLAGS=''
PNG_HOME=''
if test "x$with_png" != 'xno' ; then
  AC_MSG_CHECKING(for PNG support )
  AC_MSG_RESULT()
  if test "x$with_png" != 'x' ; then
    if test -d "$with_png" ; then
      PNG_HOME="$with_png"
    else
      AC_MSG_WARN([Sorry, $with_png does not exist, checking usual places])
      with_png=''
    fi
  fi
  if test "x$PNG_HOME" = 'x' ; then
    png_dirs="/usr /usr/local /opt /mingw"
    for i in $png_dirs;
    do
      if test -r "$i/include/png.h"; then
        PNG_HOME="$i"
        break
      fi
    done
    if test "x$PNG_HOME" != 'x' ; then
      AC_MSG_NOTICE([png home set to $PNG_HOME])
    else
      AC_MSG_NOTICE([cannot find the png directory, assuming it is specified in CFLAGS])
    fi
  fi
  failed=0;
  passed=0;
  PNG_OLD_LDFLAGS=$LDFLAGS
  PNG_OLD_CPPFLAGS=$CPPFLAGS
  if test "x$PNG_HOME" != 'x' ; then
    if test "x$HCPU" = 'xamd64' ; then
      LDFLAGS="$LDFLAGS -L$PNG_HOME/lib64"
    else
      LDFLAGS="$LDFLAGS -L$PNG_HOME/lib"
    fi
    CPPFLAGS="$CPPFLAGS -I$PNG_HOME/include"
  fi
  AC_LANG_SAVE
  AC_LANG_C
  AC_CHECK_HEADER(png.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_CHECK_LIB([png],[png_get_io_ptr],passed=`expr $passed + 1`,failed=`expr $failed + 1`,[-lz -lm])
  AC_LANG_RESTORE
  LDFLAGS="$PNG_OLD_LDFLAGS"
  CPPFLAGS="$PNG_OLD_CPPFLAGS"

  AC_MSG_CHECKING(if PNG package is complete)
  if test $passed -gt 0 ; then
    if test $failed -gt 0 ; then
      AC_MSG_RESULT(no -- some components failed test)
      have_png='no (failed tests)'
    else
      if test "x$PNG_HOME" = 'x' || test "x$PNG_HOME" = 'x/usr' ; then
        LIB_PNG="-lpng"
        PNG_FLAGS="-DHasPNG"
      else
        if test "x$HCPU" = 'xamd64' ; then
          LIB_PNG="-L$PNG_HOME/lib64 -lpng"
        else
          LIB_PNG="-L$PNG_HOME/lib -lpng"
        fi
        PNG_FLAGS="-I$PNG_HOME/include -DHasPNG"
      fi
      AC_DEFINE(HasPNG,1,Define if you have PNG library)
      AC_MSG_RESULT(yes)
      have_png='yes'
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi
AM_CONDITIONAL(HasPNG, test "x$have_png" = 'xyes')
AC_SUBST(LIB_PNG)
AC_SUBST(PNG_FLAGS)
])

#
# Check for JPEG
#
AC_DEFUN([AX_CHECK_JPEG],
[
AC_ARG_WITH([jpeg],
            AC_HELP_STRING([--with-jpeg=PATH],
                           [Use version of jpeg in PATH]),
            [with_jpeg=$withval],
            [with_jpeg=''])
have_jpeg='no'
LIB_JPEG=''
JPEG_FLAGS=''
JPEG_HOME=''
dnl TIFF requires jpeg so enable jpeg check if TIFF is requested
if test "x$with_jpeg" != 'xno' || test "x$with_tiff" != 'xno'  ; then
  AC_MSG_CHECKING(for JPEG support )
  AC_MSG_RESULT()
  if test "x$with_jpeg" != 'x' ; then
    if test -d "$with_jpeg" ; then
      JPEG_HOME="$with_jpeg"
    else
      AC_MSG_WARN([Sorry, $with_jpeg does not exist, checking usual places])
      with_jpeg=''
    fi
  fi
  if test "x$JPEG_HOME" = 'x' ; then
    jpeg_dirs="/usr /usr/local /opt /mingw"
    for i in $jpeg_dirs;
    do
      if test -r "$i/include/jpeglib.h"; then
        JPEG_HOME="$i"
        break
      fi
    done
    if test "x$JPEG_HOME" != 'x' ; then
      AC_MSG_NOTICE([jpeg home set to $JPEG_HOME])
    else
      AC_MSG_NOTICE([cannot find the jpeg directory, assuming it is specified in CFLAGS])
    fi
  fi
  failed=0;
  passed=0;
  JPEG_OLD_LDFLAGS=$LDFLAGS
  JPEG_OLD_CPPFLAGS=$CPPFLAGS
  if test "x$JPEG_HOME" != 'x' ; then
    if test "x$HCPU" = 'xamd64' ; then
      LDFLAGS="$LDFLAGS -L$JPEG_HOME/lib64"
    else
      LDFLAGS="$LDFLAGS -L$JPEG_HOME/lib"
    fi
    CPPFLAGS="$CPPFLAGS -I$JPEG_HOME/include"
  fi
  AC_LANG_SAVE
  AC_LANG_C
  AC_CHECK_HEADER(jpeglib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_CHECK_LIB(jpeg,jpeg_read_header,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)

# Test for compatible JPEG library
  if test "x$ac_cv_jpeg_version_ok" != 'xyes' ; then
    AC_CACHE_CHECK(for JPEG library is version 6b or later, ac_cv_jpeg_version_ok,
    [AC_TRY_COMPILE(
    #include <stdio.h>
    #include <stdlib.h>
    #include <jpeglib.h>
    ,
    changequote(<<, >>)dnl
    <<
    #if JPEG_LIB_VERSION < 62
    #error IJG JPEG library must be version 6b or newer! 
    #endif
    return 0;
    >>,
    changequote([, ])dnl
    ac_cv_jpeg_version_ok='yes',
    ac_cv_jpeg_version_ok='no')])
    if test "x$ac_cv_jpeg_version_ok" = 'xyes' ; then  
      passed=`expr $passed + 1`
    else
      failed=`expr $failed + 1`
    fi
  fi
  AC_LANG_RESTORE
  LDFLAGS="$JPEG_OLD_LDFLAGS"
  CPPFLAGS="$JPEG_OLD_CPPFLAGS"

  AC_MSG_CHECKING(if JPEG package is complete)
  if test $passed -gt 0 ; then
    if test $failed -gt 0 ; then
      AC_MSG_RESULT(no -- some components failed test)
      have_jpeg='no (failed tests)'
    else
      if test "x$JPEG_HOME" = 'x' || test "x$JPEG_HOME" = 'x/usr' ; then
        LIB_JPEG="-ljpeg"
        JPEG_FLAGS="-DHasJPEG"
      else
        if test "x$HCPU" = 'xamd64' ; then
          LIB_JPEG="-L$JPEG_HOME/lib64 -ljpeg"
        else
          LIB_JPEG="-L$JPEG_HOME/lib -ljpeg"
        fi
        JPEG_FLAGS="-I$JPEG_HOME/include -DHasJPEG"
      fi
      AC_DEFINE(HasJPEG,1,Define if you have JPEG library)
      AC_MSG_RESULT(yes)
      have_jpeg='yes'
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi
AM_CONDITIONAL(HasJPEG, test "x$have_jpeg" = 'xyes')
AC_SUBST(LIB_JPEG)
AC_SUBST(JPEG_FLAGS)
])

#
# Check for TIFF
#
AC_DEFUN([AX_CHECK_TIFF],
[
AC_ARG_WITH([tiff],
            AC_HELP_STRING([--with-tiff=PATH],
                           [Use version of TIFF in PATH]),
            [with_tiff=$withval],
			[with_tiff=''])
have_tiff='no'
LIB_TIFF=''
TIFF_FLAGS=''
TIFF_HOME=''
if test "x$with_tiff" != 'xno' ; then
  AC_MSG_CHECKING(for TIFF support )
  AC_MSG_RESULT()
  if test "x$with_tiff" != 'x' ; then
    if test -d "$with_tiff" ; then
      TIFF_HOME="$with_tiff"
    else
      AC_MSG_WARN([Sorry, $with_tiff does not exist, checking usual places])
      with_tiff=''
    fi
  fi
  if test "x$TIFF_HOME" = 'x' ; then
    tiff_dirs="/usr /usr/local /opt /mingw"
    for i in $tiff_dirs;
    do
      if test -r "$i/include/tiff.h"; then
        TIFF_HOME="$i"
        break
      fi
    done
    if test "x$TIFF_HOME" != 'x' ; then
      AC_MSG_NOTICE([tiff home set to $TIFF_HOME])
    else
      AC_MSG_NOTICE([cannot find the tiff directory, assuming it is specified in CFLAGS])
    fi
  fi
  failed=0;
  passed=0;
  TIFF_OLD_LDFLAGS=$LDFLAGS
  TIFF_OLD_CPPFLAGS=$CPPFLAGS
  if test "x$TIFF_HOME" != 'x' ; then
    if test "x$HCPU" = 'xamd64' ; then
      LDFLAGS="$LDFLAGS -L$TIFF_HOME/lib64"
    else
      LDFLAGS="$LDFLAGS -L$TIFF_HOME/lib"
    fi
    CPPFLAGS="$CPPFLAGS -I$TIFF_HOME/include"
  fi    
  AC_LANG_SAVE
  AC_LANG_C
  AC_CHECK_HEADER(tiff.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_CHECK_LIB(tiff,TIFFOpen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,[-lz -ljpeg -lm])
  AC_LANG_RESTORE
  LDFLAGS="$TIFF_OLD_LDFLAGS"
  CPPFLAGS="$TIFF_OLD_CPPFLAGS"

  AC_MSG_CHECKING(if TIFF package is complete)
  if test $passed -gt 0 ; then
    if test $failed -gt 0 ; then
      AC_MSG_RESULT(no -- some components failed test)
      have_tiff='no (failed tests)'
    else
      if test "x$TIFF_HOME" = 'x' || test "x$TIFF_HOME" = 'x/usr' ; then
        LIB_TIFF="-ltiff"
        TIFF_FLAGS="-DHasTIFF"
      else
        if test "x$HCPU" = 'xamd64' ; then
          LIB_TIFF="-L$TIFF_HOME/lib64 -ltiff"
        else
          LIB_TIFF="-L$TIFF_HOME/lib -ltiff"
        fi
        TIFF_FLAGS="-I$TIFF_HOME/include -DHasTIFF"
      fi
      AC_DEFINE(HasTIFF,1,Define if you have TIFF library)
      AC_MSG_RESULT(yes)
      have_tiff='yes'
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi
AM_CONDITIONAL(HasTIFF, test "x$have_tiff" = 'xyes')
AC_SUBST(LIB_TIFF)
AC_SUBST(TIFF_FLAGS)
])

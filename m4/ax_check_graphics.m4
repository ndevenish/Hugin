dnl ---------------------------------------------------------------------------
dnl AM_OPTIONS_GRAPHICS
dnl
dnl adds support for --with-jpeg --with-png --with-tiff and --with-zlib
dnl command line options
dnl ---------------------------------------------------------------------------

AC_DEFUN([AM_OPTIONS_GRAPHICS],
[
    AC_ARG_WITH([jpeg],
                AC_HELP_STRING([--with-jpeg=PATH],
		               [Use version of jpeg in PATH]),
                [with_jpeg=$withval],
		[with_jpeg=''])
    AC_ARG_WITH([jpeg],
                AC_HELP_STRING([--with-png=PATH],
		               [Use version of png in PATH]),
                [with_png=$withval],
		[with_png=''])
    AC_ARG_WITH([tiff],
                AC_HELP_STRING([--with-tiff=PATH],
		               [Use version of TIFF in PATH]),
                [with_tiff=$withval],
		[with_tiff=''])
    AC_ARG_WITH([zlib],
                AC_HELP_STRING([--with-zlib=PATH],
		               [Use version of zlib in PATH]),
                [with_zlib=$withval],
		[with_zlib=''])
])

#
# Check for ZLIB
#
AC_DEFUN([AX_CHECK_ZLIB],
[
have_zlib='no'
LIB_ZLIB=''
ZLIB_FLAG=''
dnl PNG requires zlib so enable zlib check if PNG is requested
if test "$with_zlib" != 'no' || test "$with_png" != 'no'
then
  AC_MSG_CHECKING(for ZLIB support )
  AC_MSG_RESULT()
  failed=0;
  passed=0;
  AC_CHECK_HEADER(zconf.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
  AC_CHECK_HEADER(zlib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
dnl  AC_CHECK_LIB(z,compress,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
dnl  AC_CHECK_LIB(z,uncompress,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_CHECK_LIB(z,deflate,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_CHECK_LIB(z,inflate,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
dnl  AC_CHECK_LIB(z,gzseek,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
dnl  AC_CHECK_LIB(z,gztell,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
  AC_MSG_CHECKING(if ZLIB package is complete)
  if test $passed -gt 0
  then
    if test $failed -gt 0
    then
      AC_MSG_RESULT(no -- some components failed test)
      have_zlib='no (failed tests)'
    else
      LIB_ZLIB='-lz'
      ZLIB_FLAG='-DHasZLIB'
      AC_MSG_RESULT(yes)
      have_zlib='yes'
    fi
  else
    AC_MSG_RESULT(no)
  fi
fi
AM_CONDITIONAL(HasZLIB, test "$have_zlib" = 'yes')
AC_SUBST(LIB_ZLIB)
AC_SUBST(ZLIB_FLAG)
])

#
# Check for PNG
#
AC_DEFUN([AX_CHECK_PNG],
[
have_png='no'
LIB_PNG=''
PNG_FLAG=''
if test "$with_png" != 'no'
then
    AC_MSG_CHECKING(for PNG support )
    AC_MSG_RESULT()
    failed=0;
    passed=0;
    AC_CHECK_HEADER(png.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(png,png_get_io_ptr,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING(if PNG package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
  AC_MSG_RESULT(no -- some components failed test)
  have_png='no (failed tests)'
    else
  LIB_PNG='-lpng'
  PNG_FLAG='-DHasPNG'
  AC_DEFINE(HasPNG,1,Define if you have PNG library)
  AC_MSG_RESULT(yes)
  have_png='yes'
    fi
    else
    AC_MSG_RESULT(no)
    fi
fi
AM_CONDITIONAL(HasPNG, test "$have_png" = 'yes')
AC_SUBST(LIB_PNG)
AC_SUBST(PNG_FLAG)
])

#
# Check for JPEG
#
AC_DEFUN([AX_CHECK_JPEG],
[
have_jpeg='no'
LIB_JPEG=''
JPEG_FLAG=''
if test "$with_jpeg" != 'no'
then
    AC_MSG_CHECKING(for JPEG support )
    AC_MSG_RESULT()
    failed=0;
    passed=0;
    AC_CHECK_HEADER(jconfig.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(jerror.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(jmorecfg.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(jpeglib.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(jpeg,jpeg_read_header,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)

# Test for compatible JPEG library
if test "$ac_cv_jpeg_version_ok" != 'yes' ; then
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
  if test "$ac_cv_jpeg_version_ok" = 'yes' ; then
    passed=`expr $passed + 1`
  else
    failed=`expr $failed + 1`
  fi
fi
AC_MSG_CHECKING(if JPEG package is complete)
if test $passed -gt 0
then
  if test $failed -gt 0
  then
    AC_MSG_RESULT(no -- some components failed test)
    have_jpeg='no (failed tests)'
  else
    LIB_JPEG='-ljpeg'
    JPEG_FLAG='-DHasJPEG'
    AC_DEFINE(HasJPEG,1,Define if you have JPEG library)
    AC_MSG_RESULT(yes)
    have_jpeg='yes'
  fi
else
    AC_MSG_RESULT(no)
fi
fi
AM_CONDITIONAL(HasJPEG, test "$have_jpeg" = 'yes')
AC_SUBST(LIB_JPEG)
AC_SUBST(JPEG_FLAG)
])

#
# Check for TIFF
#
AC_DEFUN([AX_CHECK_TIFF],
[
have_tiff='no'
LIB_TIFF=''
TIFF_FLAG=''
if test "$with_tiff" != 'no'
then
    AC_MSG_CHECKING(for TIFF support )
    AC_MSG_RESULT()
    failed=0;
    passed=0;
    AC_CHECK_HEADER(tiff.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_HEADER(tiffio.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`)
    AC_CHECK_LIB(tiff,TIFFOpen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
dnl    AC_CHECK_LIB(tiff,TIFFClientOpen,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
dnl    AC_CHECK_LIB(tiff,TIFFIsByteSwapped,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
dnl    AC_CHECK_LIB(tiff,TIFFReadRGBATile,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
dnl    AC_CHECK_LIB(tiff,TIFFReadRGBAStrip,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING(if TIFF package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
  AC_MSG_RESULT(no -- some components failed test)
  have_tiff='no (failed tests)'
    else
  LIB_TIFF='-ltiff'
  TIFF_FLAG='-DHasTIFF'
  AC_DEFINE(HasTIFF,1,Define if you have TIFF library)
  AC_MSG_RESULT(yes)
  have_tiff='yes'
  AC_CHECK_HEADERS(tiffconf.h)
    fi
    else
    AC_MSG_RESULT(no)
    fi
fi
AM_CONDITIONAL(HasTIFF, test "$have_tiff" = 'yes')
AC_SUBST(LIB_TIFF)
AC_SUBST(TIFF_FLAG)
])

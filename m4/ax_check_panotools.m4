#
# Check for PANO
#
AC_DEFUN([AX_CHECK_PANO],
[
have_pano='no'
LIB_PANO=''
PANO_FLAG=''
if test "$with_pano" != 'no'
then
    AC_MSG_CHECKING(for PanoTools support )
    AC_MSG_RESULT()
    failed=0;
    passed=0;
    AC_CHECK_HEADER(pano12/panorama.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(pano12,fcnPano,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_MSG_CHECKING(if Panotools package is complete)
    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
  AC_MSG_RESULT(no -- some components failed test)
  have_pano='no (failed tests)'
    else
  LIB_PANO='-lpano12'
  PANO_FLAG='-DHasPANO'
  AC_DEFINE(HasPANO,1,Define if you have Panotools library (pano12))
  AC_MSG_RESULT(yes)
  have_pano='yes'
    fi
    else
    AC_MSG_RESULT(no)
    fi
fi
AM_CONDITIONAL(HasPANO, test "$have_pano" = 'yes')
AC_SUBST(LIB_PANO)
AC_SUBST(PANO_FLAG)
])


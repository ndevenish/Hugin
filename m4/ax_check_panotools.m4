#
# Check for PANO
#

AC_DEFUN([AX_CHECK_PANO],
[
AC_ARG_WITH(pano,
            AC_HELP_STRING([--with-pano=PATH],
                           [Use version of pano in PATH]),
            [with_pano=$withval],
            [with_pano=''])

have_pano='no'
LIB_PANO=''
PANO_FLAGS=''
PANO_HOME=''
if test "x$with_pano" != 'xno'
then
    AC_MSG_CHECKING(for PanoTools support )
    AC_MSG_RESULT()
    if test "x$with_pano" != 'x'
    then
      if test -d "$with_pano"
      then
        PANO_HOME="$with_pano"
      else
        AC_MSG_WARN([Sorry, $with_pano does not exist, checking usual places])
	with_pano=''
      fi
    fi
    if test "x$PANO_HOME" = 'x'
    then
      pano_dirs="/usr /usr/local /opt"
      for i in $pano_dirs;
      do
        if test -r "$i/include/pano12/panorama.h"; then
          PANO_HOME="$i"
	  break
	fi
      done
      if test "x$PANO_HOME" != 'x'
      then
	AC_MSG_NOTICE([pano home set to $PANO_HOME])
      else
        AC_MSG_ERROR([cannot find the panorama tools directory],[1])
      fi
    fi
    failed=0;
    passed=0;
    PANO_OLD_LDFLAGS=$LDFLAGS
    PANO_OLD_CPPFLAGS=$LDFLAGS
    LDFLAGS="$LDFLAGS -L$PANO_HOME/lib"
    CPPFLAGS="$CPPFLAGS -I$PANO_HOME/include"
    AC_LANG_SAVE
    AC_LANG_C
    AC_CHECK_HEADER(pano12/panorama.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_CHECK_LIB(pano12,fcnPano,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    AC_LANG_RESTORE
    LDFLAGS="$PANO_OLD_LDFLAGS"
    CPPFLAGS="$PANO_OLD_CPPFLAGS"

    AC_MSG_CHECKING(if Panotools package is complete)

    if test $passed -gt 0
    then
    if test $failed -gt 0
    then
      AC_MSG_RESULT(no -- some components failed test)
      have_pano='no (failed tests)'
    else
      if test "x$with_pano" = 'x'
      then
        LIB_PANO="-lpano12"
        PANO_FLAGS="-DHasPANO"
      else
        LIB_PANO="-L$PANO_HOME/lib -lpano12"
        PANO_FLAGS="-I$PANO_HOME/include -DHasPANO"
      fi
      AC_DEFINE(HasPANO,1,Define if you have Panotools library (pano12))
      AC_MSG_RESULT(yes)
      have_pano='yes'
    fi
    else
      AC_MSG_RESULT(no)
    fi
fi
AM_CONDITIONAL(HasPANO, test "x$have_pano" = 'xyes')
AC_SUBST(LIB_PANO)
AC_SUBST(PANO_FLAGS)
])



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

have_pano13='no'
have_pano='no'
LIB_PANO=''
PANO_FLAGS=''
PANO_HOME=''
if test "x$with_pano" != 'xno' ; then
    AC_MSG_CHECKING(for PanoTools support )
    AC_MSG_RESULT()
    if test "x$with_pano" != 'x' ; then
      if test -d "$with_pano" ; then
        PANO_HOME="$with_pano"
      else
        AC_MSG_WARN([Sorry, $with_pano does not exist, checking usual places])
	with_pano=''
      fi
    fi
    if test "x$PANO_HOME" = 'x' ; then
      pano_dirs="/usr /usr/local /opt /mingw"
      for i in $pano_dirs;
      do
        if test -r "$i/include/pano13/panorama.h"; then
          PANO_HOME="$i"
          have_pano13='yes'
          break
        fi
      done
      if test "$have_pano13" = 'no' ; then
        for i in $pano_dirs;
        do
          if test -r "$i/include/pano12/panorama.h"; then
            PANO_HOME="$i"
            echo "havpano12 yes"
            break
          fi
        done
      fi

      if test "x$PANO_HOME" != 'x' ; then
        AC_MSG_NOTICE([pano home set to $PANO_HOME])
      else
        AC_MSG_NOTICE([cannot find the libpano13 directory, assuming it is specified in CFLAGS])
      fi
    fi
    failed=0;
    passed=0;
    PANO_OLD_LDFLAGS=$LDFLAGS
    PANO_OLD_CPPFLAGS=$CPPFLAGS
    if test "x$HCPU" = 'xamd64' ; then
      LDFLAGS="$LDFLAGS -L$PANO_HOME/lib64"
    else
      LDFLAGS="$LDFLAGS -L$PANO_HOME/lib"
    fi
    CPPFLAGS="$CPPFLAGS -I$PANO_HOME/include"
    AC_LANG_SAVE
    AC_LANG_C
    echo "have_pano13: $have_pano13"
    if test "x$have_pano13" = 'xyes' ; then
      AC_CHECK_HEADER(pano13/panorama.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
      AC_CHECK_HEADERS(pano13/queryfeature.h,AC_MSG_RESULT(panotools query functions enabled),AC_MSG_RESULT(panotools query functions disabled),)
      AC_CHECK_LIB(pano13,fcnPano,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    else
      AC_CHECK_HEADER(pano12/panorama.h,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
      AC_CHECK_HEADERS(pano12/queryfeature.h,AC_MSG_RESULT(panotools query functions enabled),AC_MSG_RESULT(panotools query functions disabled),)
      AC_CHECK_LIB(pano12,fcnPano,passed=`expr $passed + 1`,failed=`expr $failed + 1`,)
    fi
    AC_LANG_RESTORE
    LDFLAGS="$PANO_OLD_LDFLAGS"
    CPPFLAGS="$PANO_OLD_CPPFLAGS"

    AC_MSG_CHECKING(if Panotools package is complete)

    if test $passed -gt 0 ; then
      if test $failed -gt 0 ; then
        AC_MSG_RESULT(no -- some components failed test)
        have_pano='no (failed tests)'
      else
        if test "x$PANO_HOME" = 'x' || test "x$PANO_HOME" = 'x/usr' ; then
          if test "$have_pano13" = 'yes' ; then
            LIB_PANO="-lpano13"
            PANO_FLAGS="-DHasPANO13"
          else
            LIB_PANO="-lpano12"
            PANO_FLAGS="-DHasPANO12"
          fi
        else
          if test "x$HCPU" = 'xamd64' ; then
            if test "$have_pano13" = 'yes' ; then
              LIB_PANO="-L$PANO_HOME/lib64 -lpano13"
            else
              LIB_PANO="-L$PANO_HOME/lib64 -lpano12"
            fi
          else
            if test "$have_pano13" = 'yes' ; then
              LIB_PANO="-L$PANO_HOME/lib -lpano13"
            else
              LIB_PANO="-L$PANO_HOME/lib -lpano12"
            fi
          fi
          if test "$have_pano13" = 'yes' ; then
            PANO_FLAGS="-I$PANO_HOME/include -DHasPANO13"
          else
            PANO_FLAGS="-I$PANO_HOME/include -DHasPANO12"
          fi
        fi
        AC_DEFINE(HasPANO,1,Define if you have Panotools library)
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



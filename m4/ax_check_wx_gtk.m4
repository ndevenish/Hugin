dnl ----------------------------------------------------
dnl CHECK_WX_BUILT_WITH_GTK
dnl check gtk version wx windows was compiled with
dnl check GTK2 first sa it is more common
dnl ----------------------------------------------------

AC_DEFUN([CHECK_WX_BUILT_WITH_GTK2],
[
  GTK_USEDVERSION=''
  AC_MSG_CHECKING(if wxWindows was linked with GTK2)
  if $WX_CONFIG_PATH --libs | grep -E '_gtk2u?d?[[-_]]' > /dev/null ; then
     GTK_USEDVERSION=2
     AC_MSG_RESULT(yes)
  else
     AC_MSG_RESULT(no)
     AC_MSG_CHECKING(if wxWindows was linked with GTK)
     if $WX_CONFIG_PATH --libs | grep -E '_gtku?d?[[-_]]' > /dev/null ; then
       GTK_USEDVERSION=1
       AC_MSG_RESULT(yes)
     else
       AC_MSG_RESULT(no)
     fi
  fi
  AC_SUBST(GTK_USEDVERSION)
])


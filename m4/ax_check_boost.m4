#
# check for boost presence
# default is version 1.31
#
AC_DEFUN([AX_CHECK_BOOST],
[
 AC_ARG_WITH([boost-version],
             AC_HELP_STRING([--with-boost-version],
			                [tell which boost version to use (default: 1.31)]),
 [
  BOOST_VERSION="$withval"
  BOOST_VERSION_=`echo $withval | sed "s/\./_/g"`
  BOOST_VERSION_="-${BOOST_VERSION_}"
 ],
 [
  BOOST_VERSION="1.31"
  BOOST_VERSION_="-1_31"
 ]) # end AC_ARG_WITH

 AC_MSG_CHECKING([boost version])
 AC_MSG_RESULT($BOOST_VERSION)

 ac_cppflags_save="${CPPFLAGS}"
 ac_ldflags_save="${LDFLAGS}"
 
 #
 # --with-boost to be used when using boost from release or CVS
 #
 AC_ARG_WITH([boost],
 AC_HELP_STRING([--with-boost],[tell where boost resides]),
 [
  ac_boost_includedir="$withval/include/boost$BOOST_VERSION_"
  CPPFLAGS="-I$ac_boost_includedir ${CPPFLAGS}"
  ac_boost_libdir="$withval/lib/"
  LDFLAGS="${LDFLAGS} -L${ac_boost_libdir}"
 ],
 [
########################################################
  AC_MSG_CHECKING([Boost headers files presence])
  ac_boost_includedirs="/usr/include /usr/local/include /opt/include /mingw/include /usr/include/boost$BOOST_VERSION_ /usr/local/include/boost$BOOST_VERSION_ /opt/include/boost$BOOST_VERSION_ /mingw/include/boost$BOOST_VERSION_"
  AC_FIND_FILE("boost/any.hpp",$ac_boost_includedirs , ac_boost_includedir)
  AC_MSG_RESULT([$ac_boost_includedir])
  if ! test $ac_boost_includedir = "NO"; then
   CPPFLAGS="-I$ac_boost_includedir ${CPPFLAGS}"
  fi

  AC_MSG_CHECKING([Boost libraries presence])
  ac_boost_libdirs="/usr/lib /usr/local/lib /opt/lib /mingw/lib"
  AC_FIND_FILE("libboost_regex-gcc.a", $ac_boost_libdirs, ac_boost_libdir)
  # echo $ac_boost_libdir
  if ! test $ac_boost_libdir = "NO"; then
   LDFLAGS="${LDFLAGS} -L${ac_boost_libdir}"
  else
    ac_boost_libdir=/usr/lib
  fi
  AC_MSG_RESULT([$ac_boost_libdir])
 ]) # end AC_ARG_WITH

  AC_LANG_SAVE
  AC_LANG_CPLUSPLUS
  AC_MSG_CHECKING([for boost])
  AC_TRY_COMPILE([
   #include <boost/any.hpp>
  ], [
   boost::any __any = 1;
   return 0;
  ],
  [
   AC_MSG_RESULT(yes)
   have_boost=yes
  ],
  [
   AC_MSG_RESULT(no)
   have_boost=no
  ])
  AC_LANG_RESTORE
  CPPFLAGS="$ac_cppflags_save"
  LDFLAGS="$ac_ldflags_save"
  if test x$have_boost = xyes; then
   AC_DEFINE(HAVE_BOOST, 1, [have boost])
   if test "x$ac_boost_libdir" = 'x/usr/lib' || test "x$ac_boost_libdir" = 'x/usr/lib64' ; then 
    BOOST_LIBS=""
   else
    BOOST_LIBS="-L${ac_boost_libdir}"
   fi
   if test "x$ac_boost_includedir" = 'x/usr/include' ; then 
	 BOOST_CFLAGS=""
   else
	 BOOST_CFLAGS="-I${ac_boost_includedir}"
   fi
   boost_error=no
  else
   boost_error=yes
  fi
 AC_SUBST(BOOST_LIBS)
 AC_SUBST(BOOST_CFLAGS)
 AC_SUBST(BOOST_VERSION)
 AC_PROVIDE([AX_CHECK_BOOST([$BOOST_VERSION])])
])

#
# check boost library variants
#
AC_DEFUN([AX_CHECK_BOOST_LIBVARIANT],
[
 AC_REQUIRE([AX_CHECK_BOOST])
 
 boost_lib=$1
 AC_MSG_CHECKING([for $boost_lib library variant presence])
 boost_compiler="-gcc"
 boost_mt="-mt"
 if test "$enable_debug" = "yes"; then
  boost_debug="-d"
 fi

 if test -f ${ac_boost_libdir}/lib$boost_lib${boost_compiler}${boost_mt}${boost_debug}${BOOST_VERSION_}.so; then

  boost_libvariant="${boost_compiler}${boost_mt}${boost_debug}${BOOST_VERSION_}"

 elif test -f ${ac_boost_libdir}/lib$boost_lib${boost_compiler}${boost_debug}${BOOST_VERSION_}.so; then

  boost_libvariant="${boost_compiler}${boost_debug}${BOOST_VERSION_}"

 elif test -f ${ac_boost_libdir}/lib$boost_lib${boost_compiler}${BOOST_VERSION_}.so; then

  boost_libvariant="${boost_compiler}${BOOST_VERSION_}"

 elif test -f ${ac_boost_libdir}/lib$boost_lib${boost_compiler}.so; then

  boost_libvariant="${boost_compiler}"

 elif test -f ${ac_boost_libdir}/lib$boost_lib.so; then

  boost_libvariant=

 else
  
  boost_lib=no 
 
 fi
 if test "x$boost_lib" != "xno"; then
  AC_MSG_RESULT([${ac_boost_libdir}/lib$boost_lib$boost_libvariant.so])
 else
  AC_MSG_RESULT([no])
 fi
])

#
# Check for boost test library 
#
AC_DEFUN([AX_CHECK_BOOST_TEST],
[
 AC_REQUIRE([AX_CHECK_BOOST])

 ac_cppflags_save="${CPPFLAGS}"
 ac_ldflags_save="${LDFLAGS}"
 
 CPPFLAGS="${BOOST_CFLAGS} ${CPPFLAGS}"
 LDFLAGS="${LDFLAGS} ${BOOST_LIBS}"

 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS

 AC_CHECK_HEADERS([boost/test/test_tools.hpp])

  AC_MSG_CHECKING(for boost unit test framework library)
  AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[
#include <iostream>                  // for std::cout
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <boost/test/test_tools.hpp>
]],[[
    BOOST_CHECK( 1==1);
   ]]),
  [
   AC_MSG_RESULT(yes)
   ac_have_boost_test=yes
  ],
  [
   AC_MSG_RESULT(no)
   ac_have_boost_test=no
  ])
  AC_LANG_RESTORE

  CPPFLAGS="$ac_cppflags_save"
  LDFLAGS="$ac_ldflags_save"

  AX_CHECK_BOOST_LIBVARIANT([boost_unit_test_framework])
  if test x$ac_have_boost_test = xyes -a x$boost_lib != xno; then
   AC_DEFINE(HAVE_BOOST_TEST, 1, [have boost unit test framework library])
#   BOOST_LIBS="-lboost_unit_test_framework$boost_libvariant ${BOOST_LIBS}"
    BOOST_TEST_LIB="-lboost_unit_test_framework$boost_libvariant"
   boost_test_error=no
  else
   boost_test_error=yes
  fi
  AC_SUBST(BOOST_TEST_LIB)
])

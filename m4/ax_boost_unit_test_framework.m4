dnl Available from the GNU Autoconf Macro Archive at:
dnl http://www.gnu.org/software/ac-archive/htmldoc/ax_boost_unit_test_framework.html
dnl
AC_DEFUN([AX_BOOST_UNIT_TEST_FRAMEWORK],
[AC_REQUIRE([AC_CXX_NAMESPACES])dnl
AC_CACHE_CHECK(whether the boost::unit_test_framework library is available,
ax_cv_boost_unit_test_framework,
[AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[#include <boost/test/unit_test.hpp>]],
			           [[BOOST_TEST_SUITE("boost"); return 0;]]),
  	           ax_cv_boost_unit_test_framework=yes, ax_cv_boost_unit_test_framework=no)
 AC_LANG_RESTORE
])
if test "$ax_cv_boost_unit_test_framework" = yes; then
  AC_DEFINE(HAVE_BOOST_UNIT_TEST_FRAMEWORK,,[define if the boost::unit_test_framework library is available])
  dnl Now determine the appropriate file names
  AC_ARG_WITH([boost-unit_test],
              AC_HELP_STRING([--with-boost-unit_test_framework],
                             [specify the boost unit test framework library or suffix to use]),
  [if test "x$with_boost_unit_test_framework" != "xno"; then
    ax_unit_test_framework_lib=$with_boost_unit_test_framework
    ax_boost_unit_test_framework_lib=boost_unit_test_framework-$with_boost_unit_test_framework
  fi])
  for ax_lib in $ax_unit_test_framework_lib $ax_boost_unit_test_framework_lib boost_unit_test_framework; do
    AC_CHECK_LIB($ax_lib, init_unit_test_suite, [BOOST_UNIT_TEST_FRAMEWORK_LIB=$ax_lib break])
  done
  AC_SUBST(BOOST_UNIT_TEST_FRAMEWORK_LIB)
fi
])dnl


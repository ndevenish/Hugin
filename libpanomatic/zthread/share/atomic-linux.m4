dnl Copyright (c) 2005, Eric Crahen
dnl
dnl Permission is hereby granted, free of charge, to any person obtaining a copy
dnl of this software and associated documentation files (the "Software"), to deal
dnl in the Software without restriction, including without limitation the rights
dnl to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
dnl copies of the Software, and to permit persons to whom the Software is furnished
dnl to do so, subject to the following conditions:
dnl 
dnl The above copyright notice and this permission notice shall be included in all
dnl copies or substantial portions of the Software.
dnl 
dnl THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
dnl IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
dnl FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
dnl AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
dnl WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
dnl CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

dnl
dnl Enables AM_ENABLE_ATOMIC_LINUX to test for a working 
dnl <asm/atomic.h> header. Not all linux distributions
dnl seem to include one that exposes atomic_inc() & 
dnl atomic_dec_and_test()
dnl
dnl --enable-atomic-linux=yes|no [default=no]
dnl --with-atomic-linux-include=kernel header path [optional]
dnl 
dnl If support is available, then HAVE_ATOMIC_LINUX 
dnl will be set and CXXFLAGS will be updated
dnl
ifdef([AM_ENABLE_ATOMIC_LINUX],,[

ATOMIC_LINUX_ERROR=<<"EOF"
This system does not contain an <asm/atomic.h> kernel header 
that exposes atomic the neccessary atomic functions. Not all
Linux distributions do.
EOF

atomic_linux_explicit="no"

AC_DEFUN([AM_ENABLE_ATOMIC_LINUX],
[AC_ARG_ENABLE(atomic-linux,
	AC_HELP_STRING([--enable-atomic-linux],
		       [use linux atomic instructions [default=no]]),
 [atomic_linux_explicit="yes"], 
 [enable_atomic_linux="no"])

 AC_ARG_WITH(atomic-linux-include,
	AC_HELP_STRING([--with-atmoic-linux-include],
		       [path to kernel headers (optional)]),
 [atomic_prefix="$withval"
  enable_atomic_linux="yes"
  atomic_linux_explicit="yes"
 ], 
 [atomic_prefix=""])

 if test $enable_atomic_linux = "yes"; then
 
   ac_save_CXXFLAGS="$CXXFLAGS"

   if test x$atomic_prefix != x; then
     CXXFLAGS="$CXXFLAGS -I$atomic_prefix"
   fi

 AC_CHECK_HEADER([asm/atomic.h],
 [ AC_MSG_CHECKING([for atomic_inc(), atomic_dec_and_test() support])

   AC_TRY_LINK([#include <asm/atomic.h>],
   [
	atomic_t i = ATOMIC_INIT(0); 
	atomic_inc(&i); 
	atomic_dec_and_test(&i);

   ],
   [ AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_ATOMIC_LINUX,, [Defined if <asm/atomic.h> is usable]) 
   ],
   [ AC_MSG_RESULT(no) 
     CXXFLAGS="$ac_save_CXXFLAGS"

     if test $atomic_linux_explicit = "yes"; then
       AC_MSG_ERROR(${ATOMIC_LINUX_ERROR})
     fi
   ])

 ])
 
fi

])
])

dnl Eric Crahen <crahen at code-foo dot com>

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
dnl Enables AM_ENABLE_ATOMIC_GCC to test for 
dnl GCC's atomic instruction support.
dnl
dnl --enable-atomic-gcc=yes|no [default=no]
dnl
dnl If support is available, then HAVE_ATOMIC_GCC 
dnl will be set
dnl
ifdef([AM_ENABLE_ATOMIC_GCC],,[

ATOMIC_GCC_ERROR=<<"EOF"
This compiler does not support the __atomic_add() and
__exchange_and_add() functions.
EOF

atomic_gcc_explicit="no"

AC_DEFUN([AM_ENABLE_ATOMIC_GCC],
[AC_ARG_ENABLE(atomic-gcc,
	AC_HELP_STRING([--enable-atomic-gcc],
		       [use gcc atomic instructions [default=no]]),
 [atomic_gcc_explicit="yes"], 
 [enable_atomic_gcc="no"])

 if test $enable_atomic_gcc = "yes"; then

 AC_CHECK_HEADER([bits/atomicity.h],
 [ AC_MSG_CHECKING([for __atomic_add(), __exchange_and_add() support])

   AC_TRY_LINK([#include <bits/atomicity.h>],
   [
      _Atomic_word i(0); 
      __atomic_add(&i, 1); 
      __exchange_and_add(&i, 1); 

   ],
   [ AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_ATOMIC_GCC,, [Defined if <bits/atomicity.h> is usable]) 
   ],
   [ AC_MSG_RESULT(no)

     if test $atomic_gcc_explicit = "yes"; then 
       AC_MSG_ERROR(${ATOMIC_GCC_ERROR})   
     fi
   ])

 ])
 
fi

])
])

dnl Eric Crahen <crahen at code-foo dot com>

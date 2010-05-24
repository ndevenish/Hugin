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
dnl Enables AM_DETECT_FTIME to test for 
dnl the right ftime()/_ftime() function
dnl
dnl --enable-atomic-gcc=yes|no [default=no]
dnl
dnl If support is available, then SYSTEM_FTIME
dnl is defined to be the ftime function 
dnl will be set
dnl
ifdef(AM_DETECT_FTIME,,[
  
NO_FTIME_ERROR=<<"EOF"
ftime()/_ftime() could not be found on this system.
EOF

have_ftime="detect"

AC_DEFUN([AM_DETECT_FTIME],
[

 AC_ARG_WITH(ftime,
	AC_HELP_STRING([--with-ftime],
		       [select an ftime() [default=detect]]),
 [have_ftime="$withval"
  if test $withval = "yes"; then have_ftime="detect"; fi
 ], 
 [have_ftime="detect"])

 if test $have_ftime = "detect"; then
   echo "detecting for ftime() function"
 fi

 if test $have_ftime = "win32"; then
   echo "verifying WIN32 _ftime() function"
 fi

 if test $have_ftime = "posix"; then
   echo "verifying POSIX ftime() function"
 fi

 dnl Test win32 style 
 if test $have_ftime != "posix"; then

 AC_CHECK_HEADER([sys/time.h],
 [ AC_MSG_CHECKING([for _ftime()])

   AC_TRY_LINK([#include <sys/time.h>],[_ftime(0);],
   [ AC_MSG_RESULT(yes)
     AC_DEFINE_UNQUOTED(SYSTEM_FTIME, [_ftime],
	[Defined if ftime()/_ftime() is usable])	
     have_ftime="win32"
   ],
   [ AC_MSG_RESULT(no) ])

 ])

 fi

 dnl Test posix style 
 if test $have_ftime != "win32"; then

 AC_CHECK_HEADER([sys/timeb.h],
 [ AC_MSG_CHECKING([for ftime()])

   AC_TRY_LINK([#include <sys/timeb.h>], [ ftime(0); ] ,
   [ AC_MSG_RESULT(yes)
     AC_DEFINE_UNQUOTED(SYSTEM_FTIME, [ftime],
	[Defined if ftime()/_ftime() is usable])	
     have_ftime="posix"
   ],
   [ AC_MSG_RESULT(no) ])

 ])

 fi

 dnl Display error if no ftime() is found
 if test $have_ftime = "detect"; then
  AC_MSG_ERROR(${NO_FTIME_ERROR})
 fi

 ])
])

dnl Eric Crahen <crahen at code-foo dot com>

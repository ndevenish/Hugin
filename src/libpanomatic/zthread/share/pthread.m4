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
dnl Enables AX_DETECT_PTHREAD to test for 
dnl pthreads support
dnl
dnl --with-pthread
dnl
dnl If support is available, then HAVE_POSIX_THREADS 
dnl will be set
dnl TODO: update the checks to only use -lrt when needed
dnl
dnl ifdef([AX_DETECT_PTHREAD],,[

NO_PTHREAD_ERROR=<<"EOF"
Please install/update your POSIX threads (pthreads) library. Updates
should be available from either your system vendor, or, for GNU/Linux
systems, go to http://pauillac.inria.fr/~xleroy/linuxthreads/.
GNU Pth can also be used if it was configured with --enable-pthread.
EOF

AC_DEFUN([AX_DETECT_PTHREAD],
[

pthread_explicit="no"

 AC_ARG_ENABLE([pthread],
        AC_HELP_STRING([--enable-pthread],
                       [use pthreads [default=detect]]),
 [pthread_explicit="yes"], 
 [enable_pthread="yes"])


 AC_ARG_WITH([pthread-prefix],
  [  --with-pthread-prefix   POSIX threads library prefix (optional)],
  [

   if test x$pthread_prefix != x ; then

     PTHREAD_LIBS="-L$withval/lib"
     PTHREAD_CXXFLAGS="-I$withval/include"

   fi

   pthread_explicit="yes"
   enable_pthread="yes"

  ], 
  [PTHREAD_LIBS=""
   PTHREAD_CXXFLAGS=""
  ])

  PTHREAD_CXXFLAGS="-DREENTRANT $PTHREAD_CXXFLAGS"

  if test x$enable_pthread != xno; then

   ac_save_LIBS="$LIBS" 
   ac_save_CXXFLAGS="$CXXFLAGS"
 
   AC_CHECK_HEADER([pthread.h],
   [AC_MSG_CHECKING([for linker option -pthread])

    LIBS="ac_save_LIBS $PTHREAD_LIBS -pthread"
    CXXFLAGS="$CXXFLAGS $PTHREAD_CXXFLAGS"

    AC_TRY_LINK([#include <pthread.h>],
    [
      pthread_create((pthread_t*) 0,(pthread_attr_t*) 0, 0, 0);
    ],
    [AC_MSG_RESULT(yes)
     AC_DEFINE(HAVE_POSIX_THREADS,,[defined when pthreads is available])
     PTHREAD_LIBS="$PTHREAD_LIBS -pthread" 

    ],
    [AC_MSG_RESULT(no)
     AC_MSG_CHECKING(for linker option -lpthread)

     LIBS="$ac_save_LIBS $PTHREAD_LIBS -lpthread"

     AC_TRY_LINK([#include <pthread.h>],
     [
       pthread_create((pthread_t*) 0,(pthread_attr_t*) 0, 0, 0);
     ], 
     [AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_POSIX_THREADS,,[defined when pthreads is available])
      PTHREAD_LIBS="$PTHREAD_LIBS -lpthread"
     ],
     [AC_MSG_RESULT(no)
    
     if test $pthread_explicit = "yes"; then
       AC_MSG_ERROR(${NO_PTHREAD_ERROR})
     fi

    ])
   ])
  ])

  dnl Check for SunOS rt library
  AC_CHECK_LIB(rt, sched_get_priority_max, 
  [ AC_DEFINE(HAVE_SCHED_RT,,[Defined if -lrt is needed for RT scheduling])
    PTHREAD_LIBS="$LIBS -lrt" ])

  AC_SUBST(PTHREAD_LIBS)
  AC_SUBST(PTHREAD_CXXFLAGS)

  dnl Check for sched_yield
  AC_MSG_CHECKING(for sched_yield);
  AC_TRY_LINK([#include <sched.h>],
    [ sched_yield(); ], 
    [ AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_SCHED_YIELD,,[Defined if sched_yield() is available]) ],  
    [ AC_MSG_RESULT(no) ])

  dnl Check for pthread_yield
  AC_MSG_CHECKING(for pthread_yield);
  AC_TRY_LINK([#include <pthread.h>],
    [ pthread_yield(); ], 
    [ AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_PTHREAD_YIELD,,[Defined if pthread_yield() is available]) ],  
    [ AC_MSG_RESULT(no) ])

  dnl Check for pthread_key_create
  AC_MSG_CHECKING(for pthread_key_create)
  AC_TRY_LINK([#include <pthread.h>],
  [ pthread_key_create(0, 0);],
  [ AC_MSG_RESULT(yes) 
    AC_DEFINE(HAVE_PTHREADKEY_CREATE,,[Defined if pthread_key_create() is available]) ],
  [ AC_MSG_RESULT(no)
    AC_MSG_CHECKING(for pthread_keycreate)
    AC_TRY_LINK([#include <pthread.h>],
    [ pthread_keycreate(0,0); ], 
    [ AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_PTHREADKEYCREATE,,[Defined if pthread_keycreate() is available]) 
    ], AC_MSG_RESULT(no))
  ])

  CXXFLAGS="$ac_save_CXXFLAGS"
  LIBS="$ac_save_LIBS"

 fi

])
dnl ])


dnl Eric Crahen <crahen at code-foo dot com>

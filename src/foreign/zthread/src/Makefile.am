## Copyright (c) 2005, Eric Crahen
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is furnished
## to do so, subject to the following conditions:
## 
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
## 
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
## WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
## CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#AM_CXXFLAGS = @COMPILER_OPTIONS@ @EXTRA_COMPILER_OPTIONS@
INCLUDES = -I../include
SUBDIRS=.
AM_CXXFLAGS=-DNDEBUG

#libdir=$(prefix)/lib

noinst_LIBRARIES = libZThread.a

#libZThread_a_LIBADD=@LINKER_OPTIONS@ @EXTRA_LINKER_OPTIONS@
#LIBADD=@LINKER_OPTIONS@ @EXTRA_LINKER_OPTIONS@


libZThread_a_SOURCES = \
AtomicCount.cxx \
Condition.cxx \
ConcurrentExecutor.cxx \
CountingSemaphore.cxx \
FastMutex.cxx \
FastRecursiveMutex.cxx \
Mutex.cxx \
RecursiveMutexImpl.cxx \
RecursiveMutex.cxx \
Monitor.cxx \
PoolExecutor.cxx \
PriorityCondition.cxx \
PriorityInheritanceMutex.cxx \
PriorityMutex.cxx \
PrioritySemaphore.cxx \
Semaphore.cxx \
SynchronousExecutor.cxx \
Thread.cxx \
ThreadedExecutor.cxx \
ThreadImpl.cxx \
ThreadLocalImpl.cxx \
ThreadQueue.cxx \
Time.cxx \
ThreadOps.cxx


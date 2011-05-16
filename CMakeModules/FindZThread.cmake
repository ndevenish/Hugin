# Search for ZThread library
# Copyright (C) 2011 Andreas Metzler. Public Domain

# Search for locally install zthread library, fall back to copy in
# src/foreign otherwise.
# Sets these variables:
# ZTHREAD_INCLUDE_DIRS
# ZTHREAD_LIBRARIES
# ZTHREAD_FOUND


find_path(ZTHREAD_INCLUDE_DIRS zthread/Runnable.h)

if(ZTHREAD_INCLUDE_DIRS)
  find_library(ZTHREAD_LIBRARIES ZThread)
  if(ZTHREAD_LIBRARIES)
    MESSAGE(STATUS "ZThread library found")
    set(ZTHREAD_FOUND "YES")
  endif(ZTHREAD_LIBRARIES)
else(ZTHREAD_INCLUDE_DIRS)
  message(STATUS "ZThread library not found. falling back to included copy")
  set(ZTHREAD_LIBRARIES "ZThread")
  set(ZTHREAD_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src/foreign/zthread/include")
endif(ZTHREAD_INCLUDE_DIRS)


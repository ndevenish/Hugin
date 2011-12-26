# Search for flann library
# Copyright (C) 2011 Andreas Metzler. Public Domain

# Search for locally install flann library, fall back to copy in
# src/foreign otherwise.
# Sets these variables:
# FLANN_INCLUDE_DIRS
# FLANN_LIBRARIES
# FLANN_FOUND


find_path(FLANN_INCLUDE_DIRS flann/flann.hpp)

if(FLANN_INCLUDE_DIRS)
  find_library(FLANN_LIBRARIES flann_cpp)
  if(FLANN_LIBRARIES)
    MESSAGE(STATUS "flann library found")
    set(FLANN_FOUND "YES")
  endif(FLANN_LIBRARIES)
else(FLANN_INCLUDE_DIRS)
  message(STATUS "flann library not found. falling back to included copy")
  set(FLANN_LIBRARIES "flann_cpp")
  set(FLANN_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/src/foreign")
endif(FLANN_INCLUDE_DIRS)



cmake_minimum_required(VERSION 2.6)
IF(WIN32)
  STRING(REGEX REPLACE "(.*)/[^/]+$" "\\1" work "${CMAKE_SOURCE_DIR}")
  # create the cache entry
  SET(SOURCE_BASE_DIR ${work} CACHE FILEPATH "parent dir of hugin source root")

  LIST(APPEND CMAKE_PREFIX_PATH ${SOURCE_BASE_DIR})
ENDIF(WIN32)

PROJECT(panomatic)

# set include path for internal libraries before using finding and initializing
# external dependencies. This makes sure that "our" include paths are specified
# before any system includes

#include_directories( ${CMAKE_BINARY_DIR}/liblocalfeature )
#include_directories( ${CMAKE_SOURCE_DIR}/keypoints )
#include_directories( ${CMAKE_SOURCE_DIR}/panomatic )
include_directories( ${CMAKE_SOURCE_DIR}/ )
#include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/vigra )
#include_directories( ${CMAKE_SOURCE_DIR}/src/celeste )

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules)

IF (UNIX)
FIND_PACKAGE(PkgConfig)
ENDIF (UNIX)

FIND_PACKAGE(TIFF REQUIRED)
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
FIND_PACKAGE(JPEG REQUIRED)
INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
FIND_PACKAGE(PNG REQUIRED)
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})
FIND_PACKAGE(VIGRA REQUIRED)
INCLUDE_DIRECTORIES(${VIGRA_INCLUDE_DIR})


IF(WIN32)
  set(BOOST_ROOT ${SOURCE_BASE_DIR}/boost_1_41_0) 
ENDIF(WIN32)
FIND_PACKAGE(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

FIND_PACKAGE(BoostPython)
IF (BOOST_PYTHON_FOUND)
INCLUDE_DIRECTORIES(${BOOST_PYTHON_INCLUDES})
ENDIF(BOOST_PYTHON_FOUND)
INCLUDE_DIRECTORIES(.)

option(ENABLE_SURF "Enable patented SURF descriptor")
if (ENABLE_SURF)
  MESSAGE("SURF enabled")
  ADD_DEFINITIONS(-DSURF_ENABLED)
endif (ENABLE_SURF)

SET(BINDIR bin CACHE STRING "Destination for binaries")
SET(LIBDIR lib CACHE STRING "Destination for libraries")
SET(INCDIR include CACHE STRING "Destination for include files")

IF(WIN32)
  # disable some warnings
  ADD_DEFINITIONS(/D_SCL_SECURE_NO_WARNINGS)
  ADD_DEFINITIONS(/D_CRT_SECURE_NO_WARNINGS)

  # install into place in build-dir
  SET( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/FILES CACHE FILEPATH "install prefix" FORCE)
  SET( INSTALL_WIN_FILES ${ZLIB_DLL} ${PNG_DLL} ${TIFF_DLL} ${JPEG_DLL} ${VIGRA_DLL} )
  SET(BINDIR  bin)
  SET(LIBDIR  lib${LIB_SUFFIX})
  SET(INCDIR  include)
  INSTALL(FILES ${INSTALL_WIN_FILES} DESTINATION ${BINDIR})
  IF(BOOST_PYTHON_FOUND)
    INSTALL(FILES ${BOOST_PYTHON_DLL} DESTINATION ${BINDIR})
  ENDIF(BOOST_PYTHON_FOUND)
ENDIF(WIN32)

add_subdirectory(localfeatures)
add_subdirectory(keypoints)
add_subdirectory(zthread/src)
add_subdirectory(panomatic)


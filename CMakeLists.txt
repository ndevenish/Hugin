set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

include(HuginMacros)
include(CheckIncludeFiles)

##  global setup
project(hugin)

# version
set(V_MAJOR 0)
set(V_MINOR 7)
set(V_PATCH 0)

# version for internal libraries.
set(HUGIN_LIB_VERSION 0.0)

#FIND_PACKAGE(BISON REQUIRED)
#FIND_PACKAGE(FLEX REQUIRED)

# find dependencies
IF (UNIX)
FIND_PACKAGE(PkgConfig)
ENDIF (UNIX)

##
## The directory that contains the hugin source root is a good
## place to look for support packages, so post it in cache.
## 
## Our FIND_PACKAGE calls modules will search SOURCE_BASE_DIR
## after the local standard places  
##
# extract pathname from CMAKE_SOURCE_DIR (note result must be a local var)
STRING( REGEX REPLACE "(.*)/[^/]+$" "\\1" work "${CMAKE_SOURCE_DIR}" )
# create the cache entry
SET( SOURCE_BASE_DIR ${work} CACHE FILEPATH "parent dir of hugin source root" )

##
## wxWidgets
##
#  This must come before FINDs for tiff, jpeg, png, zlib to enable
#  finding the wxWidgets distributions of those packages.

SET(wxWidgets_USE_LIBS base core xrc html xml adv)
FIND_PACKAGE(wxWidgets REQUIRED)
IF(NOT wxWidgets_FOUND)
  MESSAGE("wxWidgets was not found, GUI programs (hugin, nona_gui) disabled")
ENDIF(NOT wxWidgets_FOUND) 

##
## Graphics libraries
##
# wxWidgets sub-packages will be used if native ones aren't found

FIND_PACKAGE(TIFF REQUIRED)
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
FIND_PACKAGE(JPEG REQUIRED)
INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR})
FIND_PACKAGE(PNG REQUIRED)
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})

FIND_PACKAGE(OpenEXR REQUIRED)
include_directories(${OPENEXR_INCLUDE_DIR})

##
## Panotools
##

FIND_PACKAGE(PANO13)
FIND_PACKAGE(PANO12)

IF(PANO13_FOUND)
  SET(PANO_LIBRARIES ${PANO13_LIBRARIES})
  SET(PANO_INCLUDE_DIR ${PANO13_INCLUDE_DIR})
ELSEIF(PANO12_FOUND)
  SET(PANO_LIBRARIES ${PANO13_LIBRARIES})
  SET(PANO_INCLUDE_DIR ${PANO13_INCLUDE_DIR})
ELSE(PANO13_FOUND)
  MESSAGE(SEND_ERROR "Either libpano13 or libpano12 are required")
ENDIF(PANO13_FOUND) 
INCLUDE_DIRECTORIES(${PANO_INCLUDE_DIR})

##
## Boost
##


FIND_PACKAGE(Boost REQUIRED thread)
IF (NOT Boost_thread_FOUND)
  MESSAGE(FATAL_ERROR "boost thread library not found. If it is installed with a\nunrecognized suffix, specify it with -DBoost_LIB_SUFFIX=<suffix>")
ENDIF (NOT Boost_thread_FOUND)


# vips is not used right now
#IF (WIN32)
#  # todo: check for vips (not needed right now)
#ELSE (WIN32)
#  pkg_check_modules(VIPS vips-7.12)
#  LIST2STRING(VIPS_CFLAGS_OTHER VIPS_CFLAGS_OTHER_STRING)
#  LIST2STRING(VIPS_LDFLAGS_OTHER VIPS_LDFLAGS_OTHER_STRING)
#
#  pkg_check_modules(VIPSCC vipsCC-7.12)
#  LIST2STRING(VIPSCC_CFLAGS_OTHER VIPSCC_CFLAGS_OTHER_STRING)
#  LIST2STRING(VIPSCC_LDFLAGS_OTHER VIPSCC_LDFLAGS_OTHER_STRING)
#ENDIF (WIN32)


#BUILD SETUP
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# allow user to specify lib suffix
set(LIB_SUFFIX "" CACHE STRING "Define suffix of lib directory name (32/64)" )
set(HUGIN_SHARED "1" CACHE STRING "Build shared internal libraries" )

IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  MESSAGE("Building in Debug mode, resulting programs will be SLOW")
  ADD_DEFINITIONS(-DDEBUG)
  IF(UNIX)
    ADD_DEFINITIONS(-Wall)
  ENDIF(UNIX)
ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")

#IF(UNIX)
#  SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall" CACHE)
#  SET(CMAKE_C_FLAGS_RELEASE "-O2 -Wall" CACHE)
#  SET(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -Wall -DDEBUG" qCACHE)
#  SET(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -Wall -DDEBUG" CACHE)
#ENDIF(UNIX)

IF (UNIX)
  # define destination dirs (relative to prefix)
  SET(HUGINDATADIR share/hugin)
  SET(DATADIR share)
  SET(BINDIR  bin)
  SET(LOCALEDIR share/locale) 
  SET(ICONDIR share/icons) 
  SET(LINKDIR share/applications)
  SET(LIBDIR  lib${LIB_SUFFIX})
  SET(INSTALL_XRC_DIR ${CMAKE_INSTALL_PREFIX}/share/hugin/xrc)

  IF (${HUGIN_SHARED})
    # use shared internal libraries
    SET(HUGIN_SHARED_LIBS 1)
  ENDIF (${HUGIN_SHARED})
  MESSAGE("Using shared libraries")
ELSE (UNIX)
  # Windows
  SET(BINDIR ${CMAKE_INSTALL_PREFIX})
  SET(DATADIR ${CMAKE_INSTALL_PREFIX})
  SET(HUGINDATADIR ${CMAKE_INSTALL_PREFIX})
  SET(LIBDIR ${CMAKE_INSTALL_PREFIX})
  SET(LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale) 

  # no shared libs DLL on windows. The code is just
  # not set up for that..
  ADD_DEFINITIONS(-DVIGRA_STATIC_LIB)

  IF (MSVC)
    # Stop MSVC8 from bitching about the C library
    ADD_DEFINITIONS(/D_CRT_SECURE_NO_DEPRECATE)
    # compile everything for the static C runtime
    STRING(REPLACE /MD /MT CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
    STRING(REPLACE /MD /MT CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
    STRING(REPLACE /MD /MT CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_CXX_FLAGS_MINSIZEREL})
    STRING(REPLACE /MD /MT CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
    STRING(REPLACE /MD /MT CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
    STRING(REPLACE /MD /MT CMAKE_C_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG})
    STRING(REPLACE /MD /MT CMAKE_C_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL})
    STRING(REPLACE /MD /MT CMAKE_C_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO})
    # link only with the static C runtime !! THIS IS A MUST !!
    SET( CMAKE_EXE_LINKER_FLAGS
        ${CMAKE_EXE_LINKER_FLAGS} /noDefaultLib:MSVCRT /noDefaultLib:MSVCRTd
       CACHE STRING "for MSVC"
    )
  ENDIF(MSVC)
ENDIF (UNIX)

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
  MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

#This namespace workaround is not needed for CMake >= 2.5.x
IF (APPLE) 
   ## Add needed Compiler and Linker flags for OSX
      IF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
      	 SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flat_namespace -undefined suppress" )
      ENDIF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
ENDIF (APPLE)


# create config.h file
configure_file(src/hugin_config.h.in.cmake ${CMAKE_BINARY_DIR}/src/hugin_config.h)



# setup some common compilation flags
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

INCLUDE_DIRECTORIES( ${VIPSCC_INCLUDE_DIRS} )
LINK_DIRECTORIES( ${VIPSCC_LIBRARY_DIRS} )


add_subdirectory(src)
add_subdirectory(doc)

SET(CPACK_PACKAGE_VERSION_MAJOR "${V_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${V_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${V_PATCH}")
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${V_MAJOR}.${V_MINOR}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "hugin-${V_MAJOR}.${V_MINOR}.${V_PATCH}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_SOURCE_IGNORE_FILES
"/_CPack_Packages/"
"/CMakeCache.txt$"
"\\\\.dir/"
"\\\\.tar\\\\.gz$"
"\\\\.tar\\\\.Z$"
"\\\\.svn/"
"\\\\.cvsignore$"
"\\\\.swp$"
"~$"
"\\\\.#"
"/#"
"/tools/autooptimiser$"
"/tools/fulla$"
"/tools/nona$"
"/tools/vig_optimize$"
"/hugin/hugin$"
"/nona_gui/nona_gui$"
"/stitch_project/hugin_stitch_project$"
"/deghosting/hugin_hdrmerge$" 
"/build/"
"/CVS/"
"/\\\\.libs/"
"/\\\\.deps/"
"\\\\.o$"
"\\\\.a$"
"\\\\.lo$"
"\\\\.so$"
"\\\\.so\\\\.0$" 
"\\\\.so\\\\.0\\\\.0$" 
"\\\\.la$"
"Makefile\\\\.in$"
 )
INCLUDE(CPack)

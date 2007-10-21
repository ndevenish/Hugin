set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

include(HuginMacros)
include(CheckIncludeFiles)

##  global setup
project(hugin)

# version
set(V_MAJOR 0)
set(V_MINOR 7)
set(V_PATCH 1)

#FIND_PACKAGE(BISON REQUIRED)
#FIND_PACKAGE(FLEX REQUIRED)

# find dependencies
FIND_PACKAGE(PkgConfig)
FIND_PACKAGE(TIFF REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)

FIND_PACKAGE(PANO13)
FIND_PACKAGE(PANO12)

IF(PANO13_FOUND)
  SET(PANO_LIBRARIES ${PANO13_LIBRARIES})
  SET(PANO_INCLUDE_DIR ${PANO13_INCLUDE_DIR})
ELSEIF(PANO12_FOUND)
  SET(PANO_LIBRARIES ${PANO13_LIBRARIES})
  SET(PANO_INCLUDE_DIR ${PANO13_INCLUDE_DIR})
ELSE(PANO13_FOUND)
  ERROR("Either libpano13 or libpano12 are required")
ENDIF(PANO13_FOUND) 

FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(OpenEXR)

IF (OPENEXR_FOUND)
include_directories(${OPENEXR_INCLUDE_DIR})
ENDIF (OPENEXR_FOUND)

FIND_PACKAGE(Boost REQUIRED)

SET(wxWidgets_USE_LIBS base core xrc)
FIND_PACKAGE(wxWidgets REQUIRED)
IF(NOT wxWidgets_FOUND)
  MESSAGE("wxWidgets was not found, GUI (hugin, nona_gui) disabled")
ENDIF(NOT wxWidgets_FOUND) 

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
  MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

pkg_check_modules(VIPS vips-7.12)
LIST2STRING(VIPS_CFLAGS_OTHER VIPS_CFLAGS_OTHER_STRING)
LIST2STRING(VIPS_LDFLAGS_OTHER VIPS_LDFLAGS_OTHER_STRING)

pkg_check_modules(VIPSCC vipsCC-7.12)
LIST2STRING(VIPSCC_CFLAGS_OTHER VIPSCC_CFLAGS_OTHER_STRING)
LIST2STRING(VIPSCC_LDFLAGS_OTHER VIPSCC_LDFLAGS_OTHER_STRING)

MESSAGE("tiff libraries: ${TIFF_LIBRARIES}")
MESSAGE("tiff include dir: ${TIFF_INCLUDE_DIR}")

#BUILD SETUP
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

#None, Debug, Release, or custom ones
#IF(WANT_DEBUG)
#  MESSAGE("Enabled Debug build")
#  SET(CMAKE_BUILD_TYPE Debug)
#  ADD_DEFINITIONS(-DDEBUG)
#ELSE(WANT_DEBUG)
#  SET(CMAKE_BUILD_TYPE Release)
#ENDIF(WANT_DEBUG)

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
#  SET(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -Wall -DDEBUG" CACHE)
#  SET(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -Wall -DDEBUG" CACHE)
#ENDIF(UNIX)

IF (UNIX)
  # define destination dirs (relative to prefix)
  SET(DATADIR share/hugin)
  SET(BINDIR  bin)
  SET(ICONDIR share/icons) 
  SET(LINKDIR share/applications)

  # backward compatibility
  SET(INSTALL_XRC_DIR ${CMAKE_INSTALL_PREFIX}/share/hugin/xrc)
ELSEIF (WIN32)
  SET(DATADIR xrc)
  SET(BINDIR  "")
  SET(ICONDIR  "")
  SET(LINKDIR  "")

  # backward compatibility
  SET(INSTALL_XRC_DIR xrc)
  SET(INSTALL_LOCALE_DIR locale)
ENDIF(UNIX)


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


IF(SHARED_INTERNAL_LIBS)
ELSE(SHARED_INTERNAL_LIBS)
ADD_DEFINITIONS(-DVIGRA_STATIC_LIB)
ENDIF(SHARED_INTERNAL_LIBS)

# setup some common compilation flags
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

LINK_DIRECTORIES( ${VIPSCC_LIBRARY_DIRS} )
INCLUDE_DIRECTORIES( ${VIPSCC_INCLUDE_DIRS} )

INCLUDE_DIRECTORIES(${PANO_INCLUDE_DIR})

add_subdirectory(src)
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
"/build/"
"/CVS/"
"/\\\\.libs/"
"/\\\\.deps/"
"\\\\.o$"
"\\\\.a$"
"\\\\.lo$"
"\\\\.so$"
"\\\\.la$"
"Makefile\\\\.in$"
 )
INCLUDE(CPack)

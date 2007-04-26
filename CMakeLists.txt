set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

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

FIND_PACKAGE(TIFF REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(PANO13 REQUIRED)
FIND_PACKAGE(OpenEXR)
FIND_PACKAGE(Boost REQUIRED)

SET(wxWidgets_USE_LIBS base core xrc)
FIND_PACKAGE(wxWidgets REQUIRED)
IF(wxWidgets_FOUND)
  INCLUDE(${wxWidgets_USE_FILE})
ENDIF(wxWidgets_FOUND) 

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
  MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)


#BUILD SETUP
#None, Debug, Release, or custom ones
IF(WANT_DEBUG)
  MESSAGE("Enabled Debug build")
  SET(CMAKE_BUILD_TYPE Debug)
  ADD_DEFINITIONS(-DDEBUG)
ELSE(WANT_DEBUG)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF(WANT_DEBUG)

IF(UNIX)
  SET(CMAKE_CXX_FLAGS_RELEASE "-O2 -Wall")
  SET(CMAKE_C_FLAGS_RELEASE "-O2 -Wall")
  SET(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -Wall")
  SET(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -Wall")
ENDIF(UNIX)

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
ELSEIF(APPLE)
  MESSAGE("WARNING: installation for OSX not yet supported")
ENDIF(UNIX)

# create config.h file
configure_file(src/include/config.h.in.cmake ${CMAKE_BINARY_DIR}/src/include/config.h)

# setup some common compilation flags
INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

add_subdirectory(src)

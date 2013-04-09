
# require at least cmake 2.8
cmake_minimum_required(VERSION 2.8 FATAL_ERROR )

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

##
## Cleaner Debian Packaging (disable by -DDISABLE_DPKG=ON)
##
OPTION (DISABLE_DPKG "Do not use debian scripts to get package dependences" OFF)


IF(NOT DISABLE_DPKG)
  set(ARCH_TRIPLET)
  if(UNIX)
    FIND_PROGRAM(DPKG_ARCHITECTURE_EXECUTABLE dpkg-architecture)
    if(DPKG_ARCHITECTURE_EXECUTABLE)
      EXECUTE_PROCESS(COMMAND dpkg-architecture -qDEB_HOST_MULTIARCH
        OUTPUT_VARIABLE ARCH_TRIPLET
        ERROR_VARIABLE ERROR_ARCH_TRIPLET
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
  endif()
  if(ARCH_TRIPLET)
    set(SYSTEM_LIB_DIRS /usr/lib /usr/lib/${ARCH_TRIPLET} /usr/local/lib)
  else()
    set(SYSTEM_LIB_DIRS /usr/lib /usr/local/lib)
  endif()
ENDIF()

include(HuginMacros)
include(CheckIncludeFiles)

##  global setup
project(hugin)

# version
set(V_MAJOR 2013)
set(V_MINOR 0)
set(V_PATCH 0)

#
# Check for mercurial and get current revision
#

IF(EXISTS ${PROJECT_SOURCE_DIR}/.hg)
  FIND_PROGRAM(_hg hg)
  IF(NOT ${_hg} MATCHES "-NOTFOUND")
    EXECUTE_PROCESS(COMMAND ${_hg} summary WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE _release OUTPUT_STRIP_TRAILING_WHITESPACE)
    foreach(_v_l ${_release})
      if(_v_l MATCHES "^.*: *[^0-9]*\([0-9]+\):\([a-z0-9]+\)")
        set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_1})
        set(HUGIN_WC_REVISION ${CMAKE_MATCH_2})
      endif()
    endforeach()
  ELSE()
    SET(HUGIN_WC_REVISION 0)
  ENDIF()
  SET(HUGIN_PACKAGE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH})
  SET(HUGIN_DEVELOPMENT_VERSION 1)
  FILE(WRITE ${PROJECT_SOURCE_DIR}/rev.txt "${HUGIN_WC_REVISION}")
  MESSAGE("-- Current HG revision is ${HUGIN_WC_REVISION}")
ELSE()
  IF (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
    # comment out the following line to compile an old version from the subversion repository
    MESSAGE(FATAL_ERROR "Hugin subversion repository is not up to date. The development happens now in mercurial repository. Please switch to mercurial. Sorry for the inconsistency")
  ENDIF()

  SET(HUGIN_PACKAGE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH})
  SET(HUGIN_WC_REVISION 0)
  FILE(READ "${PROJECT_SOURCE_DIR}/rev.txt" HUGIN_WC_REVISION)
  MESSAGE("-- Current HG revision is ${HUGIN_WC_REVISION}")
  MESSAGE("-- Assuming this is a tarball (release) build for ${HUGIN_PACKAGE_VERSION}")
  SET(HUGIN_DEVELOPMENT_VERSION 0)
ENDIF()

#
# Check for mercurial and update changelog
#
IF (EXISTS ${PROJECT_SOURCE_DIR}/.hg)
  IF (UPDATE_CHANGELOG)
   FIND_PROGRAM(_hg hg)
   EXECUTE_PROCESS(COMMAND ${_hg} log --follow --style ${PROJECT_SOURCE_DIR}/Changelog.style
   WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
   OUTPUT_FILE ${PROJECT_SOURCE_DIR}/ChangeLog)
   MESSAGE("-- New ChangeLog generated")
  ENDIF (UPDATE_CHANGELOG)
ENDIF (EXISTS ${PROJECT_SOURCE_DIR}/.hg)

# version to display
IF (HUGIN_DEVELOPMENT_VERSION EQUAL 1)
  set(DISPLAY_VERSION "Pre-Release ${V_MAJOR}.${V_MINOR}.${V_PATCH}.${HUGIN_WC_REVISION}")
ELSE (HUGIN_DEVELOPMENT_VERSION EQUAL 1)
  set(DISPLAY_VERSION "${V_MAJOR}.${V_MINOR}.${V_PATCH}.${HUGIN_WC_REVISION}")
ENDIF (HUGIN_DEVELOPMENT_VERSION EQUAL 1)

# version for internal libraries.
set(HUGIN_LIB_VERSION 0.0)

# set include path for internal libraries before using finding and initializing
# external dependencies. This makes sure that "our" include paths are specified
# before any system includes

include_directories( ${CMAKE_BINARY_DIR}/src )
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin_base )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/vigra )
include_directories( ${CMAKE_SOURCE_DIR}/src/celeste )
include_directories( ${CMAKE_BINARY_DIR}/src/celeste )
include_directories( ${CMAKE_SOURCE_DIR}/src )

# this function is not available on some systems, notably FreeBSD and Windows
include(CheckFunctionExists)
set(CMAKE_REQUIRED_LIBRARIES -lm)
check_function_exists(log2 HAVE_LOG2)
# use messages only for debug - on Windows they are extremely intrusive
# message("LOG2 ${HAVE_LOG2}")

IF (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
  IF (UPDATE_CHANGELOG)
    EXECUTE_PROCESS(COMMAND "svn2cl" "--group-by-day" "--include-rev" "--linelen=78"
      "--output=${PROJECT_SOURCE_DIR}/ChangeLog" "${PROJECT_SOURCE_DIR}" )
  ENDIF (UPDATE_CHANGELOG)
ENDIF (EXISTS ${PROJECT_SOURCE_DIR}/.svn)


#
# find dependencies
#
IF (UNIX)
FIND_PACKAGE(PkgConfig)
ENDIF (UNIX)

IF(WIN32)
    set(HUGIN_SHARED "0" CACHE STRING "Build shared internal libraries" )
ELSE(WIN32)
    set(HUGIN_SHARED "1" CACHE STRING "Build shared internal libraries" )
ENDIF(WIN32)

#
# Setup windows search paths for dependencies
#
IF (WIN32)
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

  # wxWidgets
  IF(NOT wxWidgets_ROOT_SUFFIX)
    SET( wxWidgets_ROOT_SUFFIX 2.8.10 CACHE STRING "version of wxWidgets." FORCE )
  ENDIF(NOT wxWidgets_ROOT_SUFFIX)

  SET(wxWidgets_ROOT_DIR ${SOURCE_BASE_DIR}/wxWidgets-${wxWidgets_ROOT_SUFFIX})
  # GLEW moved to FildGLEW.cmake module
  # SET(GLEW_INCLUDE_DIR ${SOURCE_BASE_DIR}/glew/include)
  # for dynamic build, it's glew32.lib and the dll must be copied into hugin's bin folder
  # SET(GLEW_LIBRARIES ${SOURCE_BASE_DIR}/glew/lib/glew32s.lib)
ENDIF(WIN32)

##
## wxWidgets
##
#  This must come before FINDs for tiff, jpeg, png, zlib to enable
#  finding the wxWidgets distributions of those packages (Win32 only).
SET(wxWidgets_USE_LIBS base core aui xrc html xml adv gl net)
FIND_PACKAGE(wxWidgets REQUIRED)

# hugin requires wxwidgets 2.7.0 or above
IF (UNIX)
  EXEC_PROGRAM(sh ARGS "${wxWidgets_CONFIG_EXECUTABLE} --version" OUTPUT_VARIABLE wxWidgets_VERSION)
  STRING(REGEX REPLACE "^([0-9]+\\.[0-9]+)\\..*" "\\1"  wxWidgets_VERSION ${wxWidgets_VERSION})
  IF(wxWidgets_VERSION STREQUAL "2.5")
    SET(wxWidgets_FOUND FALSE)
    MESSAGE("wxWidgets 2.5.x not supported")
  ENDIF(wxWidgets_VERSION STREQUAL "2.5")
  IF(wxWidgets_VERSION STREQUAL "2.6")
    SET(wxWidgets_FOUND FALSE)
    MESSAGE("wxWidgets 2.6.x not supported")
  ENDIF(wxWidgets_VERSION STREQUAL "2.6")
ENDIF (UNIX)

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
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})

FIND_PACKAGE(OpenEXR REQUIRED)
include_directories(${OPENEXR_INCLUDE_DIR})

FIND_PACKAGE(Exiv2 REQUIRED)
INCLUDE_DIRECTORIES(${EXIV2_INCLUDE_DIR})

## OpenGL and GLEW

FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLUT)
FIND_PACKAGE(GLEW)
IF(NOT OPENGL_FOUND)
  MESSAGE("OpenGL was not found, hugin disabled")
ENDIF(NOT OPENGL_FOUND)
IF(GLEW_FOUND)
  INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${GLUT_INCLUDE_DIR})
  INCLUDE_DIRECTORIES(${GLEW_INCLUDE_DIR})
ENDIF(GLEW_FOUND)

##
## Panotools
##

FIND_PACKAGE(PANO13 2.9.18 REQUIRED)

IF(PANO13_FOUND)
  SET(PANO_LIBRARIES ${PANO13_LIBRARIES})
  SET(PANO_INCLUDE_DIR ${PANO13_INCLUDE_DIR})
ELSE(PANO13_FOUND)
  MESSAGE(SEND_ERROR "libpano13 required")
ENDIF(PANO13_FOUND) 
INCLUDE_DIRECTORIES(${PANO_INCLUDE_DIR})

##
## Boost
##
# I am sick of having to edit the boost rev number in this file.
# Now it is a cache variable you can edit in cmakesetup --TKS
#
IF( NOT Boost_root_suffix )
  SET( Boost_root_suffix _1_41_0 CACHE STRING "suffix of boost root dir." FORCE )
ENDIF( NOT Boost_root_suffix )

IF(WIN32)
  IF(${HUGIN_SHARED})
    SET(Boost_USE_STATIC_LIBS OFF)
    # ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK)
    # ADD_DEFINITIONS(-D_DLL)
    ADD_DEFINITIONS(-DBOOST_ALL_NO_LIB)
  ELSE(${HUGIN_SHARED})
    SET(Boost_USE_STATIC_LIBS ON)
  ENDIF(${HUGIN_SHARED})
  # If BOOST_ROOT was defined in the environment, use it. 
  if (NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "") 
    set(BOOST_ROOT $ENV{BOOST_ROOT}) 
  else(NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "") 
    set(BOOST_ROOT ${SOURCE_BASE_DIR}/boost${Boost_root_suffix}) 
   endif(NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "") 
ENDIF(WIN32)
FIND_PACKAGE(Boost 1.41 COMPONENTS thread date_time regex filesystem iostreams system signals)
IF (NOT Boost_FOUND)
  IF(WIN32)
    MESSAGE(FATAL_ERROR "Boost not found. Bad BOOST_ROOT? On windows, try editing Boost_root_suffix")
  ELSE()
    MESSAGE(FATAL_ERROR "Boost not found. Maybe wrong version. Hugin requires at least version 1.41")
  ENDIF()
ENDIF (NOT Boost_FOUND)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
  MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

FIND_PACKAGE(Threads)
FIND_PACKAGE(ZThread)
FIND_PACKAGE(flann)

IF(NOT FLANN_FOUND)
  INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src/foreign)
ENDIF(NOT FLANN_FOUND)

IF(WIN32)
  # HTML Helpworkshop for creating help file
  FIND_PACKAGE(HTMLHelp)
  IF(${HTML_HELP_COMPILER} MATCHES "-NOTFOUND")
    MESSAGE(FATAL_ERROR "MS HTML Help Workshop not found. It is required to build Hugins help file.")
  ENDIF()
ENDIF(WIN32)

FIND_PACKAGE(Lensfun REQUIRED)
INCLUDE_DIRECTORIES(${LENSFUN_INCLUDE_DIR})

IF(UNIX)
  OPTION(BUILD_HSI "Python Scripting Interface" ON)
ELSE(UNIX)
  OPTION(BUILD_HSI "Python Scripting Interface" OFF)
ENDIF(UNIX)
IF(BUILD_HSI)
  # find Python first because the version of SWIG required depends on the
  # version of Python available
  if(WIN32)
    FIND_PACKAGE(PythonLibs REQUIRED)
  else()
    #  Linux/MacOS
    find_package(PythonInterp REQUIRED)
    find_package(PythonLibs ${PYTHON_VERSION_STRING} REQUIRED)
  endif()
  INCLUDE_DIRECTORIES(${PYTHON_INCLUDE_PATH})
  ADD_DEFINITIONS(-DHUGIN_HSI)
  FIND_FILE(PYTHON_PATCHLEVEL_H patchlevel.h
      PATHS ${PYTHON_INCLUDE_DIRS}
  )
  IF(${PYTHON_PATCHLEVEL_H} MATCHES "-NOTFOUND")
      MESSAGE(FATAL_ERROR "Python patchlevel.h not found")
  ENDIF()
  # get version of Python libs
  FILE(STRINGS "${PYTHON_PATCHLEVEL_H}"
      PY_DEF_MAJOR
      REGEX "#define[ \t]+PY_MAJOR_VERSION.*$")
  FILE(STRINGS "${PYTHON_PATCHLEVEL_H}"
      PY_DEF_MINOR
      REGEX "#define[ \t]+PY_MINOR_VERSION.*$")

  FILE(STRINGS "${PYTHON_PATCHLEVEL_H}"
      PY_DEF_MICRO
      REGEX "#define[ \t]+PY_MICRO_VERSION.*$")
  STRING(REGEX
        REPLACE "^.*VERSION[ \t]+([0-9]+).*" "\\1"
        PYTHON_VERSION_MAJOR
        "${PY_DEF_MAJOR}")
  STRING(REGEX
        REPLACE "^.*VERSION[ \t]+([0-9]+).*" "\\1"
        PYTHON_VERSION_MINOR
        "${PY_DEF_MINOR}")
  STRING(REGEX
        REPLACE "^.*VERSION[ \t]+([0-9]+).*" "\\1"
        PYTHON_VERSION_PATCH
        "${PY_DEF_MICRO}")
  # now we can find the right SWIG version
  find_program(SWIG_EXECUTABLE NAMES swig2.0 swig)
  # unclean to test for Python 3, but the assumption is that by the
  # time we have Python 4 this issue is long gone
  IF(${PYTHON_VERSION_MAJOR} EQUAL 3 AND 
      ${PYTHON_VERSION_MINOR} GREATER 1)
    FIND_PACKAGE(SWIG 2.0.4 REQUIRED)
  ELSE()
    FIND_PACKAGE(SWIG 2.0 REQUIRED)
  ENDIF()
  INCLUDE(${SWIG_USE_FILE})
ENDIF()

##
## LAPACK (optional, enable by -DENABLE_LAPACK=ON)
##
OPTION (ENABLE_LAPACK "Use LAPACK based solver in levmar" OFF)

IF(ENABLE_LAPACK)
  FIND_PACKAGE(LAPACK)
  IF(LAPACK_FOUND)
    MESSAGE(STATUS "LAPACK found (${LAPACK_LIBRARIES})")
  ELSE(LAPACK_FOUND)
    MESSAGE(STATUS "LAPACK not found, using LU-based solver")
  ENDIF(LAPACK_FOUND)
ENDIF(ENABLE_LAPACK)

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
#INCLUDE_DIRECTORIES( ${VIPSCC_INCLUDE_DIRS} )
#LINK_DIRECTORIES( ${VIPSCC_LIBRARY_DIRS} )

#
# Check build type and set options accordingly
# 
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# allow user to specify lib suffix
set(LIB_SUFFIX "" CACHE STRING "Define suffix of lib directory name (32/64)" )

IF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  MESSAGE("Building in Debug mode, resulting programs will be SLOW")
  ADD_DEFINITIONS(-DDEBUG)
  IF(UNIX)
    ADD_DEFINITIONS(-Wall)
  ENDIF(UNIX)
ENDIF(${CMAKE_BUILD_TYPE} STREQUAL "Debug")

#
# define destination dirs (relative to prefix)
#
SET(HUGINDATADIR share/hugin)
SET(DATADIR share)
SET(BINDIR  bin)

IF (NOT MANDIR AND NOT $ENV{MANDIR} STREQUAL "")
    SET (MANDIR "$ENV{MANDIR}")
ELSEIF(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "NetBSD")
    SET (MANDIR "man")
ELSE(NOT MANDIR AND NOT $ENV{MANDIR} STREQUAL "")
    SET (MANDIR "share/man")
ENDIF(NOT MANDIR AND NOT $ENV{MANDIR} STREQUAL "")

SET(LOCALEDIR share/locale) 
SET(ICONDIR share/icons) 
SET(LINKDIR share/applications)
SET(LIBDIR  lib${LIB_SUFFIX})
SET(INSTALL_XRC_DIR ${CMAKE_INSTALL_PREFIX}/share/hugin/xrc)
SET(INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share/hugin/data)

# setup bundle install prefix
IF (APPLE)
  IF (NOT MAC_SELF_CONTAINED_BUNDLE)
    IF (CMAKE_INSTALL_PREFIX STREQUAL "/opt/local")
      # MacPorts
      SET(INSTALL_OSX_BUNDLE_DIR /Applications/MacPorts CACHE DOCSTRING "Install directory Hugin and HuginStitchProject application bundles")
    ELSE(CMAKE_INSTALL_PREFIX STREQUAL "/opt/local")
      # fink or other custom system.
      SET(INSTALL_OSX_BUNDLE_DIR ${CMAKE_INSTALL_PREFIX}/Applications CACHE DOCSTRING "Install directory Hugin and HuginStitchProject application bundles")
    ENDIF(CMAKE_INSTALL_PREFIX STREQUAL "/opt/local")
    MESSAGE("*** Will install application bundles in ${INSTALL_OSX_BUNDLE_DIR}, set INSTALL_OSX_BUNDLE_DIR to change the location")
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ENDIF (APPLE)

IF (${HUGIN_SHARED} AND UNIX AND NOT APPLE)
  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${LIBDIR}/hugin")
  SET(HUGIN_LIBS_PRIVATE_DIR "1" CACHE INTERNAL "Store internal
    libraries in private subdirectory" )
ELSE (${HUGIN_SHARED} AND UNIX AND NOT APPLE)
  # On OS X files are moved around after make install, do not try to use
  # rpath or CMAKE_INSTALL_NAME_DIR
  SET(HUGIN_LIBS_PRIVATE_DIR "0" CACHE INTERNAL "Store internal
    libraries in private subdirectory" )
ENDIF (${HUGIN_SHARED} AND UNIX AND NOT APPLE)

IF (UNIX)
  IF (${HUGIN_SHARED})
    # use shared internal libraries
    SET(HUGIN_SHARED_LIBS 1)
  ENDIF (${HUGIN_SHARED})
  MESSAGE("-- Using shared internal libraries")
ELSE (UNIX)

  IF (${HUGIN_SHARED})
    ADD_DEFINITIONS(-DVIGRA_DLL)
    ADD_DEFINITIONS(-DOPENEXR_DLL)
    ADD_DEFINITIONS(-DHugin_shared)
    SET(HUGIN_SHARED_LIBS 1)
  ELSE(${HUGIN_SHARED})
    ADD_DEFINITIONS(-DVIGRA_STATIC_LIB)
  ENDIF(${HUGIN_SHARED})

  # Windows installer packaging related stuff
  # unfortunately, CMake/CPack does not support Warsetup directly.
  # prepare files here...
  IF(WIN32)
    # install into place in build-dir
    SET( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/FILES CACHE FILEPATH "install prefix" FORCE)
    INCLUDE (win_bundle)
	
	#Build HuginSetup
	OPTION (BUILD_HUGINSETUP "Build HuginSetup (NSIS Installer)" OFF)
    IF(BUILD_HUGINSETUP)
      FIND_PACKAGE(NSIS REQUIRED)
      IF(NSIS_FOUND)
        MESSAGE(STATUS "NSIS found (${MAKENSIS_EXECUTABLE})")
      ELSE(NSIS_FOUND)
        MESSAGE(STATUS "NSIS not found")
      ENDIF(NSIS_FOUND)
      ADD_CUSTOM_TARGET(HuginSetup COMMAND cd .. & ${MAKENSIS_EXECUTABLE} HuginSetup.nsi WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX})
    ENDIF(BUILD_HUGINSETUP)
  ENDIF(WIN32)

  IF (MSVC)
    # Stop MSVC8 from bitching about the C library
    ADD_DEFINITIONS(/D_CRT_SECURE_NO_DEPRECATE)
    # Stop MSVC9 from bitching about possibly invalid STL usage
    ADD_DEFINITIONS(/D_SCL_SECURE_NO_WARNINGS)
    # Stop MSVC9 from bitching about POSIX names without underscores
    ADD_DEFINITIONS(/D_CRT_NONSTDC_NO_DEPRECATE)
    # Show information about auto-linking library files during build, .e.g.
    # "Linking to lib file: libboost_thread-vc90-mt-s-1_37.lib"
    # This can be used to determine on which boost libraries Hugin depend on
    #ADD_DEFINITIONS(/DBOOST_LIB_DIAGNOSTIC)
    IF(NOT ${HUGIN_SHARED})
    # compile everything for the static C runtime
	FOREACH(TYPE C CXX)
		# makefiles use blank configuration
		FOREACH(CFG "_DEBUG" "_MINSIZEREL" "_RELEASE" "_RELWITHDEBINFO")
			#MESSAGE("Replacing CMAKE_${TYPE}_FLAGS${CFG}: ${CMAKE_${TYPE}_FLAGS${CFG}}")
			SET(NEW_FLAGS "${CMAKE_${TYPE}_FLAGS${CFG}}")
			# fix up static libc flags
			STRING(REPLACE "/MD" "/MT" NEW_FLAGS "${NEW_FLAGS}")
			# *FORCE* to override whats already placed into the cache
			SET(CMAKE_${TYPE}_FLAGS${CFG} "${NEW_FLAGS}" CACHE STRING
				"CMAKE_${TYPE}_FLAGS${CFG} (overwritten to ensure static build)" FORCE)
			#MESSAGE("New       CMAKE_${TYPE}_FLAGS${CFG}: ${CMAKE_${TYPE}_FLAGS${CFG}}")
		ENDFOREACH(CFG)
	ENDFOREACH(TYPE)
    # link only with the static C runtime !! THIS IS A MUST !!
	SET(NEW_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
	# ensure that the flags are not duplicaated on subsequent runs
	STRING(REPLACE " /NODEFAULTLIB:\"MSVCRT.lib\" /NODEFAULTLIB:\"MSVCRTd.lib\"" "" NEW_FLAGS "${NEW_FLAGS}")
	# note that flag names (NODEFAULTLIB) etc.) are case sensitive
    SET( CMAKE_EXE_LINKER_FLAGS
        "${NEW_FLAGS} /NODEFAULTLIB:\"MSVCRT.lib\" /NODEFAULTLIB:\"MSVCRTd.lib\""
       CACHE STRING "for MSVC" FORCE
    )
    ENDIF(NOT ${HUGIN_SHARED})
  ENDIF(MSVC)
ENDIF (UNIX)

IF (APPLE)
   #
   if (${MAC_SELF_CONTAINED_BUNDLE})
      MESSAGE("-- Configuring hugin for fully bundled version. (not yet working)")
   endif(${MAC_SELF_CONTAINED_BUNDLE})
   
   #This namespace workaround is not needed for CMake >= 2.5.x
   ## Add needed Compiler and Linker flags for OSX
      IF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
      	 SET (CMAKE_C_FLAGS:STRING "\"$CMAKE_C_FLAGS\" -flat_namespace -undefined suppress" )
	 SET (CMAKE_CXX_FLAGS:STRING "\"$CMAKE_CXX_FLAGS\" -flat_namespace -undefined suppress" )
	 SET (CMAKE_LDFLAGS:STRING "\"$CMAKE_LD_FLAGS\"" )
	 SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -flat_namespace -undefined suppress" )
	 SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS}  -flat_namespace -undefined suppress" )
	 SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS}  -flat_namespace -undefined suppress" )
         SET (CMAKE_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib" )
	 IF (${CMAKE_BUILD_TYPE} STREQUAL "Release")
		SET (CMAKE_C_FLAGS_RELEASE:STRING " $CMAKE_C_FLAGS_RELEASE ")
		SET (CMAKE_CXX_FLAGS_RELEASE:STRING " $CMAKE_CXX_FLAGS_RELEASE ")
	 ENDIF (${CMAKE_BUILD_TYPE} STREQUAL "Release")
      ENDIF (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
ENDIF (APPLE)


# create config.h file
configure_file(src/hugin_config.h.in.cmake ${CMAKE_BINARY_DIR}/src/hugin_config.h)
configure_file(src/hugin_version.h.in.cmake ${CMAKE_BINARY_DIR}/src/hugin_version.h)

add_subdirectory(src)
add_subdirectory(doc)
add_subdirectory(utils)
# install enfuse droplets and windows installer and everything else in platforms
ADD_SUBDIRECTORY(platforms)

SET(CPACK_PACKAGE_VERSION_MAJOR "${V_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${V_MINOR}")

IF(NOT DISABLE_DPKG)
  SET(CPACK_PACKAGE_VERSION_PATCH "${V_PATCH}.${CPACK_PACKAGE_VERSION_PATCH}")
ELSE()
  SET(CPACK_PACKAGE_VERSION_PATCH "${V_PATCH}.${CPACK_PACKAGE_VERSION_PATCH}.local")
ENDIF()

SET(CPACK_PACKAGE_INSTALL_DIRECTORY "CMake ${V_MAJOR}.${V_MINOR}")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "hugin-${V_MAJOR}.${V_MINOR}.${V_PATCH}")
SET(CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
SET(CPACK_PACKAGE_CONTACT "hugin-ptx@googlegroups.com")
SET(CPACK_SET_DESTDIR "ON")
SET(CPACK_SOURCE_IGNORE_FILES
"/_CPack_Packages/"
"/CMakeCache.txt$"
"\\\\.dir/"
"\\\\.tar\\\\.gz$"
"\\\\.tar\\\\.bz2$"
"\\\\.tar\\\\.Z$"
"\\\\.svn/"
"\\\\.cvsignore$"
"\\\\.swp$"
"~$"
"\\\\.#"
"/#"
"/src/api-doc/"
"/celeste/celeste_standalone$"
"/celeste/Config.h$"
"/celeste/training/celeste_train$"
"/tools/autooptimiser$"
"/tools/fulla$"
"/tools/nona$"
"/tools/vig_optimize$"
"/tools/align_image_stack$"
"/tools/tca_correct$"
"/tools/pto2mk$"
"/tools/hugin_hdrmerge$"
"/ptbatcher/PTBatcher$"
"/ptbatcher/PTBatcherGUI$"
"/hugin_base/test/open_file$"
"/matchpoint/matchpoint$"
"/hugin/hugin$"
"/whitepaper/"
"/lens_calibrate/test_lensFunc$"
"/lens_calibrate/calibrate_lens$"
"/nona_gui/nona_gui$"
"/stitch_project/hugin_stitch_project$"
"/deghosting/deghosting_mask$" 
"/build/"
"/CVS/"
"/cmake_install.cmake$"
"/CMakeOutput.log$"
"/CPackConfig.cmake$"
"/CPackSourceConfig.cmake$"
"/Makefile$"
"/Makefile2$"
"/CMakeFiles/"
"/\\\\.libs/"
"/\\\\.deps/"
"\\\\.o$"
"\\\\.gmo$"
"\\\\.mo$"
"\\\\.a$"
"\\\\.lo$"
"\\\\.so$"
"\\\\.so\\\\.0$" 
"\\\\.so\\\\.0\\\\.0$" 
"\\\\.la$"
"Makefile\\\\.in$"
"/\\\\.hg/"
"/artwork-src/"
"\\\\.hgignore$"
"\\\\.hgtags$"
 )

IF(NOT DISABLE_DPKG)
  # Use dpkg-shlibdeps to get the needed dependences
  SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  # the following part copied from http://www.cmake.org/Wiki/CMake_RPATH_handling
  # use, i.e. don't skip the full RPATH for the build tree
  SET(CMAKE_SKIP_BUILD_RPATH  FALSE)
  # when building, don't use the install RPATH already
  # (but later on when installing)
  SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/hugin")
  SET(CPACK_DEBIAN_PACKAGE_DEBUG ON)

  # add the automatically determined parts of the RPATH
  # which point to directories outside the build tree to the install RPATH
  SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)


  # the RPATH to be used when installing, but only if it's not a system directory
  LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib/hugin" isSystemDir)
  IF("${isSystemDir}" STREQUAL "-1")
     SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib/hugin")
  ENDIF("${isSystemDir}" STREQUAL "-1")

  #this environment will be used by dpkg-shlibdebs to find local created libs
  string(REGEX REPLACE "^/(.*)" "\\1" LDLIBPATH "${CMAKE_INSTALL_RPATH}")
  #message(STATUS "CMAKE_INSTALL_RPATH set to ${CMAKE_INSTALL_RPATH}")
  #message(STATUS "LDLIBPATH = ${LDLIBPATH}")
  set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackEnv.cmake")
  FILE(WRITE "${CPACK_PROJECT_CONFIG_FILE}" "set(ENV{LD_LIBRARY_PATH} \"${LDLIBPATH}\")")

ELSE()
  # incomplete manually set dependencies but we don't really care since this package is not for distribution
  # and whoever use the DISABLE_DPKG option knows what they are doing
  SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libpano13(>=2.9.17)")
ENDIF()

message(STATUS "Current source dir = ${CMAKE_CURRENT_SOURCE_DIR}")
SET(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_SOURCE_DIR}/Debian/postinst"
				       "${CMAKE_CURRENT_SOURCE_DIR}/Debian/shlibs.local")
INCLUDE(CPack)

##
## Uninstall Taget
##
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


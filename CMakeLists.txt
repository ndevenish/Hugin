
# require at least cmake 2.4
cmake_minimum_required(VERSION 2.4)

if(COMMAND cmake_policy)
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

include(HuginMacros)
include(CheckIncludeFiles)

##  global setup
project(hugin)

# version
set(V_MAJOR 2009)
set(V_MINOR 1)
set(V_PATCH 0)

#
# Check for SVN and get current revision
#
IF (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
  IF(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    EXECUTE_PROCESS(COMMAND "subwcrev" "${PROJECT_SOURCE_DIR}" OUTPUT_VARIABLE SVN_REV)
    STRING(REGEX REPLACE ".* committed at revision ([0-9]+).*" "\\1" SVN_REV "${SVN_REV}")
    IF(SVN_REV)
      SET(HUGIN_WC_REVISION "${SVN_REV}" CACHE STRING "" FORCE)
      # WINDOWS: messages are obstructive pop-ups, use them only for debug
      # MESSAGE("-- Current SVN revision is ${HUGIN_WC_REVISION}")
    ELSE(SVN_REV)
      SET(HUGIN_WC_REVISION "" CACHE STRING "set SVN revision.")
    ENDIF(SVN_REV)
  ELSE(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    FIND_PACKAGE(Subversion REQUIRED)
    IF(Subversion_FOUND)
      Subversion_WC_INFO(${PROJECT_SOURCE_DIR} HUGIN)
      MESSAGE("-- Current SVN revision is ${HUGIN_WC_REVISION}")
    ELSE(Subversion_FOUND)
      MESSAGE(SEND_ERROR "Could not determine SVN revision")
    ENDIF(Subversion_FOUND)
  ENDIF(WIN32 AND NOT CMAKE_CROSSCOMPILING)
  SET(HUGIN_PACKAGE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH}.${HUGIN_WC_REVISION})
  SET(HUGIN_DEVELOPMENT_VERSION 1)
  FILE(WRITE ${PROJECT_SOURCE_DIR}/rev.txt "${HUGIN_WC_REVISION}")
ELSE (EXISTS ${PROJECT_SOURCE_DIR}/.svn)
  SET(HUGIN_PACKAGE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH})
  SET(HUGIN_WC_REVISION 0)
  FILE(READ "${PROJECT_SOURCE_DIR}/rev.txt" HUGIN_WC_REVISION)
  MESSAGE("-- Current SVN revision is ${HUGIN_WC_REVISION}")
ENDIF (EXISTS ${PROJECT_SOURCE_DIR}/.svn)

# version for internal libraries.
set(HUGIN_LIB_VERSION 0.0)

# set include path for internal libraries before using finding and initializing
# external dependencies. This makes sure that "our" include paths are specified
# before any system includes

include_directories( ${CMAKE_BINARY_DIR}/src )
include_directories( ${CMAKE_SOURCE_DIR}/src/hugin_base )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign )
include_directories( ${CMAKE_SOURCE_DIR}/src/foreign/vigra )
include_directories( ${CMAKE_SOURCE_DIR}/src/celeste )

#
# find dependencies
#
IF (UNIX)
FIND_PACKAGE(PkgConfig)
ENDIF (UNIX)

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
  SET(wxWidgets_ROOT_DIR ${SOURCE_BASE_DIR}/wxWidgets-2.8.10)
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

SET(wxWidgets_USE_LIBS base core xrc html xml adv gl net)
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

FIND_PACKAGE(PANO13)
FIND_PACKAGE(PANO12)

IF(PANO13_FOUND)
  SET(PANO_LIBRARIES ${PANO13_LIBRARIES})
  SET(PANO_INCLUDE_DIR ${PANO13_INCLUDE_DIR})
ELSEIF(PANO12_FOUND)
  SET(PANO_LIBRARIES ${PANO12_LIBRARIES})
  SET(PANO_INCLUDE_DIR ${PANO12_INCLUDE_DIR})
ELSE(PANO13_FOUND)
  MESSAGE(SEND_ERROR "Either libpano13 or libpano12 are required")
ENDIF(PANO13_FOUND) 
INCLUDE_DIRECTORIES(${PANO_INCLUDE_DIR})

##
## Boost
##
IF(WIN32)
  SET(Boost_USE_STATIC_LIBS ON)
  # If BOOST_ROOT was defined in the environment, use it. 
  if (NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "") 
    set(BOOST_ROOT $ENV{BOOST_ROOT}) 
  else(NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "") 
    set(BOOST_ROOT ${SOURCE_BASE_DIR}/boost_1_39_0) 
   endif(NOT BOOST_ROOT AND NOT $ENV{BOOST_ROOT} STREQUAL "") 
ENDIF(WIN32)
FIND_PACKAGE(Boost COMPONENTS thread)
IF (NOT Boost_FOUND)
  MESSAGE(FATAL_ERROR "boost thread library not found. If it is installed with a\nunrecognized suffix, specify it with -DBOOST_LIBRARYDIR=<librarydir> and -DBOOST_INCLUDEDIR=<includedir>")
ENDIF (NOT Boost_FOUND)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
  MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

##
## LAPACK (optional, enable by -DENABLE_LAPACK=YES
##

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
set(HUGIN_SHARED "1" CACHE STRING "Build shared internal libraries" )

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
      SET(INSTALL_OSX_BUNDLE_DIR ${CMAKE_INSTALL_PREFIX}/Application CACHE DOCSTRING "Install directory Hugin and HuginStitchProject application bundles")
    ENDIF(CMAKE_INSTALL_PREFIX STREQUAL "/opt/local")
    MESSAGE("*** Will install application bundles in ${INSTALL_OSX_BUNDLE_DIR}, set INSTALL_OSX_BUNDLE_DIR to change the location")
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ENDIF (APPLE)


IF (UNIX)
  IF (${HUGIN_SHARED})
    # use shared internal libraries
    SET(HUGIN_SHARED_LIBS 1)
  ENDIF (${HUGIN_SHARED})
  MESSAGE("-- Using shared internal libraries")
ELSE (UNIX)

  # no shared libs DLL on windows. The code is just
  # not set up for that..
  ADD_DEFINITIONS(-DVIGRA_STATIC_LIB)

  # Windows installer packaging related stuff
  # unfortunately, CMake/CPack does not support Warsetup directly.
  # prepare files here...
  IF(WIN32)
    # install into place in build-dir
    SET( CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/FILES CACHE FILEPATH "install prefix" FORCE)
    INCLUDE (win_bundle)
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
# install enfuse droplets and windows installer and everything else in platforms
ADD_SUBDIRECTORY(platforms)

SET(CPACK_PACKAGE_VERSION_MAJOR "${V_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${V_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${V_PATCH}")
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
"/ptbatcher/PTBatcher$"
"/ptbatcher/PTBatcherGUI$"
"/hugin_base/test/open_file$"
"/matchpoint/matchpoint$"
"/hugin/hugin$"
"/nona_gui/nona_gui$"
"/stitch_project/hugin_stitch_project$"
"/deghosting/hugin_hdrmerge$" 
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
 )
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


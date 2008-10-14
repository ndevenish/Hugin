
# require at least cmake 2.4
cmake_minimum_required(VERSION 2.4)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

include(HuginMacros)
include(CheckIncludeFiles)

##  global setup
project(hugin)

# version
set(V_MAJOR 0)
set(V_MINOR 8)
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
  # TODO: at each release, set current SVN revision here!
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

  # search boost there
  SET(BOOST_DIR_SEARCH_USER ${SOURCE_BASE_DIR})
  # wxWidgets
  SET(wxWidgets_ROOT_DIR ${SOURCE_BASE_DIR}/wxMSW-2.8.9)
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

SET(wxWidgets_USE_LIBS base core xrc html xml adv gl)
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
INCLUDE_DIRECTORIES(${ZLIB_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${PNG_INCLUDE_DIR})


FIND_PACKAGE(OpenEXR REQUIRED)
include_directories(${OPENEXR_INCLUDE_DIR})

FIND_PACKAGE(Exiv2)
IF(EXIV2_FOUND)
  INCLUDE_DIRECTORIES(${EXIV2_INCLUDE_DIR})
ENDIF(EXIV2_FOUND)

## OpenGL and GLEW

FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLEW)
IF(NOT OPENGL_FOUND)
  MESSAGE("OpenGL was not found, hugin disabled")
ENDIF(NOT OPENGL_FOUND)
IF(GLEW_FOUND)
  INCLUDE_DIRECTORIES(${OPENGL_INCLUDE_DIR})
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


SET(Boost_LIB_SUFFIX_DEBUG gd)
FIND_PACKAGE(Boost REQUIRED thread)
IF (NOT Boost_thread_FOUND)
  MESSAGE(FATAL_ERROR "boost thread library not found. If it is installed with a\nunrecognized suffix, specify it with -DBoost_LIB_SUFFIX=<suffix>")
ENDIF (NOT Boost_thread_FOUND)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

FIND_PACKAGE(MSGFMT REQUIRED)
IF(MSGFMT_FOUND)
  MESSAGE(STATUS "Program msgfmt found (${MSGFMT_EXECUTABLE})")
ENDIF(MSGFMT_FOUND)

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
      	 SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -flat_namespace -undefined suppress" )
	 SET (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -flat_namespace -undefined suppress" )
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
"/tools/align_image_stack$"
"/tools/tca_correct$"
"/tools/pto2mk$"
"/PTBatcher/PTBatcher$"
"/PTBatcherGUI/PTBatcherGUI$"
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
"\\\\.a$"
"\\\\.lo$"
"\\\\.so$"
"\\\\.so\\\\.0$" 
"\\\\.so\\\\.0\\\\.0$" 
"\\\\.la$"
"Makefile\\\\.in$"
 )
INCLUDE(CPack)

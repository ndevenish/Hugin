#
#trimmed cmake list for installer
#

# require at least cmake 2.6
cmake_minimum_required(VERSION 2.6 FATAL_ERROR )

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules )

##  global setup
project(windowsSetup)

# version
set(V_MAJOR 2010)
set(V_MINOR 3)
set(V_PATCH 0)

# hugin directory
SET (HUGIN_ROOT_DIR "" CACHE PATH
  "set Hugin directory"
)
IF(NOT HUGIN_ROOT_DIR) 
  MESSAGE(FATAL_ERROR "Need to set HUGIN_ROOT_DIR manually")
ENDIF(NOT HUGIN_ROOT_DIR)


#
# Check for mercurial and get current revision
#

IF(EXISTS ${HUGIN_ROOT_DIR}/.hg)
  FIND_PROGRAM(_hg hg)
  IF(NOT ${_hg} MATCHES "-NOTFOUND")
    EXECUTE_PROCESS(COMMAND ${_hg} summary WORKING_DIRECTORY "${HUGIN_ROOT_DIR}" OUTPUT_VARIABLE _release OUTPUT_STRIP_TRAILING_WHITESPACE)
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
ENDIF()

# version to display
IF (HUGIN_DEVELOPMENT_VERSION EQUAL 1)
  set(DISPLAY_VERSION "Pre-Release ${V_MAJOR}.${V_MINOR}.${V_PATCH}.${HUGIN_WC_REVISION}")
ELSE (HUGIN_DEVELOPMENT_VERSION EQUAL 1)
  set(DISPLAY_VERSION "${V_MAJOR}.${V_MINOR}.${V_PATCH}.${HUGIN_WC_REVISION}")
ENDIF (HUGIN_DEVELOPMENT_VERSION EQUAL 1)

#
# New
#

# Windows installer settings
IF(WIN32)
  # Set install directory
  IF(CMAKE_SIZEOF_VOID_P EQUAL 4) #32bit
    SET(WINDOWS_INSTALL_DIRECTORY "$PROGRAMFILES\\Hugin")
	SET(WINDOWS_ARCH_TYPE "32")
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 4)
  IF(CMAKE_SIZEOF_VOID_P EQUAL 8) #64bit
    SET(WINDOWS_INSTALL_DIRECTORY "$PROGRAMFILES64\\Hugin")
	SET(WINDOWS_ARCH_TYPE "64")
  ENDIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
  
  # Toggle Development Version Warning
  IF(HUGIN_DEVELOPMENT_VERSION EQUAL 1)
    SET(DEV_WARNING_TOGGLE "$(TEXT_PreReleaseWelcomePage)")
  ELSE(HUGIN_DEVELOPMENT_VERSION EQUAL 1)
    SET(DEV_WARNING_TOGGLE "$(TEXT_WelcomePage)")
  ENDIF(HUGIN_DEVELOPMENT_VERSION EQUAL 1)
  
  # Configure Files
  CONFIGURE_FILE(HuginSetup.nsi HuginSetup.nsi)
  
  # Install Files
  SET(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR} CACHE FILEPATH "install prefix" FORCE)
  INSTALL(FILES HuginSetup_common.nsh
    HuginSetup_x32.nsi
    HuginSetup_x64.nsi
    DESTINATION ${CMAKE_INSTALL_PREFIX})
  add_subdirectory(CPGenerators)
  add_subdirectory(Functions)
  add_subdirectory(Graphics)
  add_subdirectory(Languages)
  add_subdirectory(Licenses)
  add_subdirectory(Plugins)
ENDIF(WIN32)
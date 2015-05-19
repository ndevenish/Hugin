# - Find VIGRA 
# Added to support Windows build but should work anywhere.
# After looking in UNIX standard places, tries wxWidgets build 
# tree, which should have this package.
#
# 
# reads cache variables
# defines cache variables
#  VIGRA_INCLUDE_DIR, where to find headers
#  VIGRA_LIBRARIES, list of release link libraries
#  VIGRA_FOUND, If != "YES", error out as VIGRA is required

SET( VIGRA_FOUND "NO" )

IF(WIN32)
  FIND_PATH(VIGRA_INCLUDE_DIR vigra/gaborfilter.hxx 
      PATHS ${SOURCE_BASE_DIR}/vigra/include
  )

  FIND_LIBRARY_WITH_DEBUG(VIGRA_LIBRARIES
    WIN32_DEBUG_POSTFIX d
    NAMES vigraimpex libvigraimpex 
    PATHS
    ${VIGRA_ROOT_PATH}
    ${VIGRA_ROOT_PATH}/Release
    ${VIGRA_ROOT_PATH}/lib
    ${SOURCE_BASE_DIR}/vigra/lib
    )
ELSE(WIN32)
  FIND_PATH(VIGRA_INCLUDE_DIR vigra/gaborfilter.hxx
    /usr/local/include
    /usr/include
    /opt/local/include
  )

  FIND_LIBRARY(VIGRA_LIBRARIES
    NAMES vigraimpex libvigraimpex 
    PATHS /usr/lib /usr/local/lib /opt/local/lib
 ) 
ENDIF(WIN32)


IF (VIGRA_INCLUDE_DIR AND VIGRA_LIBRARIES)
   SET(VIGRA_FOUND TRUE)
ENDIF (VIGRA_INCLUDE_DIR AND VIGRA_LIBRARIES)

IF (VIGRA_FOUND)
  MESSAGE(STATUS "Found VIGRA: ${VIGRA_LIBRARIES}")
  FIND_FILE(
    VIGRA_CONFIG_VERSION_HXX
    NAMES configVersion.hxx config_version.hxx
    PATHS "${VIGRA_INCLUDE_DIR}/vigra/"
  )
  IF(NOT VIGRA_CONFIG_VERSION_HXX)
    MESSAGE(FATAL_ERROR "Could not find vigra/configVersion.hxx or vigra/config_version.hxx. Your vigra installation seems to be corrupt.")
  ENDIF()
  FILE(STRINGS "${VIGRA_CONFIG_VERSION_HXX}" VIGRA_VERSION_HXX REGEX ".*#define +VIGRA_VERSION +\"")
  STRING(REGEX REPLACE ".*#define +VIGRA_VERSION +\"([.0-9]+).*" "\\1" VIGRA_VERSION "${VIGRA_VERSION_HXX}")
  IF(${VIGRA_VERSION} VERSION_EQUAL VIGRA_FIND_VERSION OR ${VIGRA_VERSION} VERSION_GREATER VIGRA_FIND_VERSION)
    SET(VIGRA_VERSION_CHECK TRUE)
    MESSAGE(STATUS "VIGRA version: ${VIGRA_VERSION}")
  ELSE()
    MESSAGE(FATAL_ERROR 
        "VIGRA lib is too old.\nHugin requires at least version 1.9.0, but found version ${VIGRA_VERSION}."
    )
  ENDIF()

  # check if vigraimpex is linked against OpenEXR
  IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    FIND_PROGRAM(LDD_EXECUTABLE ldd)
    IF(LDD_EXECUTABLE)
      EXECUTE_PROCESS(
        COMMAND ${LDD_EXECUTABLE} ${VIGRA_LIBRARIES}
        OUTPUT_VARIABLE OUTPUT_LDD_VIGRA
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      STRING(TOLOWER "${OUTPUT_LDD_VIGRA}" OUTPUT_LDD_VIGRA_LOWER)
      IF(NOT "${OUTPUT_LDD_VIGRA_LOWER}" MATCHES "libilmimf")
        MESSAGE(FATAL_ERROR "Libvigraimpex found. But vigraimpex seems to compiled without OpenEXR support. OpenEXR support is required for Hugin.")
      ENDIF()
    ELSE()
      MESSAGE(FATAL_ERROR "Could not check libraries on which vigraimpex depends. (ldd is missing)") 
    ENDIF()
  ENDIF()
ELSE (VIGRA_FOUND)
	MESSAGE(FATAL_ERROR "Could not find VIGRA")
ENDIF (VIGRA_FOUND)


MARK_AS_ADVANCED(
  VIGRA_LIBRARIES
  VIGRA_INCLUDE_DIR
  )


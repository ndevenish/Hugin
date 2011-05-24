# - Find PANO13 headers and libraries
# modified for Hugin 0.7 Windows build 02Nov2007 TKSharpless
# reads cache variable
#  SOURCE_BASE_DIR -- directory that contains hugin source root
# defines cache vars
#  PANO13_INCLUDE_DIR, where to find pano13/panorama.h, etc.
#  PANO13_LIBRARIES, release link library list.
#  PANO13_DEBUG_LIBRARIES, debug ditto.
#  PANO13_FOUND, If != "YES", do not try to use PANO13.


## NOTE the form "pano13/panorama.h" is used in #includes in 
## some hugin source files, so we are stuck with that.
FIND_PATH(PANO13_INCLUDE_DIR pano13/panorama.h
  /usr/local/include
  /usr/include
  ${SOURCE_BASE_DIR}/libpano13/include
  ${SOURCE_BASE_DIR}/libpano
  ${SOURCE_BASE_DIR}
)


FIND_LIBRARY(PANO13_LIBRARIES
  NAMES pano13
  PATHS ${SYSTEM_LIB_DIRS} 
        "${PANO13_INCLUDE_DIR}/pano13/Release LIB CMD"
        "${PANO13_INCLUDE_DIR}/pano13/Release CMD/Win32"
        ${PANO13_INCLUDE_DIR}/pano13/Release
        ${SOURCE_BASE_DIR}/libpano13/lib
        ${SOURCE_BASE_DIR}/pano13
  )

IF(PANO13_INCLUDE_DIR)
  IF(PANO13_LIBRARIES)
    FILE(STRINGS "${PANO13_INCLUDE_DIR}/pano13/version.h" PANO13_VERSION_H REGEX "#define VERSION")
    STRING(REGEX REPLACE ".*#define +VERSION +\"([.0-9]+).*" "\\1" PANO13_VERSION "${PANO13_VERSION_H}")
    STRING(REGEX REPLACE "([^.]+)\\.([^.]+)\\.([^.]+)" "\\1" PANO13_VERSION_MAJOR "${PANO13_VERSION}")
    STRING(REGEX REPLACE "([^.]+)\\.([^.]+)\\.([^.]+)" "\\2" PANO13_VERSION_MINOR "${PANO13_VERSION}")
    STRING(REGEX REPLACE "([^.]+)\\.([^.]+)\\.([^.]+)" "\\3" PANO13_VERSION_PATCH "${PANO13_VERSION}")
    # version comparison stuff. Very ugly for multiple reasons:
    # + VERSION_GREATER is not available in cmake 2.6.0.
    # + cmake does not allow combining AND with OR like in e.g.
    #   "if(2 LESS 3 AND (3 LESS 4 OR 5 LESS 6))".
    # + At least cmake 2.6.0 does not set PANO13_FIND_VERSION_* to 0 by default.
    IF(NOT DEFINED PANO13_FIND_VERSION_MAJOR OR "${PANO13_VERSION_MAJOR}" GREATER "${PANO13_FIND_VERSION_MAJOR}")
      SET( PANO13_FOUND "YES" )
    ELSEIF("${PANO13_VERSION_MAJOR}" EQUAL "${PANO13_FIND_VERSION_MAJOR}")
      IF(NOT DEFINED PANO13_FIND_VERSION_MINOR OR "${PANO13_VERSION_MINOR}" GREATER "${PANO13_FIND_VERSION_MINOR}")
        SET( PANO13_FOUND "YES" )
      ELSEIF("${PANO13_VERSION_MINOR}" EQUAL "${PANO13_FIND_VERSION_MINOR}")
        IF(NOT DEFINED PANO13_FIND_VERSION_PATCH OR NOT "${PANO13_VERSION_PATCH}" LESS "${PANO13_FIND_VERSION_PATCH}")
          SET( PANO13_FOUND "YES" )
        ENDIF()
      ENDIF()
    ENDIF()
  ENDIF(PANO13_LIBRARIES)
ENDIF(PANO13_INCLUDE_DIR)
 
IF(PANO13_FOUND)
  SET(PANO13_VERSION_COUNT 3)
  IF(NOT PANO13_FIND_QUIETLY)
    MESSAGE(STATUS "libpano13 version: ${PANO13_VERSION} major ${PANO13_VERSION_MAJOR} minor ${PANO13_VERSION_MINOR} patch ${PANO13_VERSION_PATCH}")
  ENDIF(NOT PANO13_FIND_QUIETLY)
  FIND_LIBRARY( PANO13_DEBUG_LIBRARIES
    NAMES Panotools pano13d pano13
    PATHS ${SYSTEM_LIB_DIRS}
	  "${PANO13_INCLUDE_DIR}/pano13/Debug LIB CMD"
	  "${PANO13_INCLUDE_DIR}/pano13/Debug CMD/Win32"
	  ${PANO13_INCLUDE_DIR}/pano13/Debug
	  ${SOURCE_BASE_DIR}/pano13/lib
	  ${SOURCE_BASE_DIR}/pano13
  )
ELSE(PANO13_FOUND)
  IF(PANO13_FIND_REQUIRED)
    IF(PANO13_VERSION)
      MESSAGE(FATAL_ERROR "libpano13 version: ${PANO13_FIND_VERSION} required, ${PANO13_VERSION} found")
    ELSE(PANO13_VERSION)
      MESSAGE(FATAL_ERROR "libpano13 not found")
    ENDIF(PANO13_VERSION)
  ELSE(PANO13_FIND_REQUIRED)
    IF(PANO13_VERSION)
      MESSAGE(STATUS "libpano13 version: ${PANO13_FIND_VERSION} required, ${PANO13_VERSION} found")
    ELSE(PANO13_VERSION)
      MESSAGE(STATUS "libpano13 not found")
    ENDIF(PANO13_VERSION)
  ENDIF(PANO13_FIND_REQUIRED)
ENDIF(PANO13_FOUND)

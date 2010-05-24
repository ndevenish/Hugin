# - Modified Find zlib
# Added to support Windows build but should work anywhere.
#
# Find the native ZLIB includes and library
#
#  ZLIB_INCLUDE_DIR - where to find zlib.h, etc.
#  ZLIB_LIBRARIES   - List of libraries when using zlib.
#  ZLIB_FOUND       - True if zlib found.

FIND_PATH(ZLIB_INCLUDE_DIR zlib.h
  /usr/local/include
  /usr/include
  ${SOURCE_BASE_DIR}/zlib
)

FIND_LIBRARY(ZLIB_LIBRARIES
  NAMES z zlib zlib1
  PATHS /usr/lib /usr/local/lib ${SOURCE_BASE_DIR}/zlib
)

IF(WIN32)
  FIND_FILE(ZLIB_DLL
    NAMES zlib1.dll 
    PATHS ${SOURCE_BASE_DIR}/zlib
    NO_SYSTEM_ENVIRONMENT_PATH
  )
ENDIF(WIN32)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ZLIB  DEFAULT_MSG  ZLIB_INCLUDE_DIR ZLIB_LIBRARIES )

IF (ZLIB_FOUND)
	MESSAGE(STATUS "Found zlib include dir: ${ZLIB_INCLUDE_DIR}")
    MESSAGE(STATUS "Found zlib library: ${ZLIB_LIBRARIES}")
ELSE (ZLIB_FOUND)
	MESSAGE(FATAL_ERROR "Could not find zlib")
ENDIF (ZLIB_FOUND)

MARK_AS_ADVANCED(ZLIB_LIBRARIES ZLIB_INCLUDE_DIR)

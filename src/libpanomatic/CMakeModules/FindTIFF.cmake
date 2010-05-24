# - Modified Find TIFF for Hugin 
# Added to support Windows build but should work anywhere.
#
# defines cache variables
#  TIFF_INCLUDE_DIR, where to find headers
#  TIFF_LIBRARIES, list of link libraries for release
#  TIFF_FOUND, If != "YES", do not try to use TIFF.

FIND_PATH(TIFF_INCLUDE_DIR tiff.h
  /usr/local/include
  /usr/include
  ${SOURCE_BASE_DIR}/tiff-4.0.0beta5/libtiff
  ${SOURCE_BASE_DIR}/tiff-3.8.2/libtiff
)

# on windows libtiff_i is the shared import library, it should be tested first, so it can found
# libtiff is the static library on windows
FIND_LIBRARY(TIFF_LIBRARIES
  NAMES tiff libtiff_i libtiff
  PATHS /usr/lib /usr/local/lib
        ${SOURCE_BASE_DIR}/tiff-4.0.0beta5/libtiff
        ${SOURCE_BASE_DIR}/tiff-3.8.2/libtiff
)

IF(WIN32)
  FIND_FILE(TIFF_DLL
    NAMES libtiff.dll 
    PATHS ${SOURCE_BASE_DIR}/tiff-4.0.0beta5/libtiff
  )
ENDIF(WIN32)


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(TIFF  DEFAULT_MSG  TIFF_LIBRARIES  TIFF_INCLUDE_DIR)

IF (TIFF_FOUND)
	MESSAGE(STATUS "Found TIFF include dir: ${TIFF_INCLUDE_DIR}")
    MESSAGE(STATUS "Found TIFF library: ${TIFF_LIBRARIES}")
ELSE (TIFF_FOUND)
	MESSAGE(FATAL_ERROR "Could not find libtiff")
ENDIF (TIFF_FOUND)

MARK_AS_ADVANCED(TIFF_INCLUDE_DIR TIFF_LIBRARIES)


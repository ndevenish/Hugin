# - Modified Find JPEG for Hugin
# Added to support Windows build but should work anywhere.
#
# defines cache variables
#  JPEG_INCLUDE_DIR, where to find headers
#  JPEG_LIBRARIES, list of release link libraries
#  JPEG_FOUND, If != "YES", do not try to use JPEG

FIND_PATH(JPEG_INCLUDE_DIR jpeglib.h
  /usr/local/include
  /usr/include
  ${SOURCE_BASE_DIR}/jpeg-8
)

FIND_LIBRARY(JPEG_LIBRARIES
  NAMES jpeg libjpeg 
  PATHS /usr/lib /usr/local/lib ${SOURCE_BASE_DIR}/jpeg-8/Release
)

IF(WIN32)
  FIND_FILE(JPEG_DLL
    NAMES jpeg.dll 
    PATHS ${SOURCE_BASE_DIR}/jpeg-8/Release
  )
ENDIF(WIN32)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(JPEG  DEFAULT_MSG  JPEG_LIBRARIES JPEG_INCLUDE_DIR )

IF (JPEG_FOUND)
	MESSAGE(STATUS "Found JPEG include dir: ${JPEG_INCLUDE_DIR}")
    MESSAGE(STATUS "Found JPEG library: ${JPEG_LIBRARIES}")
ELSE (JPEG_FOUND)
	MESSAGE(FATAL_ERROR "Could not find libjpeg")
ENDIF (JPEG_FOUND)

MARK_AS_ADVANCED(JPEG_INCLUDE_DIR JPEG_LIBRARIES )


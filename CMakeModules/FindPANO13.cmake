# - Find PANO13 library
# Find the native PANO13 includes and library
# This module defines
#  PANO13_INCLUDE_DIR, where to find pano13/panorama.h, etc.
#  PANO13_LIBRARIES, libraries to link against to use PANO13.
#  PANO13_FOUND, If false, do not try to use PANO13.
# also defined, but not for general use are
#  PANO13_LIBRARY, where to find the PANO13 library.

FIND_PATH(PANO13_INCLUDE_DIR tiff.h
  /usr/local/include
  /usr/include
)

SET(PANO13_NAMES ${PANO13_NAMES} pano13)
FIND_LIBRARY(PANO13_LIBRARY
  NAMES ${PANO13_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

IF(PANO13_INCLUDE_DIR)
  IF(PANO13_LIBRARY)
    SET( PANO13_FOUND "YES" )
    SET( PANO13_LIBRARIES ${PANO13_LIBRARY} )
  ENDIF(PANO13_LIBRARY)
ENDIF(PANO13_INCLUDE_DIR)


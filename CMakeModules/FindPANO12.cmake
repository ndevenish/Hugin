# - Find PANO12 library
# Find the native PANO12 includes and library
# This module defines
#  PANO12_INCLUDE_DIR, where to find pano12/panorama.h, etc.
#  PANO12_LIBRARIES, libraries to link against to use PANO12.
#  PANO12_FOUND, If false, do not try to use PANO12.
# also defined, but not for general use are
#  PANO12_LIBRARY, where to find the PANO12 library.

FIND_PATH(PANO12_INCLUDE_DIR pano12/panorama.h
  /usr/local/include
  /usr/include
)

SET(PANO12_NAMES ${PANO12_NAMES} pano12)
FIND_LIBRARY(PANO12_LIBRARY
  NAMES ${PANO12_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

IF(PANO12_INCLUDE_DIR)
  IF(PANO12_LIBRARY)
    SET( PANO12_FOUND "YES" )
    SET( PANO12_LIBRARIES ${PANO12_LIBRARY} )
  ENDIF(PANO12_LIBRARY)
ENDIF(PANO12_INCLUDE_DIR)


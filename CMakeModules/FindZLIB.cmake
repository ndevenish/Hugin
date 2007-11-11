# - Find zlib
# Find the native ZLIB includes and library
#
#  ZLIB_INCLUDE_DIR - where to find zlib.h, etc.
#  ZLIB_LIBRARIES   - List of libraries when using zlib.
#  ZLIB_FOUND       - True if zlib found.

# - Find TIFF for Hugin 0.7 01Nov2007 TKSharpless
# Added to support Windows build but should work anywhere.
# After looking in UNIX standard places, tries wxWidgets build 
# tree, which should have this package.
#
# reads cache variables
#  wxWidgets_ROOT_DIR
#  wxWidgets_LIB_DIR
# 

IF (ZLIB_INCLUDE_DIR)
  # Already in cache, be silent
  SET(ZLIB_FIND_QUIETLY TRUE)
ENDIF (ZLIB_INCLUDE_DIR)

FIND_PATH(ZLIB_INCLUDE_DIR zlib.h
  /usr/local/include
  /usr/include
  ${wxWidgets_ROOT_DIR}/src/zlib
)

SET(ZLIB_NAMES z zlib zdll wxzlib)
FIND_LIBRARY(ZLIB_LIBRARY
  NAMES ${ZLIB_NAMES}
  PATHS /usr/lib /usr/local/lib
  ${wxWidgets_ROOT_DIR}/src/zlib
)

SET(ZLIB_DEBUG_NAMES zd zlibd zdlld wxzlibd)
FIND_LIBRARY(ZLIB_DEBUG_LIBRARY
  NAMES ${ZLIB_DEBUG_NAMES}
  PATHS /usr/lib /usr/local/lib
)

IF (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
    SET(ZLIB_FOUND TRUE)
    SET( ZLIB_LIBRARIES ${ZLIB_LIBRARY} )
    IF (ZLIB_DEBUG_LIBRARY)
        SET(ZLIB_DEBUG_LIBRARIES ${ZLIB_DEBUG_LIBRARY})
    ENDIF(ZLIB_DEBUG_LIBRARY)
ELSE (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
   SET(ZLIB_FOUND FALSE)
   SET( ZLIB_LIBRARIES )
ENDIF (ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)

IF (ZLIB_FOUND)
   IF (NOT ZLIB_FIND_QUIETLY)
      MESSAGE(STATUS "Found ZLIB: ${ZLIB_LIBRARY}")
   ENDIF (NOT ZLIB_FIND_QUIETLY)
ELSE (ZLIB_FOUND)
   IF (ZLIB_FIND_REQUIRED)
      MESSAGE(STATUS "Looked for Z libraries named ${ZLIBS_NAMES}.")
      MESSAGE(FATAL_ERROR "Could NOT find z library")
   ENDIF (ZLIB_FIND_REQUIRED)
ENDIF (ZLIB_FOUND)

MARK_AS_ADVANCED(
  ZLIB_LIBRARY
  ZLIB_DEBUG_LIBRARY
  ZLIB_INCLUDE_DIR
  )

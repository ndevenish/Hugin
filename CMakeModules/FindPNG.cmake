# - Find PNG for Hugin 0.7 01Nov2007 TKSharpless
# Added to support Windows build but should work anywhere.
# After looking in UNIX standard places, tries wxWidgets build 
# tree, which should have this package.
#
# Call FIND_PACKAGE(wxWidgets REQUIRED) before calling this!
# 
# reads cache variables
#  wxWidgets_ROOT_DIR
#  wxWidgets_LIB_DIR
# defines cache variables
#  PNG_INCLUDE_DIR, where to find headers
#  PNG_LIBRARIES, list of release link libraries.
#  PNG_DEBUG_LIBRARIES, list of release link libraries.
#  PNG_FOUND, If != "YES", do not try to use PNG.
# None of the above will be defined unless ZLIB can be found

INCLUDE(FindZLIB)

SET(PNG_FOUND "NO")

IF (ZLIB_FOUND)
  FIND_PATH(PNG_PNG_INCLUDE_DIR png.h
    /usr/local/include
    /usr/include
    ${wxWidgets_ROOT_DIR}/src/png
  )

  FIND_LIBRARY(PNG_LIBRARY
    NAMES png libpng wxpng
    PATHS /usr/lib /usr/local/lib ${wxWidgets_LIB_DIR}
  )

  IF(PNG_PNG_INCLUDE_DIR AND PNG_LIBRARY)
      SET(PNG_INCLUDE_DIR ${PNG_PNG_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} )
      SET(PNG_LIBRARIES ${PNG_LIBRARY} ${ZLIB_LIBRARY})

      SET( PNG_FOUND "YES" )
      FIND_LIBRARY( PNG_DEBUG_LIBRARY
        NAMES pngd wxpngd
        PATHS /usr/lib /usr/local/lib ${wxWidgets_LIB_DIR}
      )
      SET(PNG_DEBUG_LIBRARIES ${PNG_DEBUGLIBRARY} ${ZLIB_DEBUG_LIBRARY})
  ENDIF(PNG_PNG_INCLUDE_DIR AND PNG_LIBRARY)
ENDIF(ZLIB_FOUND)

IF (PNG_FOUND)
  IF (NOT PNG_FIND_QUIETLY)
    MESSAGE(STATUS "Found PNG: ${PNG_LIBRARY}")
  ENDIF (NOT PNG_FIND_QUIETLY)
ELSE (PNG_FOUND)
  IF (PNG_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Could not find PNG library")
  ENDIF (PNG_FIND_REQUIRED)
ENDIF (PNG_FOUND)

MARK_AS_ADVANCED(PNG_PNG_INCLUDE_DIR PNG_LIBRARY )


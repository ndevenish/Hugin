# - Find TIFF for Hugin 0.7 01Nov2007 TKSharpless
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
#  TIFF_INCLUDE_DIR, where to find headers
#  TIFF_LIBRARIES, list of link libraries for release
#  TIFF_DEBUG_LIBRARIES ditto for debug
#  TIFF_FOUND, If != "YES", do not try to use TIFF.

FIND_PATH(TIFF_INCLUDE_DIR tiff.h
  /usr/local/include
  /usr/include
  ${wxWidgets_ROOT_DIR}/src/tiff
)

FIND_LIBRARY( TIFF_LIBRARIES
  NAMES tiff wxtiff
  PATHS /usr/lib /usr/local/lib ${wxWidgets_LIB_DIR}
)

IF(TIFF_INCLUDE_DIR)
  IF(TIFF_LIBRARIES)
    SET( TIFF_FOUND "YES" )
    FIND_LIBRARY( TIFF_DEBUG_LIBRARIES 
      NAMES tiffd wxtiffd
      PATHS /usr/lib /usr/local/lib ${wxWidgets_LIB_DIR}
    )
  ENDIF(TIFF_LIBRARIES)
ENDIF(TIFF_INCLUDE_DIR)




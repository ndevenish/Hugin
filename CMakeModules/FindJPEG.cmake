# - Find JPEG for Hugin 0.7 01Nov2007 TKSharpless
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
#  JPEG_INCLUDE_DIR, where to find headers
#  JPEG_LIBRARIES, list of release link libraries
#  JPEG_DEBUG_LIBRARIES, list of debug link libraries
#  JPEG_FOUND, If != "YES", do not try to use JPEG

FIND_PATH(JPEG_INCLUDE_DIR jpeglib.h
  /usr/local/include
  /usr/include
  ${wxWidgets_ROOT_DIR}/src/jpeg
)

FIND_LIBRARY( JPEG_LIBRARIES
  NAMES jpeg wxjpeg
  PATHS /usr/lib /usr/local/lib ${wxWidgets_LIB_DIR}
)

IF(JPEG_INCLUDE_DIR)
  IF(JPEG_LIBRARIES)
    SET( JPEG_FOUND "YES" )
    FIND_LIBRARY( JPEG_DEBUG_LIBRARIES
      NAMES jpegd wxjpegd
      PATHS /usr/lib /usr/local/lib ${wxWidgets_LIB_DIR}
    )
  ENDIF(JPEG_LIBRARIES)
ENDIF(JPEG_INCLUDE_DIR)




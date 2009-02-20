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

include(FindLibraryWithDebug)

find_library_with_debug(ZLIB_LIBRARIES
  WIN32_DEBUG_POSTFIX d
  NAMES z zlib wxzlib
  PATHS /usr/lib /usr/local/lib ${wxWidgets_LIB_DIR}
)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZLIB DEFAULT_MSG 
                                  ZLIB_INCLUDE_DIR ZLIB_LIBRARIES)

MARK_AS_ADVANCED(
  ZLIB_LIBRARIES
  ZLIB_INCLUDE_DIR
  )

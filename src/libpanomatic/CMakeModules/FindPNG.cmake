# - Modified Find PNG for Hugin
# Added to support Windows build but should work anywhere.
#
# defines cache variables
#  PNG_INCLUDE_DIR, where to find headers
#  PNG_LIBRARIES, list of release link libraries.
#  PNG_FOUND, If != "YES", do not try to use PNG.
# None of the above will be defined unless ZLIB can be found

INCLUDE(FindZLIB REQUIRED)

FIND_PATH(PNG_INCLUDE_DIR png.h
    /usr/local/include
    /usr/include
    ${SOURCE_BASE_DIR}/lpng140
  )

FIND_LIBRARY(PNG_LIBRARIES
    NAMES png libpng libpng14
    PATHS /usr/lib /usr/local/lib ${SOURCE_BASE_DIR}/lpng140/lib
  )

IF(WIN32)
  FIND_FILE(PNG_DLL
    NAMES libpng14.dll 
    PATHS ${SOURCE_BASE_DIR}/lpng140/lib
  )
ENDIF(WIN32)


INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PNG  DEFAULT_MSG  PNG_INCLUDE_DIR PNG_LIBRARIES )

IF (PNG_FOUND)
	MESSAGE(STATUS "Found PNG include dir: ${PNG_INCLUDE_DIR}")
    MESSAGE(STATUS "Found PNG library: ${PNG_LIBRARIES}")
ELSE (PNG_FOUND)
	MESSAGE(FATAL_ERROR "Could not find libpng")
ENDIF (PNG_FOUND)


SET(PNG_LIBRARIES ${PNG_LIBRARIES} ${ZLIB_LIBRARIES})
MARK_AS_ADVANCED(PNG_INCLUDE_DIR PNG_LIBRARIES )

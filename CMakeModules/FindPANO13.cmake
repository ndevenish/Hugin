# - Find PANO13 headers and libraries
# modified for Hugin 0.7 Windows build 02Nov2007 TKSharpless
# reads cache variable
#  SOURCE_BASE_DIR -- directory that contains hugin source root
# defines cache vars
#  PANO13_INCLUDE_DIR, where to find pano13/panorama.h, etc.
#  PANO13_LIBRARIES, release link library list.
#  PANO13_DEBUG_LIBRARIES, debug ditto.
#  PANO13_FOUND, If != "YES", do not try to use PANO13.


## NOTE the form "pano13/panorama.h" is used in #includes in 
## some hugin source files, so we are stuck with that.
FIND_PATH(PANO13_INCLUDE_DIR pano13/panorama.h
  /usr/local/include
  /usr/include
  ${SOURCE_BASE_DIR}/libpano13/include
  ${SOURCE_BASE_DIR}/libpano
  ${SOURCE_BASE_DIR}
)


FIND_LIBRARY(PANO13_LIBRARIES
  NAMES pano13
  PATHS /usr/lib /usr/local/lib 
        "${PANO13_INCLUDE_DIR}/pano13/Release LIB CMD"
        "${PANO13_INCLUDE_DIR}/pano13/Release CMD/Win32"
        ${PANO13_INCLUDE_DIR}/pano13/Release
        ${SOURCE_BASE_DIR}/libpano13/lib
        ${SOURCE_BASE_DIR}/pano13
  )

IF(PANO13_INCLUDE_DIR)
  IF(PANO13_LIBRARIES)
    SET( PANO13_FOUND "YES" )
    FIND_LIBRARY( PANO13_DEBUG_LIBRARIES
      NAMES Panotools pano13d pano13
      PATHS /usr/lib /usr/local/lib 
            "${PANO13_INCLUDE_DIR}/pano13/Debug LIB CMD"
            "${PANO13_INCLUDE_DIR}/pano13/Debug CMD/Win32"
            ${PANO13_INCLUDE_DIR}/pano13/Debug
            ${SOURCE_BASE_DIR}/pano13/lib
            ${SOURCE_BASE_DIR}/pano13
    )
  ENDIF(PANO13_LIBRARIES)
ENDIF(PANO13_INCLUDE_DIR)


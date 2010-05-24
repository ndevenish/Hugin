# - Find VIGRA 
# Added to support Windows build but should work anywhere.
# After looking in UNIX standard places, tries wxWidgets build 
# tree, which should have this package.
#
# 
# reads cache variables
# defines cache variables
#  VIGRA_INCLUDE_DIR, where to find headers
#  VIGRA_LIBRARIES, list of release link libraries
#  VIGRA_FOUND, If != "YES", error out as VIGRA is required

SET( VIGRA_FOUND "NO" )

IF(WIN32)
  FIND_PATH(VIGRA_INCLUDE_DIR vigra/gaborfilter.hxx 
      PATHS ${VIGRA_ROOT_PATH}/include ${SOURCE_BASE_DIR}/vigra1.6.0/include
  )
  # for dynamic build, it's vigraimpex.lib and the dll must be copied into hugin's bin folder
  #SET(VIGRA_LIBRARIES ${SOURCE_BASE_DIR}/vigra/lib/libvigraindex.dll)
  FIND_LIBRARY( VIGRA_LIBRARIES 
    NAMES vigraimpex vigraimpex.dll libvigraimpex 
    PATHS
    ${VIGRA_ROOT_PATH}
    ${VIGRA_ROOT_PATH}/Release
    ${VIGRA_ROOT_PATH}/lib
    ${SOURCE_BASE_DIR}/vigra1.6.0/lib
    )
  FIND_FILE(VIGRA_DLL
    NAMES vigraimpex.dll
    PATHS ${SOURCE_BASE_DIR}/vigra1.6.0/lib
  )
ELSE(WIN32)
  FIND_PATH(VIGRA_INCLUDE_DIR gaborfilter.hxx
    /usr/local/include/vigra
    /usr/include/vigra
    /opt/local/include/vigra
  )

  FIND_LIBRARY(VIGRA_LIBRARIES
    NAMES vigraimpex libvigraimpex 
    PATHS /usr/lib /usr/local/lib /opt/local/lib
 ) 
ENDIF(WIN32)


IF (VIGRA_INCLUDE_DIR AND VIGRA_LIBRARIES)
   SET(VIGRA_FOUND TRUE)
ENDIF (VIGRA_INCLUDE_DIR AND VIGRA_LIBRARIES)

IF (VIGRA_FOUND)
	MESSAGE(STATUS "Found VIGRA: ${VIGRA_LIBRARIES}")
ELSE (VIGRA_FOUND)
	MESSAGE(FATAL_ERROR "Could not find VIGRA")
ENDIF (VIGRA_FOUND)


MARK_AS_ADVANCED(
  VIGRA_LIBRARIES
  VIGRA_INCLUDE_DIR
  )


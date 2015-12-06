# find lcms2 for Hugin

FIND_PATH(LCMS2_ROOT_DIR
  NAMES include/lcms2.h
  PATHS /usr/local
    /usr
    ${SOURCE_BASE_DIR}
  PATH_SUFFIXES
    lcms2-2.7
    lcms2-2.6
    lcms2-2.5
)

FIND_PATH(LCMS2_INCLUDE_DIR 
  NAMES lcms2.h
  PATHS
    ${LCMS2_ROOT_DIR}/include
)

include(FindLibraryWithDebug)
find_library_with_debug(LCMS2_LIBRARIES 
  WIN32_DEBUG_POSTFIX d    
  NAMES lcms2 lcms2_static
  PATHS ${SYSTEM_LIB_DIRS} ${LCMS2_ROOT_DIR}/lib ${LCMS2_ROOT_DIR}/bin ${LCMS2_ROOT_DIR}/Lib/MS
)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LCMS2 DEFAULT_MSG 
                                  LCMS2_INCLUDE_DIR LCMS2_LIBRARIES)

MARK_AS_ADVANCED(
  LCMS2_ROOT_DIR
  LCMS2_LIBRARIES
  LCMS2_INCLUDE_DIR
  )        

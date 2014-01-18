# - Find lensfun headers and libraries

FIND_PATH(LENSFUN_INCLUDE_DIR lensfun.h
  /usr/local/include
  /usr/local/include/lensfun
  /usr/include
  /usr/include/lensfun
  ${SOURCE_BASE_DIR}/lensfun/include/lensfun
)

INCLUDE(FindLibraryWithDebug)
FIND_LIBRARY_WITH_DEBUG(LENSFUN_LIBRARIES
  WIN32_DEBUG_POSTFIX d
  NAMES lensfun
  PATHS ${SYSTEM_LIB_DIRS} 
        ${SOURCE_BASE_DIR}/lensfun/lib
)

IF(NOT ${HUGIN_SHARED})
    FIND_LIBRARY_WITH_DEBUG(LENSFUN_REGEX_LIBRARIES
      NAMES tre_regex
      WIN32_DEBUG_POSTFIX d
      PATHS ${SYSTEM_LIB_DIRS} 
            ${SOURCE_BASE_DIR}/lensfun/lib
  )
  # base path for searching for glib on windows
  IF(WIN32)
    IF(NOT GLIB2_BASE_DIR)
      SET(GLIB2_BASE_DIR "${SOURCE_BASE_DIR}/glib-2.28.1" CACHE STRING "Base path of glib2 dir." FORCE)
    ENDIF()
  ENDIF()
  FIND_PACKAGE(GLIB2 REQUIRED)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lensfun DEFAULT_MSG LENSFUN_INCLUDE_DIR LENSFUN_LIBRARIES)
MARK_AS_ADVANCED(LENSFUN_INCLUDE_DIR LENSFUN_LIBRARIES)

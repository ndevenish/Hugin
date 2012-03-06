# - Find lensfun headers and libraries

FIND_PATH(LENSFUN_INCLUDE_DIR lensfun.h
  /usr/local/include
  /usr/include
  ${SOURCE_BASE_DIR}/lensfun/include/lensfun
)

FIND_LIBRARY(LENSFUN_LIBRARIES
  NAMES lensfun
  PATHS ${SYSTEM_LIB_DIRS} 
        ${SOURCE_BASE_DIR}/lensfun/lib
  )

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Lensfun DEFAULT_MSG LENSFUN_INCLUDE_DIR LENSFUN_LIBRARIES)
MARK_AS_ADVANCED(LENSFUN_INCLUDE_DIR LENSFUN_LIBRARIES)

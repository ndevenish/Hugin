# - Find LAPACK libraries
# 2009 Lukas Jirkovsky
# based on FindPANO13 by TKSharpless
# reads cache variable
#  SOURCE_BASE_DIR -- directory that contains hugin source root
# defines cache vars
#  LAPACK_LIBRARIES, release link library list.
#  LAPACK_FOUND, If != "YES", do not try to use PANO13.

FIND_LIBRARY(LAPACK_LIBRARIES
  NAMES lapack
  PATHS /usr/lib
        /usr/local/lib
        ${SOURCE_BASE_DIR}/
        ${SOURCE_BASE_DIR}/lapack
  )

IF(LAPACK_LIBRARIES)
  SET( LAPACK_FOUND "YES" )
ENDIF(LAPACK_LIBRARIES)


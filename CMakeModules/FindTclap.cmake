# search for tclap Templatized C++ Command Line Parser

IF(WIN32)
FIND_PATH(TCLAP_INCLUDEDIR tclap/CmdLine.h
  ${SOURCE_BASE_DIR}/tclap-1.2.0/include
)
IF(TCLAP_INCLUDEDIR)
SET(TCLAP_FOUND TRUE)
ENDIF()

ELSE(WIN32)
  pkg_check_modules(TCLAP tclap>=1.1)
ENDIF(WIN32)

# search for tclap Templatized C++ Command Line Parser

FIND_PATH(TCLAP_INCLUDEDIR tclap/CmdLine.h
  /usr/local/include
  /usr/include
  ${SOURCE_BASE_DIR}/tclap-1.2.1/include
  ${SOURCE_BASE_DIR}/tclap-1.2.0/include
)
IF(TCLAP_INCLUDEDIR)
SET(TCLAP_FOUND TRUE)
ELSE()
SET(TCLAP_FOUND FALSE)
ENDIF()

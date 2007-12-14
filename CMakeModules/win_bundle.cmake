
IF(WIN32)

  # copy installer files
  CONFIGURE_FILE(platforms/windows/msi/WixFragmentRegistry.wxs ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/WixFragmentRegistry.wxs COPYONLY)
  CONFIGURE_FILE(platforms/windows/msi/hugin.warsetup ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/hugin.warsetup )
  CONFIGURE_FILE(platforms/windows/msi/top_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/top_banner.bmp COPYONLY)
  CONFIGURE_FILE(platforms/windows/msi/big_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/big_banner.bmp COPYONLY)

  # find the path to enblend and panotools build directories
  # and copy required binaries into hugin installation folder
  FIND_PATH(PANO13_EXE_DIR PTmender.exe 
            ${SOURCE_BASE_DIR}/libpano/pano13/tools
            ${SOURCE_BASE_DIR}/libpano/pano13/tools/Release
            DOC "Location of pano13 executables"
            NO_DEFAULT_PATH)
  FILE(GLOB PANO13_EXECUTABLES ${PANO13_EXE_DIR}/*.exe)
  INSTALL(FILES ${PANO13_EXECUTABLES} DESTINATION ${BINDIR})

  # TODO: install documentation for panotools?

  FIND_PATH(ENBLEND_EXE_DIR enblend.exe 
            ${SOURCE_BASE_DIR}/enblend-3.1
            DOC "Location of enblend executables"
            NO_DEFAULT_PATH
            )
  FILE(GLOB ENBLEND_EXECUTABLES ${ENBLEND_EXE_DIR}/*.exe)
  SET(ENBLEND_DOC_FILES ${ENBLEND_EXE_DIR}/AUTHORS
                        ${ENBLEND_EXE_DIR}/ChangeLog  
                        ${ENBLEND_EXE_DIR}/COPYING  
                        ${ENBLEND_EXE_DIR}/INSTALL  
                        ${ENBLEND_EXE_DIR}/NEWS
                        ${ENBLEND_EXE_DIR}/README
                        ${ENBLEND_EXE_DIR}/READMEWIN
                        ${ENBLEND_EXE_DIR}/TODO  
                        ${ENBLEND_EXE_DIR}/VIGRA_LICENSE)

  INSTALL(FILES ${ENBLEND_EXECUTABLES} DESTINATION ${BINDIR})
  INSTALL(FILES ${ENBLEND_DOC_FILES} DESTINATION doc/enblend)

  # install make
  FIND_PATH(MAKE_EXE_DIR make.exe 
            ${SOURCE_BASE_DIR}/tools/
            ${SOURCE_BASE_DIR}/make/
            DOC "Location of GNU make executable"
            NO_DEFAULT_PATH)
  INSTALL(FILES ${MAKE_EXE_DIR}/make.exe DESTINATION ${BINDIR})

ENDIF(WIN32)


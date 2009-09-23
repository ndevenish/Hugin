IF(WIN32)

  # copy installer files
#  CONFIGURE_FILE(platforms/windows/msi/WixFragmentRegistry.wxs ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/WixFragmentRegistry.wxs COPYONLY)
#  CONFIGURE_FILE(platforms/windows/msi/hugin.warsetup ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/hugin.warsetup )
  # bug: CONFIGURE_FILE destroys the bitmaps.
#  CONFIGURE_FILE(platforms/windows/msi/top_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/top_banner.bmp COPYONLY)
#  CONFIGURE_FILE(platforms/windows/msi/big_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/big_banner.bmp COPYONLY)

  # install hugin readme, license etc.
  INSTALL(FILES AUTHORS COPYING LICENCE_VIGRA
          DESTINATION doc/hugin)

  # find the path to enblend and panotools build directories
  # and copy required binaries into hugin installation folder
  FIND_PATH(PANO13_EXE_DIR PTmender.exe 
            ${SOURCE_BASE_DIR}/libpano/pano13/tools
            ${SOURCE_BASE_DIR}/libpano/pano13/tools/Release
            ${SOURCE_BASE_DIR}/libpano/tools
            ${SOURCE_BASE_DIR}/libpano/tools/Release          
            DOC "Location of pano13 executables"
            NO_DEFAULT_PATH)
  FILE(GLOB PANO13_EXECUTABLES ${PANO13_EXE_DIR}/*.exe)
  INSTALL(FILES ${PANO13_EXECUTABLES} DESTINATION ${BINDIR})

  # TODO: install documentation for panotools?
  FIND_PATH(PANO13_SRC_DIR filter.h 
            ${SOURCE_BASE_DIR}/libpano/pano13
            DOC "Location of pano13 source"
            NO_DEFAULT_PATH)
  INSTALL(FILES ${PANO13_SRC_DIR}/AUTHORS
          ${PANO13_SRC_DIR}/gpl.txt
          ${PANO13_SRC_DIR}/README
          ${PANO13_SRC_DIR}/TODO
          ${PANO13_SRC_DIR}/doc/Optimize.txt
          ${PANO13_SRC_DIR}/doc/PTblender.readme
          ${PANO13_SRC_DIR}/doc/PTmender.readme
          ${PANO13_SRC_DIR}/doc/stitch.txt
          DESTINATION doc/panotools)

  FIND_PATH(ENBLEND_EXE_DIR enblend.exe 
            ${SOURCE_BASE_DIR}/enblend-3.1
            ${SOURCE_BASE_DIR}/enblend-enfuse-3.2
            ${SOURCE_BASE_DIR}/enblend.build
            ${SOURCE_BASE_DIR}/enblend
            DOC "Location of enblend executables"
            NO_DEFAULT_PATH
            )
  FILE(GLOB ENBLEND_EXECUTABLES ${ENBLEND_EXE_DIR}/*.exe)
  SET(ENBLEND_DOC_FILES ${ENBLEND_EXE_DIR}/AUTHORS
                        ${ENBLEND_EXE_DIR}/ChangeLog  
                        ${ENBLEND_EXE_DIR}/COPYING  
                        ${ENBLEND_EXE_DIR}/NEWS
                        ${ENBLEND_EXE_DIR}/README
                        ${ENBLEND_EXE_DIR}/README_WINDOWS.txt
                        ${ENBLEND_EXE_DIR}/TODO  
                        ${ENBLEND_EXE_DIR}/VIGRA_LICENSE)

  INSTALL(FILES ${ENBLEND_EXECUTABLES} DESTINATION ${BINDIR})
  INSTALL(FILES ${ENBLEND_DOC_FILES} DESTINATION doc/enblend)

  # find path to UnxUtils and install required files
  FIND_PATH(UnxUtils_DIR UnxUtilsDist.html
            ${SOURCE_BASE_DIR}/UnxUtils
            DOC "Location of UnxUtils (http://sf.net/projects/unxutils) files"
            NO_DEFAULT_PATH)

  INSTALL(FILES ${UnxUtils_DIR}/usr/local/wbin/make.exe
	  ${UnxUtils_DIR}/usr/local/wbin/basename.exe
	  ${UnxUtils_DIR}/usr/local/wbin/cp.exe
	  ${UnxUtils_DIR}/usr/local/wbin/rm.exe
	  ${UnxUtils_DIR}/usr/local/wbin/echo.exe
	  ${UnxUtils_DIR}/usr/local/wbin/uname.exe
	  ${UnxUtils_DIR}/bin/sh.exe
	  DESTINATION ${BINDIR})

  # install exiftool
  FIND_PATH(EXIFTOOL_EXE_DIR exiftool.exe
	    ${SOURCE_BASE_DIR}/tools
	    ${SOURCE_BASE_DIR}/exiftool
	    DOC "Location of exiftool.exe"
	    NO_DEFAULT_PATH)
  INSTALL(FILES ${EXIFTOOL_EXE_DIR}/exiftool.exe DESTINATION ${BINDIR})

  # grab and install autopano-sift-C
  FIND_PATH(AP_SIFT_DIR bin/autopano.exe
	    ${SOURCE_BASE_DIR}/autopano-sift-C
        ${SOURCE_BASE_DIR}/autopano-sift-C/INSTALL/FILES
	    DOC "Base directory of autopano-sift-C installation"
	    NO_DEFAULT_PATH)
  FILE(GLOB AP_SIFT_EXE ${AP_SIFT_DIR}/bin/*)
  INSTALL(FILES ${AP_SIFT_EXE} DESTINATION ${BINDIR})

  FILE(GLOB AP_SIFT_MAN ${AP_SIFT_DIR}/share/man/man*/*)
  INSTALL(FILES ${AP_SIFT_MAN} DESTINATION doc/autopano-sift-C)

ENDIF(WIN32)


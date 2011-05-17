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
            ${PANO13_INCLUDE_DIR}/pano13/tools
            ${PANO13_INCLUDE_DIR}/pano13/tools/Release
            "${PANO13_INCLUDE_DIR}/pano13/tools/Release CMD/win32"
            ${SOURCE_BASE_DIR}/libpano/tools
            "${SOURCE_BASE_DIR}/libpano/pano13/tools/Release CMD/Win32"
            ${SOURCE_BASE_DIR}/libpano/tools/Release          
            ${SOURCE_BASE_DIR}/libpano13/bin
            DOC "Location of pano13 executables"
            NO_DEFAULT_PATH)
  FILE(GLOB PANO13_EXECUTABLES ${PANO13_EXE_DIR}/*.exe)
  INSTALL(FILES ${PANO13_EXECUTABLES} DESTINATION ${BINDIR})
  IF(${HUGIN_SHARED})
    FIND_FILE(PANO13_DLL pano13.dll
              PATHS ${SOURCE_BASE_DIR}/libpano13/lib
              NO_SYSTEM_ENVIRONMENT_PATH
              )
    INSTALL(FILES ${PANO13_DLL} DESTINATION ${BINDIR})
  ENDIF(${HUGIN_SHARED})

  # TODO: install documentation for panotools?
  FIND_PATH(PANO13_SRC_DIR filter.h 
            ${PANO13_INCLUDE_DIR}/pano13
            DOC "Location of pano13 source"
            NO_DEFAULT_PATH)
  INSTALL(FILES ${PANO13_SRC_DIR}/AUTHORS
          ${PANO13_SRC_DIR}/COPYING
          ${PANO13_SRC_DIR}/README
          ${PANO13_SRC_DIR}/doc/Optimize.txt
          ${PANO13_SRC_DIR}/doc/PTblender.readme
          ${PANO13_SRC_DIR}/doc/PTmender.readme
          ${PANO13_SRC_DIR}/doc/stitch.txt
          DESTINATION doc/panotools)

  # install enblend/enfuse files

  FIND_PATH(ENBLEND_EXE_DIR enblend_openmp.exe
            PATHS ${SOURCE_BASE_DIR}/enblend-enfuse-4.1 ${SOURCE_BASE_DIR}/enblend-enfuse-4.0
            DOC "Location of enblend executables"
            NO_DEFAULT_PATH
            )
  IF(${ENBLEND_EXE_DIR} MATCHES "-NOTFOUND")
  # enblend-enfuse 4.0 not found
  # try finding enblend-enfuse 3.0, 3.2
    FIND_PATH(ENBLEND_EXE_DIR enblend.exe 
              ${SOURCE_BASE_DIR}/enblend-3.1
              ${SOURCE_BASE_DIR}/enblend-enfuse-3.2
              ${SOURCE_BASE_DIR}/enblend.build
              ${SOURCE_BASE_DIR}/enblend
              DOC "Location of enblend executables"
              NO_DEFAULT_PATH
              )
    SET(ENBLEND_DOC_FILES ${ENBLEND_EXE_DIR}/AUTHORS
                          ${ENBLEND_EXE_DIR}/ChangeLog  
                          ${ENBLEND_EXE_DIR}/COPYING  
                          ${ENBLEND_EXE_DIR}/NEWS
                          ${ENBLEND_EXE_DIR}/README
                          ${ENBLEND_EXE_DIR}/README_WINDOWS.txt
                          ${ENBLEND_EXE_DIR}/TODO  
                          ${ENBLEND_EXE_DIR}/VIGRA_LICENSE)
  ELSE()
    # file of enblend-enfuse 4.0
    SET(ENBLEND_DOC_FILES ${ENBLEND_EXE_DIR}/AUTHORS.txt
                          ${ENBLEND_EXE_DIR}/ChangeLog.txt  
                          ${ENBLEND_EXE_DIR}/COPYING.txt  
                          ${ENBLEND_EXE_DIR}/NEWS.txt
                          ${ENBLEND_EXE_DIR}/README.txt
                          ${ENBLEND_EXE_DIR}/VIGRA_LICENSE.txt
                          ${ENBLEND_EXE_DIR}/doc/enblend.pdf
                          ${ENBLEND_EXE_DIR}/doc/enfuse.pdf
                          )

  ENDIF()

  FILE(GLOB ENBLEND_EXECUTABLES ${ENBLEND_EXE_DIR}/*.exe)
  INSTALL(FILES ${ENBLEND_EXECUTABLES} DESTINATION ${BINDIR})
  INSTALL(FILES ${ENBLEND_DOC_FILES} DESTINATION doc/enblend)

  # find path to gnu make 
  FIND_PATH(GNUMake_DIR make.exe
            ${SOURCE_BASE_DIR}/Make-3.82/Release
            ${SOURCE_BASE_DIR}/Make-3.81/Release
            DOC "Location of gnu make"
            NO_DEFAULT_PATH)
  INSTALL(FILES ${GNUMake_DIR}/make.exe
          DESTINATION ${BINDIR})
 
  # install exiftool
  FIND_PATH(EXIFTOOL_EXE_DIR exiftool.exe
	    ${SOURCE_BASE_DIR}/tools
	    ${SOURCE_BASE_DIR}/exiftool
	    DOC "Location of exiftool.exe"
	    NO_DEFAULT_PATH)
  INSTALL(FILES ${EXIFTOOL_EXE_DIR}/exiftool.exe DESTINATION ${BINDIR})

  # now install all necessary DLL
  IF(${HUGIN_SHARED})
    FIND_FILE(TIFF_DLL
      NAMES libtiff.dll 
      PATHS ${SOURCE_BASE_DIR}/tiff-4.0.0beta7/libtiff ${SOURCE_BASE_DIR}/tiff-4.0.0beta5/libtiff
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(JPEG_DLL
      NAMES jpeg.dll 
      PATHS ${SOURCE_BASE_DIR}/jpeg-8c/lib ${SOURCE_BASE_DIR}/jpeg-8b/Release ${SOURCE_BASE_DIR}/jpeg-8a/Release ${SOURCE_BASE_DIR}/jpeg-8/Release
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(PNG_DLL
      NAMES libpng14.dll libpng15.dll
      PATHS ${SOURCE_BASE_DIR}/libpng/bin ${SOURCE_BASE_DIR}/lpng142/lib ${SOURCE_BASE_DIR}/lpng141/lib ${SOURCE_BASE_DIR}/lpng140/lib
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(ZLIB_DLL
      NAMES zlib1.dll 
      PATHS ${SOURCE_BASE_DIR}/zlib ${SOURCE_BASE_DIR}/zlib/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_PATH(OPENEXR_BIN_DIR Half.dll 
            ${SOURCE_BASE_DIR}/Deploy/bin/Release
            DOC "Location of OpenEXR libraries"
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_DEFAULT_PATH
    )
    FILE(GLOB OPENEXR_DLL ${OPENEXR_BIN_DIR}/*.dll)
    FILE(GLOB BOOST_THREAD_DLL ${Boost_LIBRARY_DIRS}/boost_thread*.dll)
    FILE(GLOB BOOST_DATE_TIME_DLL ${Boost_LIBRARY_DIRS}/boost_date_time*.dll)
    FILE(GLOB BOOST_SYSTEM_DLL ${Boost_LIBRARY_DIRS}/boost_system*.dll)
    FILE(GLOB BOOST_REGEX_DLL ${Boost_LIBRARY_DIRS}/boost_regex*.dll)
    FILE(GLOB BOOST_SIGNALS_DLL ${Boost_LIBRARY_DIRS}/boost_signals*.dll)
    FIND_FILE(EXIV2_DLL 
      NAMES exiv2.dll 
      PATHS ${SOURCE_BASE_DIR}/exiv2-0.21.1/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.20/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.19/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/bin/ReleaseDLL
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LIBEXPAT_DLL 
      NAMES libexpat.dll 
      PATHS ${SOURCE_BASE_DIR}/exiv2-0.21.1/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.20/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.19/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/bin/ReleaseDLL
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(GLEW_DLL
      NAMES glew32.dll
      PATHS ${SOURCE_BASE_DIR}/glew/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(GLUT_DLL
      NAMES glut.dll freeglut.dll
      PATHS ${SOURCE_BASE_DIR}/freeglut-2.6.0/VisualStudio2008/Release ${SOURCE_BASE_DIR}/glut/Release
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    # hand tuned dll, so that only necesarry dll are install and not all wxWidgets DLL to save space
    FIND_FILE(WXWIDGETS_DLL1 NAMES wxbase28u_vc_custom.dll wxbase291u_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL2 NAMES wxmsw28u_core_vc_custom.dll wxmsw291u_core_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL3 NAMES wxmsw28u_xrc_vc_custom.dll wxmsw291u_xrc_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL4 NAMES wxmsw28u_adv_vc_custom.dll wxmsw291u_adv_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL5 NAMES wxmsw28u_gl_vc_custom.dll wxmsw291u_gl_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL6 NAMES wxmsw28u_html_vc_custom.dll wxmsw291u_html_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL7 NAMES wxbase28u_xml_vc_custom.dll wxbase291u_xml_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL8 NAMES wxmsw28u_aui_vc_custom.dll wxmsw291u_aui_vc_custom.dll PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)

    INSTALL(FILES ${TIFF_DLL} ${JPEG_DLL} ${PNG_DLL} ${ZLIB_DLL} ${OPENEXR_DLL} 
        ${BOOST_THREAD_DLL} ${BOOST_DATE_TIME_DLL} ${BOOST_SYSTEM_DLL} ${BOOST_REGEX_DLL} ${BOOST_SIGNALS_DLL}
        ${EXIV2_DLL} ${LIBEXPAT_DLL} ${GLEW_DLL} ${GLUT_DLL}
        ${WXWIDGETS_DLL1} ${WXWIDGETS_DLL2} ${WXWIDGETS_DLL2} ${WXWIDGETS_DLL3}
        ${WXWIDGETS_DLL3} ${WXWIDGETS_DLL4} ${WXWIDGETS_DLL5} ${WXWIDGETS_DLL6}
        ${WXWIDGETS_DLL7} ${WXWIDGETS_DLL8}
        DESTINATION ${BINDIR}
    )
  ENDIF(${HUGIN_SHARED})
ENDIF(WIN32)


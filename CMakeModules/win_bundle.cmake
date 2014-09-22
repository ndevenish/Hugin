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
              PATHS ${SOURCE_BASE_DIR}/libpano13/bin
              NO_SYSTEM_ENVIRONMENT_PATH
              )
    INSTALL(FILES ${PANO13_DLL} DESTINATION ${BINDIR})
  ENDIF(${HUGIN_SHARED})

  # TODO: install documentation for panotools?
  FIND_PATH(PANO13_DOC_DIR Optimize.txt 
            ${PANO13_INCLUDE_DIR}/../share/pano13/doc
            DOC "Location of pano13 documentation"
            NO_DEFAULT_PATH)
  INSTALL(FILES ${PANO13_DOC_DIR}/AUTHORS
          ${PANO13_DOC_DIR}/COPYING
          ${PANO13_DOC_DIR}/README
          ${PANO13_DOC_DIR}/Optimize.txt
          ${PANO13_DOC_DIR}/PTblender.readme
          ${PANO13_DOC_DIR}/PTmender.readme
          ${PANO13_DOC_DIR}/stitch.txt
          DESTINATION doc/panotools)

  # install enblend/enfuse files

  FIND_PATH(ENBLEND_DIR bin/enblend.exe enblend.exe
            PATHS 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.3-win64 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.3-win32 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.2-win64 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.2-win32 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.1-win64 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.1-win32 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.1 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.0
              ${SOURCE_BASE_DIR}/enblend-3.1
              ${SOURCE_BASE_DIR}/enblend-enfuse-3.2
              ${SOURCE_BASE_DIR}/enblend.build
              ${SOURCE_BASE_DIR}/enblend
            DOC "Location of enblend"
            NO_DEFAULT_PATH
            )
  FILE(GLOB ENBLEND_EXECUTABLES ${ENBLEND_DIR}/bin/*.exe ${ENBLEND_DIR}/bin/*.dll ${ENBLEND_DIR}/*.exe)
  FILE(GLOB ENBLEND_DOC_FILES ${ENBLEND_DIR}/*.* ${ENBLEND_DIR}/doc/*.pdf)
  INSTALL(FILES ${ENBLEND_EXECUTABLES} DESTINATION ${BINDIR})
  INSTALL(FILES ${ENBLEND_DOC_FILES} DESTINATION doc/enblend)

  # find path to gnu make 
  FIND_PATH(GNUMake_DIR make.exe
            ${SOURCE_BASE_DIR}/Make-4.0/x64/Release
            ${SOURCE_BASE_DIR}/Make-4.0/Release
            ${SOURCE_BASE_DIR}/Make-3.82/Release
            ${SOURCE_BASE_DIR}/Make-3.81/Release
            DOC "Location of gnu make"
            NO_DEFAULT_PATH)
  INSTALL(FILES ${GNUMake_DIR}/make.exe
          DESTINATION ${BINDIR})
 
  # install exiftool
  INSTALL(FILES ${EXIFTOOL_EXE_DIR}/exiftool.exe DESTINATION ${BINDIR})

  # now install all necessary DLL
  IF(${HUGIN_SHARED})
    FIND_FILE(TIFF_DLL
      NAMES libtiff.dll 
      PATHS 
            ${SOURCE_BASE_DIR}/tiff-4.0.3/libtiff
            ${SOURCE_BASE_DIR}/tiff-4.0.1/libtiff
            ${SOURCE_BASE_DIR}/tiff-4.0.0beta7/libtiff 
            ${SOURCE_BASE_DIR}/tiff-4.0.0beta5/libtiff
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(JPEG_DLL
      NAMES jpeg.dll 
      PATHS 
            ${SOURCE_BASE_DIR}/jpeg-9a/lib 
            ${SOURCE_BASE_DIR}/jpeg-9a/x64/Release
            ${SOURCE_BASE_DIR}/jpeg-9/lib 
            ${SOURCE_BASE_DIR}/jpeg-8d/lib 
            ${SOURCE_BASE_DIR}/jpeg-8c/lib 
            ${SOURCE_BASE_DIR}/jpeg-8b/Release 
            ${SOURCE_BASE_DIR}/jpeg-8a/Release 
            ${SOURCE_BASE_DIR}/jpeg-8/Release
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(PNG_DLL
      NAMES libpng16.dll libpng15.dll libpng14.dll 
      PATHS ${SOURCE_BASE_DIR}/libpng/bin ${SOURCE_BASE_DIR}/lpng142/lib ${SOURCE_BASE_DIR}/lpng141/lib ${SOURCE_BASE_DIR}/lpng140/lib
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(ZLIB_DLL
      NAMES zlib1.dll zlib.dll
      PATHS ${SOURCE_BASE_DIR}/zlib ${SOURCE_BASE_DIR}/zlib/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_PATH(OPENEXR_BIN_DIR 
            NAMES Half.dll 
            PATHS ${SOURCE_BASE_DIR}/Deploy/lib/Release ${SOURCE_BASE_DIR}/Deploy/bin/Release
            DOC "Location of OpenEXR libraries"
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_DEFAULT_PATH
    )
    FILE(GLOB OPENEXR_DLL ${OPENEXR_BIN_DIR}/*.dll)
    FILE(GLOB BOOST_THREAD_DLL ${Boost_LIBRARY_DIRS}/boost_thread*.dll)
    FILE(GLOB BOOST_DATE_TIME_DLL ${Boost_LIBRARY_DIRS}/boost_date_time*.dll)
    FILE(GLOB BOOST_SYSTEM_DLL ${Boost_LIBRARY_DIRS}/boost_system*.dll)
    FILE(GLOB BOOST_REGEX_DLL ${Boost_LIBRARY_DIRS}/boost_regex*.dll)
    FILE(GLOB BOOST_FILESYSTEM_DLL ${Boost_LIBRARY_DIRS}/boost_filesystem*.dll)
    FILE(GLOB BOOST_CHRONO_DLL ${Boost_LIBRARY_DIRS}/boost_chrono*.dll)
    LIST(APPEND BOOST_DLLs ${BOOST_THREAD_DLL} ${BOOST_DATE_TIME_DLL} ${BOOST_SYSTEM_DLL} ${BOOST_REGEX_DLL} ${BOOST_FILESYSTEM_DLL})
    IF(NOT "${BOOST_CHRONO_DLL}" MATCHES "-NOTFOUND")
        LIST(APPEND BOOST_DLLs ${BOOST_CHRONO_DLL})
    ENDIF()
    IF(Boost_VERSION<105400)
      FILE(GLOB BOOST_SIGNALS_DLL ${Boost_LIBRARY_DIRS}/boost_signals*.dll)
      LIST(APPEND BOOST_DLLs ${BOOST_SIGNALS_DLL})
    ENDIF()
    FIND_FILE(EXIV2_DLL 
      NAMES exiv2.dll 
      PATHS ${SOURCE_BASE_DIR}/exiv2/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.23/msvc64/bin/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.23/msvc64/bin/Win32/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.22/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.21.1/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.20/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.19/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/bin/ReleaseDLL
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LIBEXPAT_DLL 
      NAMES libexpat.dll 
      PATHS ${SOURCE_BASE_DIR}/exiv2/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.23/msvc64/bin/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.23/msvc64/bin/Win32/ReleaseDLL ${SOURCE_BASE_DIR}/expat-2.0.1/win32/bin/Release ${SOURCE_BASE_DIR}/exiv2-0.22/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.21.1/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.20/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.19/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/bin/ReleaseDLL
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(GLEW_DLL
      NAMES glew32.dll
      PATHS ${SOURCE_BASE_DIR}/glew/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(GLUT_DLL
      NAMES glut.dll freeglut.dll glut32.dll
      PATHS ${SOURCE_BASE_DIR}/freeglut-2.6.0/VisualStudio2008/Release ${SOURCE_BASE_DIR}/glut/Release ${SOURCE_BASE_DIR}/glut/lib
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    # hand tuned dll, so that only necesarry dll are install and not all wxWidgets DLL to save space
    FIND_FILE(WXWIDGETS_DLL1 
              NAMES wxbase310u_vc_custom.dll wxbase30u_vc_custom.dll wxbase295u_vc_custom.dll wxbase294u_vc_custom.dll wxbase293u_vc_custom.dll wxbase292u_vc_custom.dll wxbase291u_vc_custom.dll wxbase28u_vc_custom.dll 
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL2 
              NAMES wxmsw310u_core_vc_custom.dll wxmsw30u_core_vc_custom.dll wxmsw295u_core_vc_custom.dll wxmsw294u_core_vc_custom.dll wxmsw293u_core_vc_custom.dll wxmsw292u_core_vc_custom.dll wxmsw291u_core_vc_custom.dll wxmsw28u_core_vc_custom.dll
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL3 
              NAMES wxmsw310u_xrc_vc_custom.dll wxmsw30u_xrc_vc_custom.dll wxmsw295u_xrc_vc_custom.dll wxmsw294u_xrc_vc_custom.dll wxmsw293u_xrc_vc_custom.dll wxmsw292u_xrc_vc_custom.dll wxmsw291u_xrc_vc_custom.dll wxmsw28u_xrc_vc_custom.dll
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL4 
              NAMES wxmsw310u_adv_vc_custom.dll wxmsw30u_adv_vc_custom.dll wxmsw295u_adv_vc_custom.dll wxmsw294u_adv_vc_custom.dll wxmsw293u_adv_vc_custom.dll wxmsw292u_adv_vc_custom.dll wxmsw291u_adv_vc_custom.dll wxmsw28u_adv_vc_custom.dll
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL5 
              NAMES wxmsw310u_gl_vc_custom.dll wxmsw30u_gl_vc_custom.dll wxmsw295u_gl_vc_custom.dll wxmsw294u_gl_vc_custom.dll wxmsw293u_gl_vc_custom.dll wxmsw292u_gl_vc_custom.dll wxmsw291u_gl_vc_custom.dll wxmsw28u_gl_vc_custom.dll
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL6 
              NAMES wxmsw310u_html_vc_custom.dll wxmsw30u_html_vc_custom.dll wxmsw295u_html_vc_custom.dll wxmsw294u_html_vc_custom.dll wxmsw293u_html_vc_custom.dll wxmsw292u_html_vc_custom.dll wxmsw291u_html_vc_custom.dll wxmsw28u_html_vc_custom.dll
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL7 
              NAMES wxbase310u_xml_vc_custom.dll wxbase30u_xml_vc_custom.dll wxbase295u_xml_vc_custom.dll wxbase294u_xml_vc_custom.dll wxbase293u_xml_vc_custom.dll wxbase292u_xml_vc_custom.dll wxbase291u_xml_vc_custom.dll wxbase28u_xml_vc_custom.dll
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)
    FIND_FILE(WXWIDGETS_DLL8 
              NAMES wxmsw310u_aui_vc_custom.dll wxmsw30u_aui_vc_custom.dll wxmsw295u_aui_vc_custom.dll wxmsw294u_aui_vc_custom.dll wxmsw293u_aui_vc_custom.dll wxmsw292u_aui_vc_custom.dll wxmsw291u_aui_vc_custom.dll wxmsw28u_aui_vc_custom.dll
              PATHS ${wxWidgets_LIB_DIR} NO_SYSTEM_ENVIRONMENT_PATH)

    INSTALL(FILES ${TIFF_DLL} ${JPEG_DLL} ${PNG_DLL} ${ZLIB_DLL} ${OPENEXR_DLL} 
        ${BOOST_DLLs} ${EXIV2_DLL} ${LIBEXPAT_DLL} ${GLEW_DLL} ${GLUT_DLL}
        ${WXWIDGETS_DLL1} ${WXWIDGETS_DLL2} ${WXWIDGETS_DLL2} ${WXWIDGETS_DLL3}
        ${WXWIDGETS_DLL3} ${WXWIDGETS_DLL4} ${WXWIDGETS_DLL5} ${WXWIDGETS_DLL6}
        ${WXWIDGETS_DLL7} ${WXWIDGETS_DLL8}
        DESTINATION ${BINDIR}
    )
    
    FIND_FILE(SQLITE3_DLL NAMES sqlite3.dll PATHS ${SOURCE_BASE_DIR}/sqlite3 NO_SYSTEM_ENVIRONMENT_PATH)
    INSTALL(FILES ${SQLITE3_DLL} DESTINATION ${BINDIR})

    IF(HAVE_FFTW)
      FIND_FILE(FFTW3_DLL 
        NAMES libfftw-3.3.dll
        PATHS ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/x64 
              ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/
          NO_SYSTEM_ENVIRONMENT_PATH)
      INSTALL(FILES ${FFTW3_DLL} DESTINATION ${BINDIR})
    ENDIF()

  ENDIF(${HUGIN_SHARED})
ENDIF(WIN32)


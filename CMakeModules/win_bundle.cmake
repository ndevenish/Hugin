IF(WIN32)

  # copy installer files
#  CONFIGURE_FILE(platforms/windows/msi/WixFragmentRegistry.wxs ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/WixFragmentRegistry.wxs COPYONLY)
#  CONFIGURE_FILE(platforms/windows/msi/hugin.warsetup ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/hugin.warsetup )
  # bug: CONFIGURE_FILE destroys the bitmaps.
#  CONFIGURE_FILE(platforms/windows/msi/top_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/top_banner.bmp COPYONLY)
#  CONFIGURE_FILE(platforms/windows/msi/big_banner.bmp ${CMAKE_CURRENT_BINARY_DIR}/INSTALL/big_banner.bmp COPYONLY)

  # install hugin readme, license etc.
  INSTALL(FILES AUTHORS COPYING 
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
    FIND_FILE(PANO13_DLL 
              NAMES pano13.dll libpano13.dll
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
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.4-win64 
              ${SOURCE_BASE_DIR}/enblend-enfuse-4.1.4-win32 
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
      NAMES libtiff.dll tiff.dll
      PATHS 
            ${SOURCE_BASE_DIR}/libtiff/bin
            ${SOURCE_BASE_DIR}/tiff-4.0.6/libtiff
            ${SOURCE_BASE_DIR}/tiff-4.0.5/libtiff
            ${SOURCE_BASE_DIR}/tiff-4.0.4/libtiff
            ${SOURCE_BASE_DIR}/tiff-4.0.3/libtiff
            ${SOURCE_BASE_DIR}/tiff-4.0.1/libtiff
            ${SOURCE_BASE_DIR}/tiff-4.0.0beta7/libtiff 
            ${SOURCE_BASE_DIR}/tiff-4.0.0beta5/libtiff
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(JPEG_DLL
      NAMES jpeg.dll libjpeg.dll
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
      NAMES zlib1.dll zlib.dll libz.dll libzlib.dll
      PATHS ${SOURCE_BASE_DIR}/zlib ${SOURCE_BASE_DIR}/zlib/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_PATH(OPENEXR_BIN_DIR 
            NAMES Half.dll libHalf.dll
            PATHS ${SOURCE_BASE_DIR}/Deploy/lib ${SOURCE_BASE_DIR}/Deploy/bin ${SOURCE_BASE_DIR}/Deploy/lib/Release ${SOURCE_BASE_DIR}/Deploy/bin/Release
            DOC "Location of OpenEXR libraries"
            NO_SYSTEM_ENVIRONMENT_PATH
            NO_DEFAULT_PATH
    )
    FILE(GLOB OPENEXR_DLL ${OPENEXR_BIN_DIR}/*.dll)
    FIND_FILE(VIGRA_DLL
       NAMES vigraimpex.dll
       PATHS ${SOURCE_BASE_DIR}/vigra/bin
       NO_SYSTEM_ENVIRONMENT_PATH
    )
    IF(NOT HAVE_STD_FILESYSTEM)
      FILE(GLOB BOOST_SYSTEM_DLL ${Boost_LIBRARY_DIRS}/*boost_system*.dll)
      FILE(GLOB BOOST_FILESYSTEM_DLL ${Boost_LIBRARY_DIRS}/*boost_filesystem*.dll)
      LIST(APPEND BOOST_DLLs ${BOOST_SYSTEM_DLL} ${BOOST_FILESYSTEM_DLL})
    ENDIF()
    IF(NOT CXX11_THREAD)
      FILE(GLOB BOOST_THREAD_DLL ${Boost_LIBRARY_DIRS}/*boost_thread*.dll)
      FILE(GLOB BOOST_DATE_TIME_DLL ${Boost_LIBRARY_DIRS}/*boost_date_time*.dll)
      FILE(GLOB BOOST_CHRONO_DLL ${Boost_LIBRARY_DIRS}/*boost_chrono*.dll)
      LIST(APPEND BOOST_DLLs ${BOOST_THREAD_DLL} ${BOOST_DATE_TIME_DLL})
      IF(NOT "${BOOST_CHRONO_DLL}" MATCHES "-NOTFOUND")
        LIST(APPEND BOOST_DLLs ${BOOST_CHRONO_DLL})
      ENDIF()
    ENDIF()
    FIND_FILE(EXIV2_DLL 
      NAMES exiv2.dll libexiv2.dll
      PATHS ${SOURCE_BASE_DIR}/exiv2/bin ${SOURCE_BASE_DIR}/exiv2/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.25/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.24/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.23/msvc64/bin/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.23/msvc64/bin/Win32/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.22/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.21.1/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.20/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.19/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/bin/ReleaseDLL
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LIBEXPAT_DLL 
      NAMES libexpat.dll expat.dll
      PATHS ${SOURCE_BASE_DIR}/expat/bin ${SOURCE_BASE_DIR}/exiv2/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.25/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.24/msvc2012/exiv2lib/x64/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.23/msvc64/bin/Win32/ReleaseDLL ${SOURCE_BASE_DIR}/expat-2.0.1/win32/bin/Release ${SOURCE_BASE_DIR}/exiv2-0.22/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.21.1/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.20/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.19/msvc/bin/ReleaseDLL ${SOURCE_BASE_DIR}/exiv2-0.18.2/msvc/bin/ReleaseDLL
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(GLEW_DLL
      NAMES glew32.dll
      PATHS ${SOURCE_BASE_DIR}/glew/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    FIND_FILE(LCMS2_DLL
      NAMES lcms2.dll liblcms2.dll liblcms2-2.dll
      PATHS ${LCMS2_ROOT_DIR}/bin
      NO_SYSTEM_ENVIRONMENT_PATH
    )
    # hand tuned dll, so that only necesarry dll are install and not all wxWidgets DLL to save space
    IF(MSVC)
      SET(WXSUFFIX vc)
    ELSE()
      IF(MINGW)
        SET(WXSUFFIX gcc)
      ELSE()
        MESSAGE(FATAL_ERROR "Unknown target for Win32 wxWidgets DLLs")
      ENDIF()
    ENDIF()
    # first variant is for development versions with 3 numbers, second variant for stable versions with 2 numbers
    FILE(GLOB WXWIDGETS_DLL
      ${wxWidgets_LIB_DIR}/wxbase[2-3][0-9][0-9]u_${WXSUFFIX}*.dll     ${wxWidgets_LIB_DIR}/wxbase[2-3][0-9]u_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9][0-9]u_core_${WXSUFFIX}*.dll ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9]u_core_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9][0-9]u_xrc_${WXSUFFIX}*.dll  ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9]u_xrc_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9][0-9]u_adv_${WXSUFFIX}*.dll  ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9]u_adv_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9][0-9]u_gl_${WXSUFFIX}*.dll   ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9]u_gl_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9][0-9]u_html_${WXSUFFIX}*.dll ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9]u_html_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxbase[2-3][0-9][0-9]u_xml_${WXSUFFIX}*.dll ${wxWidgets_LIB_DIR}/wxbase[2-3][0-9]u_xml_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9][0-9]u_aui_${WXSUFFIX}*.dll  ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9]u_aui_${WXSUFFIX}*.dll
      ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9][0-9]u_qa_${WXSUFFIX}*.dll   ${wxWidgets_LIB_DIR}/wxmsw[2-3][0-9]u_qa_${WXSUFFIX}*.dll
    )
    # some checking in ensure all is found okay
    list(LENGTH WXWIDGETS_DLL COUNT_WXWIDGETS_DLL)
    IF(NOT ${COUNT_WXWIDGETS_DLL} EQUAL 9)
      MESSAGE(FATAL_ERROR "Not all necessary wxWidgets dlls could be found.")
    ENDIF()

    INSTALL(FILES ${TIFF_DLL} ${JPEG_DLL} ${PNG_DLL} ${ZLIB_DLL} ${OPENEXR_DLL} ${VIGRA_DLL}
        ${BOOST_DLLs} ${EXIV2_DLL} ${LIBEXPAT_DLL} ${GLEW_DLL} ${LCMS2_DLL}
        ${WXWIDGETS_DLL}
        DESTINATION ${BINDIR}
    )
    
    FIND_FILE(SQLITE3_DLL 
        NAMES sqlite3.dll libsqlite3.dll 
        PATHS ${SOURCE_BASE_DIR}/sqlite3 NO_SYSTEM_ENVIRONMENT_PATH
    )
    INSTALL(FILES ${SQLITE3_DLL} DESTINATION ${BINDIR})

    IF(HAVE_FFTW)
      FIND_FILE(FFTW3_DLL 
        NAMES libfftw-3.3.dll
        PATHS ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/x64/Release
              ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/x64 
              ${SOURCE_BASE_DIR}/fftw-3.3.4/fftw-3.3-libs/
              ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/x64 
              ${SOURCE_BASE_DIR}/fftw-3.3.3/fftw-3.3-libs/
          NO_SYSTEM_ENVIRONMENT_PATH)
      INSTALL(FILES ${FFTW3_DLL} DESTINATION ${BINDIR})
    ENDIF()

  ENDIF(${HUGIN_SHARED})
ENDIF(WIN32)


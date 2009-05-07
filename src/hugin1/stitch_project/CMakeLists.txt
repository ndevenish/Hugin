
set( PROGNAME hugin_stitch_project )
IF (WIN32)
  add_executable(${PROGNAME} WIN32 hugin_stitch_project.cpp hugin_stitch_project_rc.rc)
ELSEIF(APPLE)
  set( PROGNAME HuginStitchProject )
  set( MACOSX_BUNDLE_EXTENSION pto)
  set( MACOSX_BUNDLE_ICON_FILE ../hugin/Hugin.icns )
  set( MACOSX_BUNDLE_TYPE_ICON_FILE ../hugin/HuginFiles.icns )
  set( MACOSX_BUNDLE_MIME_TYPE application/x-ptoptimizer-script )
  set( MACOSX_BUNDLE_TYPE_NAME "Hugin Project" )
  set( MACOSX_BUNDLE_TYPE_ROLE Viewer )
  set( MACOSX_BUNDLE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH} )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${MACOSX_BUNDLE_VERSION})
  set( MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER net.sourceforge.hugin)
  set( MACOSX_BUNDLE_INFO_STRING "HuginStitchProject ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_BUNDLE_NAME "HuginStitchProject" )
  set( MACOSX_BUNDLE_COPYRIGHT "Copyright 2007 Pablo d'Angelo" )
  set( MACOSX_BUNDLE_SIGNATURE "HgiS" )
  #  set( CMAKE_OSX_ARCHITECTURES ppc;i386 ) #Comment out if not universal binary

  IF(MAC_SELF_CONTAINED_BUNDLE)
    MESSAGE(SEND_ERROR "TODO: add support for building fully featured HuginStitchProject bundle")
  ENDIF(MAC_SELF_CONTAINED_BUNDLE)
     
  set( ICON_FILES ${MACOSX_BUNDLE_ICON_FILE} ${MACOSX_BUNDLE_TYPE_ICON_FILE} )
  ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE hugin_stitch_project.cpp 
                  ${ICON_FILES})

ELSE (WIN32)
 add_executable(${PROGNAME} hugin_stitch_project.cpp)
ENDIF (WIN32)


target_link_libraries(${PROGNAME} huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES})

IF (APPLE)
  IF (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS ${PROGNAME}
            DESTINATION ${INSTALL_OSX_BUNDLE_DIR})
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ELSE(APPLE)
  install(TARGETS ${PROGNAME}
          DESTINATION ${BINDIR})
ENDIF(APPLE)


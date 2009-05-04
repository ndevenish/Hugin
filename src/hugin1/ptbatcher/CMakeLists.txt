set( PROGNAME PTBatcher )

add_executable(PTBatcher PTBatcher.cpp PTBatcher.h PTBatcher_rc.rc Batch.cpp Batch.h RunStitchFrame.cpp RunStitchFrame.h ProjectArray.cpp ProjectArray.h)
target_link_libraries(${PROGNAME} huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES})
install(TARGETS PTBatcher
        DESTINATION ${BINDIR})

#--------------------------------

set( PROGNAME_GUI PTBatcherGUI )
IF (WIN32)
  add_executable(${PROGNAME_GUI} WIN32 PTBatcherGUI.cpp PTBatcherGUI.h PTBatcherGUI_rc.rc Batch.cpp Batch.h RunStitchFrame.cpp RunStitchFrame.h ProjectArray.cpp ProjectArray.h ProjectListBox.cpp ProjectListBox.h BatchFrame.cpp BatchFrame.h DirTraverser.h)
ELSEIF(APPLE)
  set( PROGNAME PTBatcherGUI )
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
  set( MACOSX_BUNDLE_INFO_STRING "PTBatcherGUI ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_BUNDLE_NAME "PTBatcherGUI" )
  set( MACOSX_BUNDLE_COPYRIGHT "Copyright 2007 Pablo d'Angelo" )
  set( MACOSX_BUNDLE_SIGNATURE "HgiS" )
  #  set( CMAKE_OSX_ARCHITECTURES ppc;i386 ) #Comment out if not universal binary

  IF(MAC_SELF_CONTAINED_BUNDLE)
    MESSAGE(SEND_ERROR "TODO: add support for building fully featured PTBatcherGUI bundle")
  ENDIF(MAC_SELF_CONTAINED_BUNDLE)
     
  set( ICON_FILES ${MACOSX_BUNDLE_ICON_FILE} ${MACOSX_BUNDLE_TYPE_ICON_FILE} )
  ADD_EXECUTABLE( ${PROGNAME_GUI} MACOSX_BUNDLE PTBatcherGUI.cpp PTBatcherGUI.h PTBatcherGUI_rc.rc Batch.cpp Batch.h RunStitchFrame.cpp RunStitchFrame.h ProjectArray.cpp ProjectArray.h ProjectListBox.cpp ProjectListBox.h BatchFrame.cpp BatchFrame.h DirTraverser.h
                  ${ICON_FILES})

ELSE (WIN32)
 add_executable(${PROGNAME_GUI} PTBatcherGUI.cpp PTBatcherGUI.h PTBatcherGUI_rc.rc Batch.cpp Batch.h RunStitchFrame.cpp RunStitchFrame.h ProjectArray.cpp ProjectArray.h ProjectListBox.cpp ProjectListBox.h BatchFrame.cpp BatchFrame.h DirTraverser.h)
ENDIF (WIN32)


target_link_libraries(${PROGNAME_GUI} huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES})

IF (APPLE)
  IF (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS ${PROGNAME_GUI}
            DESTINATION ${INSTALL_OSX_BUNDLE_DIR})
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ELSE(APPLE)
  install(TARGETS ${PROGNAME_GUI}
          DESTINATION ${BINDIR})
  IF (UNIX)
    INSTALL(FILES PTBatcherGUI.desktop DESTINATION ${DATADIR}/applications)
  ENDIF (UNIX)
ENDIF(APPLE)


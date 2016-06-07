set(PTBATCHERGUI_SOURCE 
      PTBatcherGUI.cpp Batch.cpp RunStitchFrame.cpp ProjectArray.cpp ProjectListBox.cpp 
      BatchFrame.cpp FindPanoDialog.cpp FailedProjectsDialog.cpp BatchTrayIcon.cpp ProgressStatusBar.cpp)
set( PTBATCHERGUI_HEADER PTBatcherGUI.h Batch.h RunStitchFrame.h ProjectArray.h ProjectListBox.h
                         BatchFrame.h DirTraverser.h FindPanoDialog.h FailedProjectsDialog.h 
                         BatchTrayIcon.h ProgressStatusBar.h PTBatcherGUI_rc.rc)
IF (WIN32)
  add_executable(PTBatcherGUI WIN32 ${PTBATCHERGUI_SOURCE} ${PTBATCHERGUI_HEADER})
  IF (MINGW)
    # MinGW doesn't support #pragma comment(lib, "PowrProf.lib")
    target_link_libraries(PTBatcherGUI -lpowrprof)
  ENDIF(MINGW)
ELSEIF(APPLE)
  set( PROGNAME PTBatcherGUI )
  set( MACOSX_BUNDLE_ICON_FILE PTBatcherGui.icns )

  set( INFO_PLIST ${CMAKE_MODULE_PATH}/HuginBundleInfoPTO.plist.in)
  set( MACOSX_BUNDLE_TYPE_ROLE Viewer )
  
  set( MACOSX_BUNDLE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH} )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${MACOSX_BUNDLE_VERSION})
  set( MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER net.sourceforge.hugin.PTBatcherGUI)
  set( MACOSX_BUNDLE_INFO_STRING "PTBatcherGUI ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_BUNDLE_NAME "PTBatcherGUI" )
  set( MACOSX_BUNDLE_COPYRIGHT "Copyright 2007 Pablo d'Angelo" )

  IF(MAC_SELF_CONTAINED_BUNDLE)

    # Tools
    set( TOOLS ${CMAKE_BINARY_DIR}/src/tools/align_image_stack ${CMAKE_BINARY_DIR}/src/tools/nona
               ${CMAKE_BINARY_DIR}/src/tools/hugin_hdrmerge ${CMAKE_BINARY_DIR}/src/tools/verdandi
               ${CMAKE_SOURCE_DIR}/mac/ExternalPrograms/repository/bin/enblend
               ${CMAKE_SOURCE_DIR}/mac/ExternalPrograms/repository/bin/enfuse 
    )

    FOREACH(_file ${TOOLS})
      SET_SOURCE_FILES_PROPERTIES( ${_file} PROPERTIES 
           MACOSX_PACKAGE_LOCATION "MacOS" 
           GENERATED TRUE)
    ENDFOREACH(_file)

    FILE(GLOB OTHER ${CMAKE_SOURCE_DIR}/src/hugin1/executor/*.executor)
    LIST(APPEND OTHER 
                      ${CMAKE_SOURCE_DIR}/src/celeste/data/celeste.model
                      ${CMAKE_SOURCE_DIR}/src/hugin1/base_wx/hugin_exiftool_copy.arg)

    FOREACH(_file ${OTHER})
      SET_SOURCE_FILES_PROPERTIES( ${_file} PROPERTIES 
           MACOSX_PACKAGE_LOCATION 
           "Resources/xrc" )
    ENDFOREACH(_file)

    # Translations
    
    FILE(GLOB PO_FILES "${CMAKE_SOURCE_DIR}/src/translations/*.po")
    SET(TRANSLATION_FILES "")
    FOREACH(_file_po ${PO_FILES})
      GET_FILENAME_COMPONENT(LANG ${_file_po} NAME_WE)
      SET(_file_out ${CMAKE_BINARY_DIR}/src/translations/mac/${LANG}.lproj/hugin.mo)
      SET(TRANSLATION_FILES "${TRANSLATION_FILES};${_file_out}" )
      SET_SOURCE_FILES_PROPERTIES( ${_file_out} PROPERTIES 
        MACOSX_PACKAGE_LOCATION "Resources/${LANG}.lproj" 
               GENERATED TRUE)
    ENDFOREACH(_file_po)


    # xrc files
    FILE(GLOB_RECURSE XRC_FILES ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.html 
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.htm
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.hhc
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.hhk 
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.hhp
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.xrc
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.ico
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.jpg
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.png
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.txt
                                ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/xrc/*.pl )

    FOREACH(_file ${XRC_FILES})
          GET_FILENAME_COMPONENT(_file_dir ${_file} PATH)
      FILE(RELATIVE_PATH _file_dir ${CMAKE_SOURCE_DIR}/src/hugin1/hugin/ ${_file_dir})
      SET_SOURCE_FILES_PROPERTIES( ${_file} PROPERTIES 
              MACOSX_PACKAGE_LOCATION 
              "Resources/${_file_dir}")
    ENDFOREACH(_file)


    # Icons
    set( ICON_FILES "../hugin/${MACOSX_BUNDLE_ICON_FILE}" "../hugin/${MACOSX_BUNDLE_TYPE_ICON_FILE}" )
    FOREACH(_file ${ICON_FILES})
      SET_SOURCE_FILES_PROPERTIES( ${_file} PROPERTIES 
           MACOSX_PACKAGE_LOCATION 
           "Resources" )
    ENDFOREACH(_file)

    ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE ${PTBATCHERGUI_SOURCE} ${PTBATCHERGUI_HEADER}
          ${ICON_FILES} ${TRANSLATION_FILES} ${XRC_FILES} ${TOOLS} ${OTHER})
    if(INFO_PLIST)
      set_target_properties(${PROGNAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST})
    endif(INFO_PLIST)

    ADD_DEPENDENCIES(${PROGNAME} align_image_stack nona hugin_hdrmerge verdandi
      translations)


  else (MAC_SELF_CONTAINED_BUNDLE)

    set( ICON_FILES ${MACOSX_BUNDLE_ICON_FILE} ${MACOSX_BUNDLE_TYPE_ICON_FILE} )
    ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE ${PTBATCHERGUI_SOURCE} ${PTBATCHERGUI_HEADER} ${ICON_FILES} ${OTHER})

  ENDIF(MAC_SELF_CONTAINED_BUNDLE)
     

ELSE (WIN32)
 add_executable(PTBatcherGUI PTBatcherGUI.cpp ${PTBATCHERGUI_SOURCE} ${PTBATCHERGUI_HEADER})
ENDIF (WIN32)


target_link_libraries(PTBatcherGUI huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES})

IF (APPLE)
  IF (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS PTBatcherGUI
            DESTINATION ${INSTALL_OSX_BUNDLE_DIR})
  ELSE (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS ${PROGNAME}
            DESTINATION "Hugin"
            COMPONENT ${PROGNAME})
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ELSE(APPLE)
  install(TARGETS PTBatcherGUI
          DESTINATION ${BINDIR})
  IF (UNIX)
    INSTALL(FILES PTBatcherGUI.desktop DESTINATION ${DATADIR}/applications)
  ENDIF (UNIX)
ENDIF(APPLE)

set_property(INSTALL "bin/$<TARGET_FILE_NAME:PTBatcherGUI>"
  PROPERTY CPACK_START_MENU_SHORTCUTS "PTBatcherGUI - Batch processor"
)


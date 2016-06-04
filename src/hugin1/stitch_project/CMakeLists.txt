
set( PROGNAME hugin_stitch_project )
IF (WIN32)
  add_executable(${PROGNAME} WIN32 hugin_stitch_project.cpp hugin_stitch_project_rc.rc)
ELSEIF(APPLE)
  set( PROGNAME HuginStitchProject )
  set( MACOSX_BUNDLE_ICON_FILE Hugin.icns )

  set( INFO_PLIST ${CMAKE_MODULE_PATH}/HuginBundleInfoPTO.plist.in)
  set( MACOSX_BUNDLE_TYPE_ROLE Viewer )
  
  set( MACOSX_BUNDLE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH} )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${MACOSX_BUNDLE_VERSION})
  set( MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER net.sourceforge.hugin)
  set( MACOSX_BUNDLE_INFO_STRING "HuginStitchProject ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_BUNDLE_NAME "HuginStitchProject" )
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
    SET(TRANSLATION_FILES )
    FOREACH(_file_po ${PO_FILES})
      GET_FILENAME_COMPONENT(LANG ${_file_po} NAME_WE)
      SET(_file_out ${CMAKE_BINARY_DIR}/src/translations/mac/${LANG}.lproj/hugin.mo)
      LIST(APPEND TRANSLATION_FILES ${_file_out} )
      SET_SOURCE_FILES_PROPERTIES( ${_file_out} PROPERTIES 
        MACOSX_PACKAGE_LOCATION "Resources/${LANG}.lproj" 
        GENERATED TRUE)
    ENDFOREACH(_file_po)


    # Icons
    set( ICON_FILES "../hugin/${MACOSX_BUNDLE_ICON_FILE}" "../hugin/${MACOSX_BUNDLE_TYPE_ICON_FILE}" )
    FOREACH(_file ${ICON_FILES})
      SET_SOURCE_FILES_PROPERTIES( ${_file} PROPERTIES 
           MACOSX_PACKAGE_LOCATION 
           "Resources" )
    ENDFOREACH(_file)


    ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE hugin_stitch_project.cpp 
          ${ICON_FILES} ${TRANSLATION_FILES} ${TOOLS} ${OTHER})
    if(INFO_PLIST)
      set_target_properties(${PROGNAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST})
    endif(INFO_PLIST)

    ADD_DEPENDENCIES(${PROGNAME} align_image_stack nona hugin_hdrmerge verdandi
      translations)

  ELSE (MAC_SELF_CONTAINED_BUNDLE)

    set( ICON_FILES ${MACOSX_BUNDLE_ICON_FILE} ${MACOSX_BUNDLE_TYPE_ICON_FILE} )
    ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE hugin_stitch_project.cpp 
                    ${ICON_FILES})

  ENDIF(MAC_SELF_CONTAINED_BUNDLE)
     

ELSE (WIN32)
 add_executable(${PROGNAME} hugin_stitch_project.cpp)
ENDIF (WIN32)


target_link_libraries(${PROGNAME} huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES})

IF (APPLE)
  IF (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS ${PROGNAME}
            DESTINATION ${INSTALL_OSX_BUNDLE_DIR})
  ELSE (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS ${PROGNAME}
            DESTINATION "Hugin")
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ELSE(APPLE)
  install(TARGETS ${PROGNAME}
          DESTINATION ${BINDIR})
ENDIF(APPLE)


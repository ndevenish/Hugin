
set(CALLENSGUI_SOURCE LensCalApp.cpp LensCalFrame.cpp LensCalImageCtrl.cpp LensCalTypes.cpp)
set(CALLENSGUI_HEADER LensCalApp.h LensCalFrame.h LensCalImageCtrl.h LensCalTypes.h)

IF (WIN32)
  add_executable(calibrate_lens_gui WIN32 ${CALLENSGUI_SOURCE} ${CALLENSGUI_HEADER} calibrate_lens_gui_rc.rc)
ELSEIF(APPLE)

  set( PROGNAME calibrate_lens_gui )
  #set( MACOSX_BUNDLE_ICON_FILE ../hugin/Hugin.icns )

  set( INFO_PLIST ${CMAKE_MODULE_PATH}/HuginBundleInfo.plist.in)

  set( MACOSX_BUNDLE_GUI_IDENTIFIER net.sourceforge.hugin.calibrate_lens_gui)
  set( MACOSX_BUNDLE_BUNDLE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH} )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${MACOSX_BUNDLE_BUNDLE_VERSION})
  set( MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MACOSX_BUNDLE_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_INFO_STRING "calibrate_lens_gui ${MACOSX_BUNDLE_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_BUNDLE_NAME "calibrate_lens_gui" )
  set( MACOSX_BUNDLE_COPYRIGHT "Copyright 2007 Pablo d'Angelo" )
  # set( MACOSX_BUNDLE_SIGNATURE "Hgin" )

  IF(MAC_SELF_CONTAINED_BUNDLE)
    
    FILE(GLOB PO_FILES "${CMAKE_SOURCE_DIR}/src/translations/*.po")
    SET(TRANSLATION_FILES "")
    FOREACH(_file_po ${PO_FILES})
      GET_FILENAME_COMPONENT(LANG ${_file_po} NAME_WE)
      SET(_file_out ${CMAKE_BINARY_DIR}/src/translations/mac/${LANG}.lproj/hugin.mo)
      SET(TRANSLATION_FILES "${TRANSLATION_FILES};${_file_out}" )
      SET_SOURCE_FILES_PROPERTIES( ${_file_out} PROPERTIES 
        MACOSX_PACKAGE_LOCATION "Resources/${LANG}.lproj/" 
        GENERATED TRUE)
    ENDFOREACH(_file_po)


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

    ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE ${CALLENSGUI_SOURCE} ${CALLENSGUI_HEADER}
          ${TRANSLATION_FILES} ${XRC_FILES} )
    if(INFO_PLIST)
      set_target_properties(${PROGNAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${INFO_PLIST})
    endif(INFO_PLIST)

    ADD_DEPENDENCIES(${PROGNAME} translations)

  ELSE(MAC_SELF_CONTAINED_BUNDLE)

    ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE ${CALLENSGUI_SOURCE} ${CALLENSGUI_HEADER})

  ENDIF(MAC_SELF_CONTAINED_BUNDLE)

ELSE (WIN32)
 add_executable(calibrate_lens_gui ${CALLENSGUI_SOURCE} ${CALLENSGUI_HEADER})
ENDIF (WIN32)


target_link_libraries(calibrate_lens_gui huginbasewx ${common_libs} ${image_libs} ${wxWidgets_LIBRARIES} huginlines)

IF (APPLE)
  IF (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS calibrate_lens_gui
            DESTINATION ${INSTALL_OSX_BUNDLE_DIR})
  ELSE (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS ${PROGNAME}
            DESTINATION "Hugin"
            COMPONENT ${PROGNAME})
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ELSE(APPLE)
    INSTALL(TARGETS calibrate_lens_gui DESTINATION ${BINDIR})
    IF(UNIX)
        # install further stuff (icons etc.)
        INSTALL(FILES calibrate_lens_gui.desktop DESTINATION ${DATADIR}/applications)
    ENDIF(UNIX)
ENDIF(APPLE)

set_property(INSTALL "bin/$<TARGET_FILE_NAME:calibrate_lens_gui>"
  PROPERTY CPACK_START_MENU_SHORTCUTS "Lens calibration GUI"
)


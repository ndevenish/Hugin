# Application name
IF( APPLE )
  set( PROGNAME Hugin )
  set( MACOSX_BUNDLE_EXTENSION pto)
  set( MACOSX_BUNDLE_ICON_FILE Hugin.icns )
  set( MACOSX_BUNDLE_TYPE_ICON_FILE HuginFiles.icns )
  set( MACOSX_BUNDLE_MIME_TYPE application/x-ptoptimizer-script )
  set( MACOSX_BUNDLE_TYPE_NAME "Hugin Project" )
  set( MACOSX_BUNDLE_TYPE_ROLE Editor )
  set( MACOSX_BUNDLE_VERSION ${V_MAJOR}.${V_MINOR}.${V_PATCH} )
  set( MACOSX_BUNDLE_SHORT_VERSION_STRING ${MACOSX_BUNDLE_VERSION})
  set( MACOSX_BUNDLE_LONG_VERSION_STRING "Version ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_GUI_IDENTIFIER net.sourceforge.hugin)
  set( MACOSX_BUNDLE_INFO_STRING "Hugin ${MACOSX_BUNDLE_VERSION}" )
  set( MACOSX_BUNDLE_BUNDLE_NAME "Hugin" )
  set( MACOSX_BUNDLE_COPYRIGHT "Copyright 2007 Pablo d'Angelo" )
  set( MACOSX_BUNDLE_SIGNATURE "Hgin" )
  #  set( CMAKE_OSX_ARCHITECTURES ppc;i386 ) #Comment out if not universal binary
ELSE( APPLE )
  set( PROGNAME hugin )
ENDIF( APPLE )

# sources
set(hugin_SOURCES
huginApp.cpp ImagesList.cpp AutoCtrlPointCreator.cpp
MainFrame.cpp CPImageCtrl.cpp CPEditorPanel.cpp LensPanel.cpp 
ImagesPanel.cpp CommandHistory.cpp PanoPanel.cpp
PreviewFrame.cpp PreviewPanel.cpp CropPanel.cpp OptimizePhotometricPanel.cpp
OptimizePanel.cpp RunStitcherFrame.cpp CPListFrame.cpp
TextKillFocusHandler.cpp PanoDruid.cpp PreferencesDialog.cpp
LocalizedFileTipProvider.cpp VigCorrDialog.cpp
wxPanoCommand.cpp AssistantPanel.cpp HFOVDialog.cpp)

IF( APPLE )
  FILE(GLOB TRANSLATION_FILES ${CMAKE_CURRENT_BINARY_DIR}/po/*.lproj/locale/hugin.mo)
  FILE(GLOB_RECURSE XRC_FILES ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.html 
    		    	      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.htm
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.hhc
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.hhk	
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.hhp
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.xrc
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.ico
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.jpg
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.png
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.xpm
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.txt
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.mk
			      ${CMAKE_CURRENT_SOURCE_DIR}/xrc/*.pl)

  set( ICON_FILES ${MACOSX_BUNDLE_ICON_FILE} ${MACOSX_BUNDLE_TYPE_ICON_FILE} )
	
  ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE ${hugin_SOURCES} 
  		  ${ICON_FILES} ${TRANSLATION_FILES} ${XRC_FILES} )

  FOREACH(_file ${TRANSLATION_FILES})
        GET_FILENAME_COMPONENT(_file_dir ${_file} PATH)
	FILE(RELATIVE_PATH _file_dir ${CMAKE_CURRENT_BINARY_DIR}/po ${_file_dir})
	SET_SOURCE_FILES_PROPERTIES( ${_file} PROPERTIES 
				     MACOSX_PACKAGE_LOCATION 
				    "Resources/${_file_dir}" )
  ENDFOREACH(_file)

  FOREACH(_file ${XRC_FILES})	
        GET_FILENAME_COMPONENT(_file_dir ${_file} PATH)
	FILE(RELATIVE_PATH _file_dir ${CMAKE_CURRENT_SOURCE_DIR} ${_file_dir})
	SET_SOURCE_FILES_PROPERTIES( ${_file} PROPERTIES 
				    MACOSX_PACKAGE_LOCATION 
				    "Resources/${_file_dir}")
  ENDFOREACH(_file)

  SET_SOURCE_FILES_PROPERTIES(
  	${ICON_FILES}
  	PROPERTIES 
	MACOSX_PACKAGE_LOCATION Resources
  )

  ADD_CUSTOM_COMMAND( 
    TARGET ${PROGNAME}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND}
    ARGS -Dfile="${CMAKE_CURRENT_BINARY_DIR}/${PROGNAME}.app/Contents/Resources/xrc/cp_editor_panel.xrc" -P ${CMAKE_SOURCE_DIR}/CMakeModules/fix_osx_xrc.cmake
  )
ELSE( APPLE )
  ADD_EXECUTABLE( ${PROGNAME} ${hugin_SOURCES} )
ENDIF( APPLE )

target_link_libraries( ${PROGNAME} ${common_libs} ${image_libs} huginbasewx ${wxWidgets_LIBRARIES})

install(TARGETS ${PROGNAME}
        DESTINATION bin)

# install further stuff (icons etc.)

IF (APPLE)
# already done by bundle
ELSEIF(UNIX)
  INSTALL(FILES hugin.desktop DESTINATION ${DATADIR}/applications)
  INSTALL(FILES gnome-mime-application-x-ptoptimizer-script.png DESTINATION ${DATADIR}/icons/gnome/48x48/mimetypes)
  INSTALL(FILES hugin.xml DESTINATION ${DATADIR}/mime/packages)
ENDIF(APPLE)


add_subdirectory(po) 
add_subdirectory(xrc)

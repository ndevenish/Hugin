# Application name
IF(APPLE)
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
ELSE(APPLE)
  set( PROGNAME hugin )
ENDIF(APPLE)

# sources
set(hugin_SOURCES
huginApp.cpp ImagesList.cpp MainFrame.cpp CPImageCtrl.cpp CPImagesComboBox.cpp CPEditorPanel.cpp CPDetectorDialog.cpp  
ImagesPanel.cpp CommandHistory.cpp PanoPanel.cpp NonaOptionsDialog.cpp HDRMergeOptionDialog.cpp
PreviewFrame.cpp PreviewPanel.cpp GLPreviewFrame.cpp GLViewer.cpp GLRenderer.cpp 
OptimizePhotometricPanel.cpp OptimizePanel.cpp CPListFrame.cpp
TextKillFocusHandler.cpp PreferencesDialog.cpp ResetDialog.cpp
LocalizedFileTipProvider.cpp  wxPanoCommand.cpp 
HFOVDialog.cpp TextureManager.cpp MeshRemapper.cpp VertexCoordRemapper.cpp
TexCoordRemapper.cpp ChoosyRemapper.cpp MeshManager.cpp ViewState.cpp
OutputProjectionInfo.cpp ToolHelper.cpp Tool.cpp  OverviewCameraTool.cpp ProjectionGridTool.cpp OverviewOutlinesTool.cpp
PreviewCropTool.cpp DragTool.cpp PreviewIdentifyTool.cpp PanosphereSphereTool.cpp
PreviewDifferenceTool.cpp PreviewPanoMaskTool.cpp PreviewControlPointTool.cpp
GreatCircles.cpp AboutDialog.cpp LayoutRemapper.cpp PreviewLayoutLinesTool.cpp PreviewColorPickerTool.cpp PreviewGuideTool.cpp
MaskEditorPanel.cpp MaskImageCtrl.cpp MaskLoadDialog.cpp HtmlWindow.cpp 
PanoOperation.cpp treelistctrl.cpp ImagesTree.cpp PanoOutputDialog.cpp
GuiLevel.cpp ImageVariableDialog.cpp)

set(hugin_HEADERS
huginApp.h ImagesList.h MainFrame.h CPImageCtrl.h CPImagesComboBox.h CPEditorPanel.h CPDetectorDialog.h 
ImagesPanel.h CommandHistory.h PanoPanel.h NonaOptionsDialog.h HDRMergeOptionDialog.h
PreviewFrame.h PreviewPanel.h GLPreviewFrame.h GLViewer.h GLRenderer.h 
OptimizePhotometricPanel.h OptimizePanel.h CPListFrame.h
TextKillFocusHandler.h PreferencesDialog.h ResetDialog.h
LocalizedFileTipProvider.h  wxPanoCommand.h 
HFOVDialog.h TextureManager.h MeshRemapper.h VertexCoordRemapper.h
TexCoordRemapper.h ChoosyRemapper.h MeshManager.h ViewState.h
OutputProjectionInfo.h ToolHelper.h Tool.h OverviewCameraTool.h ProjectionGridTool.h OverviewOutlinesTool.h
PreviewCropTool.h DragTool.h PreviewIdentifyTool.h PanosphereSphereTool.h
PreviewDifferenceTool.h PreviewPanoMaskTool.h PreviewControlPointTool.h
GreatCircles.h AboutDialog.h LayoutRemapper.h PreviewLayoutLinesTool.h PreviewColorPickerTool.h PreviewGuideTool.h
MaskEditorPanel.h MaskImageCtrl.h MaskLoadDialog.h HtmlWindow.h 
PanoOperation.h treelistctrl.h ImagesTree.h PanoOutputDialog.h
GuiLevel.h ImageVariableDialog.h)

IF(BUILD_HSI)
LIST(APPEND hugin_SOURCES PluginItems.cpp)
LIST(APPEND hugin_HEADERS PluginItems.h)
ENDIF()

IF(APPLE)
  if (MAC_SELF_CONTAINED_BUNDLE)
    MESSAGE("TODO: MAC_SELF_CONTAINED_BUNDLE currently broken")
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

    #ADD_CUSTOM_COMMAND( 
    #  TARGET ${PROGNAME}
    #  POST_BUILD
    #  COMMAND ${CMAKE_COMMAND}
    #  ARGS -Dfile="${CMAKE_CURRENT_BINARY_DIR}/${PROGNAME}.app/Contents/Resources/xrc/cp_editor_panel.xrc" -P ${CMAKE_SOURCE_DIR}/CMakeModules/fix_osx_xrc.cmake
    #)

  else (MAC_SELF_CONTAINED_BUNDLE)

    # bare bones bundle, with only hugin inside, all other ressources outside.
    set( ICON_FILES ${MACOSX_BUNDLE_ICON_FILE} ${MACOSX_BUNDLE_TYPE_ICON_FILE} )
    ADD_EXECUTABLE( ${PROGNAME} MACOSX_BUNDLE ${hugin_SOURCES} 
  		    ${ICON_FILES})

  endif (MAC_SELF_CONTAINED_BUNDLE)

  SET_SOURCE_FILES_PROPERTIES(
  	${ICON_FILES}
  	PROPERTIES 
	MACOSX_PACKAGE_LOCATION Resources)

ELSEIF ( WIN32 )
  ADD_EXECUTABLE( ${PROGNAME} WIN32 ${hugin_SOURCES} ${hugin_HEADERS} hugin_rc.rc )
ELSE (APPLE)
  ADD_EXECUTABLE( ${PROGNAME} ${hugin_SOURCES} ${hugin_HEADERS})
ENDIF(APPLE)

target_link_libraries( ${PROGNAME} ${common_libs} ${image_libs} huginbasewx celeste icpfindlib ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} ${GLEW_LIBRARIES} ${GLUT_LIBRARIES})

IF(BUILD_HSI)
TARGET_LINK_LIBRARIES(${PROGNAME} hugin_python_interface)
ENDIF()

IF (APPLE)
  IF (NOT MAC_SELF_CONTAINED_BUNDLE)
    install(TARGETS ${PROGNAME}
            DESTINATION ${INSTALL_OSX_BUNDLE_DIR})
  ELSE (NOT MAC_SELF_CONTAINED_BUNDLE)
    # try to build selfcontained bundle
    INCLUDE (${CMAKE_SOURCE_DIR}/CMakeModules/AppleRELEASEOptions.cmake)
  ENDIF (NOT MAC_SELF_CONTAINED_BUNDLE)
ELSE(APPLE)
  install(TARGETS ${PROGNAME}
          DESTINATION ${BINDIR})
  IF(UNIX)
    # install further stuff (icons etc.)
    INSTALL(FILES hugin.desktop DESTINATION ${DATADIR}/applications)
    INSTALL(FILES gnome-mime-application-x-ptoptimizer-script.png DESTINATION ${DATADIR}/icons/gnome/48x48/mimetypes)
    INSTALL(FILES hugin.xml DESTINATION ${DATADIR}/mime/packages)
    INSTALL(FILES Makefile.equirect.mk DESTINATION ${HUGINDATADIR})
  ENDIF(UNIX)
ENDIF(APPLE)

add_subdirectory(xrc)


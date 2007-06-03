

add_library(huginwxutils STATIC
            ImageCache.cpp MyProgressDialog.cpp Plot2D.cpp PTWXDlg.cpp)

add_executable(hugin 
              huginApp.cpp ImagesList.cpp AutoCtrlPointCreator.cpp
              MainFrame.cpp CPImageCtrl.cpp CPEditorPanel.cpp LensPanel.cpp 
              ImagesPanel.cpp CommandHistory.cpp PanoPanel.cpp
              NonaStitcherPanel.cpp PTStitcherPanel.cpp PreviewFrame.cpp
              PreviewPanel.cpp CropPanel.cpp OptimizePhotometricPanel.cpp
              OptimizePanel.cpp RunStitcherFrame.cpp CPListFrame.cpp
              TextKillFocusHandler.cpp PanoDruid.cpp PreferencesDialog.cpp
              LocalizedFileTipProvider.cpp VigCorrDialog.cpp
              wxPanoCommand.cpp MyExternalCmdExecDialog.cpp
              AssistantPanel.cpp HFOVDialog.cpp)

target_link_libraries(hugin ${common_libs} ${image_libs} huginwxutils ${wxWidgets_LIBRARIES})

install(TARGETS hugin
        DESTINATION bin)

add_subdirectory(po) 
add_subdirectory(xrc)
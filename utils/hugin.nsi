;--------------------------------

!define HUGIN_VERSION "0.4 beta"
!define DISPLAY_NAME "Hugin ${HUGIN_VERSION}"
!define HAVE_MINGW
;!define HUGIN_ALLINONE

;--------------------------------
;Include Modern UI

  !include "MUI.nsh"

;--------------------------------

# [Installer Attributes]

Name "${DISPLAY_NAME}"
!ifdef HUGIN_ALLINONE
OutFile "hugin-0.4_beta_allinone_setup.exe"
!else
OutFile "hugin-0.4_beta_setup.exe"
!endif
Caption "${DISPLAY_NAME}"

# [Licence Attributes]
LicenseText "Hugin is distributed under the GNU General Public License :"
LicenseData "LICENCE"

# [Directory Selection]
InstallDir "$PROGRAMFILES\hugin"
DirText "Select the directory to install Hugin in:"

# [Additional Installer Settings ]
SetCompress force
SetCompressor lzma

;--------------------------------
;Interface Settings

ShowInstDetails show
AutoCloseWindow false
SilentInstall normal
CRCCheck on
SetCompress auto
SetDatablockOptimize on
;SetOverwrite ifnewer
XPStyle on

ComponentText "Choose components"

# [Background Gradient]
BGGradient off

!define MUI_ABORTWARNING

;--------------------------------

# [Pages]

  !insertmacro MUI_PAGE_LICENSE "LICENCE"
  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  
  !define MUI_FINISHPAGE_RUN "$INSTDIR\hugin.exe"
  !define MUI_FINISHPAGE_NOREBOOTSUPPORT

  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
;Languages

  !insertmacro MUI_LANGUAGE "English"
;--------------------------------

# [Files]
Section "Hugin program files"
  SectionIn RO

  SetOutPath "$INSTDIR"
  File "automatch.exe"
  File "autooptimiser.exe"
  File "autopano_old.exe"
  File "hugin.exe"
  File "LICENCE"
  File "mingwm10.dll"
  File "nona.exe"
  File "nona_gui.exe"
  File "panosifter.exe"
  File "sift_keypoints.exe"
  File "zhang_undistort.exe"
  File "jpeg62.dll"
  File "libpng12.dll"
  File "libtiff3.dll"
  File "zlib1.dll"
  File "panoglview.exe"
  
!ifdef HUGIN_ALLINONE
  File "pano12.dll"
  File "PTStitcher.exe"
  File "PTOptimizer.exe"
  File "autopano.exe"
  File "enblend.exe"
  WriteRegStr HKCU "Software\hugin\AutoPanoKolor" "AutopanoExe" "$INSTDIR\autopano.exe"
  WriteRegStr HKCU "Software\hugin\Enblend" "EnblendExe" "$INSTDIR\enblend.exe"
  WriteRegStr HKCU "Software\hugin\Panotools" "PTStitcherExe" "$INSTDIR\PTStitcher.exe"
  WriteRegStr HKCU "Software\hugin\Panotools" "PTOptimizerExe" "$INSTDIR\PTOptimizer.exe"
!endif

  SetOutPath "$INSTDIR\locale"
  File /r "locale\*"
  SetOutPath "$INSTDIR\xrc"
  File /r "xrc\*"

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Hugin "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "DisplayName" "${DISPLAY_NAME}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "NoRepair" 1
  WriteUninstaller "$INSTDIR\uninstall.exe"

SectionEnd

# [Shortcuts]
Section "Create shortcuts in Start Menu"
  ;try to read from registry if last installation installed for All Users/Current User
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin\Backup" \
      "Shortcuts"
  StrCmp $0 "" cont exists
  cont:

  SetShellVarContext all
  MessageBox MB_YESNO "Do you want to install Hugin for all users on this computer ?" IDYES AllUsers
  SetShellVarContext current
AllUsers:
  StrCpy $0 $SMPROGRAMS
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin\Backup" \
      "Shortcuts" "$0"

exists:
  CreateDirectory "$0\Hugin"
  SetOutPath $INSTDIR
  CreateShortCut "$0\Hugin\Hugin.lnk" "$INSTDIR\hugin.exe"
  CreateShortCut "$0\Hugin\PanoGLView.lnk" "$INSTDIR\panoglview.exe"
  CreateShortCut "$0\Hugin\Uninstall Hugin.lnk" "$INSTDIR\uninstall.exe"
SectionEnd

Section "Create Quick Launch shortcut"
  SetShellVarContext current
  CreateShortCut "$QUICKLAUNCH\Hugin.lnk" "$INSTDIR\hugin.exe"
SectionEnd

# [File association]
Section "Associate .pto files with Hugin"
  StrCpy $0 $INSTDIR\hugin.exe
  WriteRegStr HKCR ".pto" "" "Hugin.pto"
  WriteRegStr HKCR "Hugin.pto" "" "Hugin Project File"
  WriteRegStr HKCR "Hugin.pto\DefaultIcon" "" '$0,0'
  WriteRegStr HKCR "Hugin.pto\Shell\Open\Command" "" '$0 "%1"'
  Call RefreshShellIcons
SectionEnd

;--------------------------------

# [UnInstallation]

UninstallText "This program will uninstall Hugin ${HUGIN_VERSION}, continue ?"
ShowUninstDetails show

Section "Uninstall"
  Delete "$INSTDIR\automatch.exe"
  Delete "$INSTDIR\autooptimiser.exe"
  Delete "$INSTDIR\autopano_old.exe"
  Delete "$INSTDIR\hugin.exe"
  Delete "$INSTDIR\LICENCE"
  Delete "$INSTDIR\mingwm10.dll"
  Delete "$INSTDIR\nona.exe"
  Delete "$INSTDIR\nona_gui.exe"
  Delete "$INSTDIR\panosifter.exe"
  Delete "$INSTDIR\sift_keypoints.exe"
  Delete "$INSTDIR\zhang_undistort.exe"
  Delete "$INSTDIR\jpeg62.dll"
  Delete "$INSTDIR\libpng12.dll"
  Delete "$INSTDIR\libtiff3.dll"
  Delete "$INSTDIR\zlib1.dll"
  Delete "$INSTDIR\panoglview.exe"
  Delete "$INSTDIR\uninstall.exe"
    
!ifdef HUGIN_ALLINONE
  Delete "$INSTDIR\pano12.dll"
  Delete "$INSTDIR\PTStitcher.exe"
  Delete "$INSTDIR\PTOptimizer.exe"
  Delete "$INSTDIR\autopano.exe"
  Delete "$INSTDIR\enblend.exe"
!endif

  RMDir /r "$INSTDIR\locale"
  RMDir /r "$INSTDIR\xrc"

  RMDir "$INSTDIR"

  ; Remove icons
  ReadRegStr $0 HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin\Backup" \
      "Shortcuts"
  Delete "$0\Hugin\Hugin.lnk"
  Delete "$0\Hugin\Uninstall Hugin.lnk"
  RMDir  "$0\Hugin"
  SetShellVarContext current
  Delete "$QUICKLAUNCH\Hugin.lnk"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin"
  DeleteRegKey HKLM SOFTWARE\Hugin
  
  ; Remove file association
  DeleteRegKey HKCR ".pto"
  DeleteRegKey HKCR "Hugin.pto"
  Call un.RefreshShellIcons

SectionEnd

;--------------------------------

; Functions

;http://nsis.sourceforge.net/archive/viewpage.php?pageid=202
;After changing file associations, you can call this macro to refresh the shell immediatly. 
;It calls the shell32 function SHChangeNotify. This will force windows to reload your changes from the registry.
!define SHCNE_ASSOCCHANGED 0x08000000
!define SHCNF_IDLIST 0

Function RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

Function un.RefreshShellIcons
  ; By jerome tremblay - april 2003
  System::Call 'shell32.dll::SHChangeNotify(i, i, i, i) v \
  (${SHCNE_ASSOCCHANGED}, ${SHCNF_IDLIST}, 0, 0)'
FunctionEnd

;--------------------------------

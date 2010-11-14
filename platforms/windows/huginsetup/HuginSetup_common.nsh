; Hugin installer script
!define VERSION 0.2.11.0
; Author: thePanz (thepanz@gmail.com)

;--------------------------------
;Include Modern UI
  !include "MUI2.nsh"
  
  !addplugindir "./Plugins"
;--------------------------------
;General  
  !define HUGIN_BIN_ARCHIVE "hugin_bin${ARCH_TYPE}.7z"
  !define HUGIN_SHARE_ARCHIVE "hugin_share${ARCH_TYPE}.7z"
  !define HUGIN_SIZE 127000 ; .7z file - difference
  
  !define HUGIN_DOC_ARCHIVE "hugin_doc.7z"
  !define HUGIN_DOC_SIZE 400 ; .7z file - difference

  ; General CP Disclaimer
  !define CP_LICENSE_DISCLAIMER $(License_ControlPointDisclaimer)
  
  ;Name and file
  Name "Hugin ${HUGIN_VERSION}"
  BrandingText "Hugin Setup - by thePanz"
  
  OutFile "HuginSetup_${HUGIN_VERSION}-${HUGIN_VERSION_BUILD}_x${ARCH_TYPE}.exe"
  SetCompressor lzma
    
  ;Get installation folder from registry if available
  ;InstallDirRegKey HKCU "Software\Modern UI Test" ""

  ;Request application privileges for Windows Vista
  RequestExecutionLevel admin

;--------------------------------
;Interface Configuration
  !define MUI_VERSION "${HUGIN_VERSION}-${HUGIN_VERSION_BUILD}"
  !define MUI_COMPONENTSPAGE_SMALLDESC ; Small description in Component selection

  !define MUI_HEADERIMAGE
  !define MUI_HEADERIMAGE_BITMAP "${NSISDIR}\Contrib\Graphics\Header\orange.bmp" ; optional
  !define MUI_HEADERIMAGE_UNBITMAP "${NSISDIR}\Contrib\Graphics\Header\orange-uninstall.bmp" ; optional
  
  !define MUI_ICON "Graphics\hugin-installer-icon.ico"
  !define MUI_UNICON "Graphics\hugin-uninstaller-icon.ico" 
  
  !define MUI_ABORTWARNING
  
  !define MUI_FINISHPAGE_NOAUTOCLOSE
  !define MUI_WELCOMEPAGE_TEXT "${DEV_WARNING_TOGGLE}"
  !define MUI_WELCOMEFINISHPAGE_BITMAP "Graphics\Hugin-sidebar.bmp"
  !define MUI_WELCOMEFINISHPAGE_BITMAP_NOSTRETCH
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP "Graphics\Hugin-sidebar.bmp"
  !define MUI_UNWELCOMEFINISHPAGE_BITMAP_NOSTRETCH
  
  !define MUI_FINISHPAGE_RUN $INSTDIR\bin\hugin.exe
  !define MUI_FINISHPAGE_RUN_NOTCHECKED
  !define MUI_FINISHPAGE_LINK $(TEXT_FinishPageLink)
  !define MUI_FINISHPAGE_LINK_LOCATION "http://hugin.sourceforge.net"  
;--------------------------------
;Pages
  ;Show all languages, despite user's codepage
  !define MUI_LANGDLL_ALLLANGUAGES

  !insertmacro MUI_PAGE_WELCOME
  ;!insertmacro MUI_PAGE_LICENSE $(License_Hugin)
  !insertmacro MUI_PAGE_LICENSE "Licenses\GPLv2.txt"
  !insertmacro MUI_PAGE_COMPONENTS
  
  ; Additional license agreement
  !define MUI_PAGE_HEADER_TEXT $(TEXT_ControlPointDisclaimerTitle)
  !define MUI_PAGE_HEADER_SUBTEXT $(TEXT_ControlPointDisclaimerText)
  !define MUI_LICENSEPAGE_TEXT_BOTTOM  $(TEXT_ControlPointDisclaimerBottom)
  !define MUI_PAGE_CUSTOMFUNCTION_PRE skipControlPointsDisclaimer
  !insertmacro MUI_PAGE_LICENSE ${CP_LICENSE_DISCLAIMER}
  
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH
  
  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_COMPONENTS
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH  
  

 
;--------------------------------
; Include Functions
  !include "Functions\RegistryFunctions.nsh"
  !include "Functions\DownloadFunctions.nsh"

;--------------------------------
; Installer Sections

; Core Hugin files
Section "!Hugin ${HUGIN_VERSION}-${HUGIN_VERSION_BUILD}" SecHugin
  SectionIn RO
  AddSize ${HUGIN_SIZE}
  SetOutPath "$INSTDIR"
  
  Call CleanRegistryOnInstallIfSelected

  SetCompress off
  DetailPrint $(TEXT_HuginExtracting)
  SetDetailsPrint listonly
  
  SetDetailsPrint both
  File ${HUGIN_BIN_ARCHIVE}
  File ${HUGIN_SHARE_ARCHIVE}
  SetCompress auto
  
  ; Details mode - unpacking promt generated from second param, use
  ; %s to insert unpack details like "10% (5 / 10 MB)"
  Nsis7z::ExtractWithDetails ${HUGIN_BIN_ARCHIVE} $(TEXT_HuginExtractDetails)
  Nsis7z::ExtractWithDetails ${HUGIN_SHARE_ARCHIVE} $(TEXT_HuginDocExtractDetails)
  
  Delete "$OUTDIR\${HUGIN_BIN_ARCHIVE}" 
  Delete "$OUTDIR\${HUGIN_SHARE_ARCHIVE}"
  
  ; Call AddCPAutoPanoSiftCStacked
  Call AddCPAlignImageStack
  
  ; register .pto files with hugin
  WriteRegStr HKCR ".pto" "" "HuginProject"
  WriteRegStr HKCR "HuginProject" "" "Hugin PTO"
  WriteRegStr HKCR "HuginProject\DefaultIcon" "" "$INSTDIR\share\hugin\xrc\data\hugin-pto-icon.ico,0"
  
  Call WriteUninstallRegistry
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
SectionEnd

Section /o $(TEXT_SecCleanRegistrySettings) SecCleanRegistrySettings
  ; done during install
SectionEnd

; Hugin documentation section
Section $(TEXT_SecHuginDoc) SecHuginDoc
  AddSize ${HUGIN_DOC_SIZE}
  SetOutPath "$INSTDIR"

  SetCompress off
  DetailPrint $(TEXT_HuginDocExtracting)
  SetDetailsPrint listonly
  
  SetDetailsPrint both
  File ${HUGIN_DOC_ARCHIVE}
  SetCompress auto
  
  ; Details mode - unpacking promt generated from second param, use
  ; %s to insert unpack details like "10% (5 / 10 MB)"
  Nsis7z::ExtractWithDetails ${HUGIN_DOC_ARCHIVE} $(TEXT_HuginDocExtractDetails)
  
  Delete "$OUTDIR\${HUGIN_DOC_ARCHIVE}"   
  
  ; TODO: Add Documentation menu links
SectionEnd

SectionGroup /e $(TEXT_SecShortcuts) SecShortcuts
  Section $(TEXT_SecShortcutPrograms) SecShortcutPrograms
    AddSize 1
    ; SetShellVarContext all
    CreateDirectory "$SMPROGRAMS\Hugin"
    CreateShortCut "$SMPROGRAMS\Hugin\Uninstall.lnk" "$INSTDIR\uninstall.exe"
    CreateShortCut "$SMPROGRAMS\Hugin\Hugin.lnk" "$INSTDIR\bin\hugin.exe"
	CreateShortCut "$SMPROGRAMS\Hugin\Enblend Droplet.lnk" "$INSTDIR\bin\enblend_droplet.bat"
	CreateShortCut "$SMPROGRAMS\Hugin\Enblend Droplet 360.lnk" "$INSTDIR\bin\enblend_droplet_360.bat"
	CreateShortCut "$SMPROGRAMS\Hugin\Enfuse Droplet.lnk" "$INSTDIR\bin\enfuse_droplet.bat"
	CreateShortCut "$SMPROGRAMS\Hugin\Enfuse Droplet 360.lnk" "$INSTDIR\bin\enfuse_droplet_360.bat"
	CreateShortCut "$SMPROGRAMS\Hugin\Enfuse Align Droplet.lnk" "$INSTDIR\bin\enfuse_align_droplet.bat"
	CreateShortCut "$SMPROGRAMS\Hugin\Enfuse Auto Align Droplet.lnk" "$INSTDIR\bin\enfuse_auto_align_droplet.bat"
	CreateShortCut "$SMPROGRAMS\Hugin\Enfuse Auto Droplet.lnk" "$INSTDIR\bin\enfuse_auto_droplet.bat"
  SectionEnd
  
  Section $(TEXT_SecShortcutDesktop) SecShortcutDesktop
    AddSize 1
    CreateShortCut "$DESKTOP\Hugin.lnk" "$INSTDIR\bin\hugin.exe" ""
  SectionEnd
SectionGroupEnd

; Grouping ControlPoint Generators
SectionGroup /e $(TEXT_SecCPGenerators) SecCPGenerators

  ; External upgradable CP generators download and setup  
  !include "CPGenerators\Match-n-shift.nsh"
  !include "CPGenerators\Autopano-sift-c.nsh"
  !include "CPGenerators\Autopano-sift-c-lemur.nsh"
  !include "CPGenerators\Autopano-sift-jenny.nsh"
  ; !include "CPGenerators\Autopano-sift-nowozin.nsh"
  !include "CPGenerators\Panomatic.nsh"  
  
SectionGroupEnd

;--------------------------------
;Descriptions  
  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecHugin} $(DESC_SecHugin)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecHuginDoc} $(DESC_SecHuginDoc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCPGenerators} $(DESC_SecCPGenerators)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecPanomatic} $(DESC_SecPanomatic)
    
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAutopano_Jenny} $(DESC_SecAutopano_Jenny)
    ; !insertmacro MUI_DESCRIPTION_TEXT ${SecAutopano_Nowozin} $(DESC_SecAutopano_Nowozin)    
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAutopano_SIFTC} $(DESC_SecAutopano_SIFTC)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAutopano_SIFTC_Lemur} $(DESC_SecAutopano_SIFTC_Lemur)
    
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMatchNShift} $(DESC_SecMatchNShift)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCleanRegistrySettings} $(DESC_SecCleanRegistrySettings)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcuts} $(DESC_SecShortcuts)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcutPrograms} $(DESC_SecShortcutPrograms)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecShortcutDesktop} $(DESC_SecShortcutDesktop)
  !insertmacro MUI_FUNCTION_DESCRIPTION_END
 
;--------------------------------
;Uninstaller Section

Section "un.Uninstall Hugin"
  SectionIn RO
  AddSize ${HUGIN_SIZE}
  Delete "$INSTDIR\Uninstall.exe"

  RMDir /r "$INSTDIR"
  ; SetShellVarContext all  
  Delete "$SMPROGRAMS\Hugin\Hugin.lnk"
  Delete "$SMPROGRAMS\Hugin\Uninstall.lnk"
  RMDir /r "$SMPROGRAMS\Hugin"

  ;Delete Desktop shortcut (if exists)
  Delete "$DESKTOP\Hugin.lnk"
   
  ;Delete Uninstaller And Unistall Registry Entries
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" 
  
  ;Delete .pto file assiciation
  DeleteRegKey HKLM "SOFTWARE\Classes\.pto"
  DeleteRegKey HKLM "SOFTWARE\Classes\HuginProject"
  
  ; DeleteRegKey /ifempty HKCU "Software\Modern UI Test"
SectionEnd

Section /o "un.$(TEXT_SecCleanRegistrySettings)"
  DeleteRegKey  HKCU "Software\Hugin"
SectionEnd


;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"
  !insertmacro MUI_LANGUAGE "German"
  !insertmacro MUI_LANGUAGE "Italian"

  ; Language Files
  !include "Licenses\licenses.nsh"
  
  !include "Languages\EN.nsh"
  !include "Languages\DE.nsh"
  !include "Languages\IT.nsh"    
  
; ----------------------
; Installer versions
  VIProductVersion ${VERSION}
  VIAddVersionKey /LANG=${LANG_ENGLISH} "ProductName" "Hugin ${HUGIN_VERSION}-${HUGIN_VERSION_BUILD}"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileVersion" ${VERSION}
  VIAddVersionKey /LANG=${LANG_ENGLISH} "FileDescription" "Hugin Setup - by thePanz"
  VIAddVersionKey /LANG=${LANG_ENGLISH} "LegalCopyright" ""

;Function that calls a messagebox when installation finished correctly
;Function .onInstSuccess
;  MessageBox MB_OK "You have successfully installed Hugin ${HUGIN_VERSION}${HUGIN_VERSION_BUILD}."
;FunctionEnd
 
;Function un.onUninstSuccess
;  MessageBox MB_OK "You have successfully uninstalled Hugin ${HUGIN_VERSION}-${HUGIN_VERSION_BUILD}."
;FunctionEnd

; --------------------------
; Functions

Function .onInit

  !insertmacro MUI_LANGDLL_DISPLAY
  
FunctionEnd

; License skipping functions
Function skipControlPointsDisclaimer
  SectionGetFlags ${SecCPGenerators} $R0 
  IntOp $R1 $R0 & ${SF_PSELECTED}
  IntCmp $R1 ${SF_PSELECTED} show
  
  IntOp $R1 $R0 & ${SF_SELECTED}
  IntCmp $R1 ${SF_SELECTED} show
  Abort
  show:
FunctionEnd


;Clean registry on install if selected
Function CleanRegistryOnInstallIfSelected
 
  SectionGetFlags ${SecCleanRegistrySettings} $R0 
  IntOp $R0 $R0 & ${SF_SELECTED} 
  IntCmp $R0 ${SF_SELECTED} CleanRegistry DoNotCleanRegistry
 
  CleanRegistry: 
    DeleteRegKey  HKCU "Software\Hugin" 
 
  DoNotCleanRegistry:
    ;do nothing
FunctionEnd

; Add Align_Image_Stack control point generator settings to registry
; Align_image_stack.exe is provided directly from Hugin 
Function AddCPAlignImageStack
  StrCpy $R0 1 ; R0 = Type
  StrCpy $R1 1 ; R1 = Option
  StrCpy $R2 "$INSTDIR\bin\align_image_stack.exe" ; R2 = Program
  StrCpy $R3 "-f %v -v -p %o %i" ; R3 = Arguments
  StrCpy $R4 "Align image stack";  R4 = Description
  Call ControlPointRegistryAdd
FunctionEnd









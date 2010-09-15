; Hugin installer script
!define VERSION 0.2.6.0 ; (2010-09-08)
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
  
  ; !define MUI_ICON "Graphics\hugin-installer-icon.ico"
  
  !define MUI_ABORTWARNING
  
  !define MUI_FINISHPAGE_NOAUTOCLOSE
  !define MUI_WELCOMEPAGE_TEXT $(TEXT_WelcomePage)
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
; Installer Sections

; Core Hugin files
Section "!Hugin ${HUGIN_VERSION}-${HUGIN_VERSION_BUILD}" SecHugin
  SectionIn RO
  AddSize ${HUGIN_SIZE}
  SetOutPath "$INSTDIR"

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
  
  Call AddCPAutoPanoSiftCStacked
  Call AddCPAlignImageStack
  
  Call WriteUninstallRegistry
  
  ;Create uninstaller
  WriteUninstaller "$INSTDIR\Uninstall.exe"
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
  !include "CPGenerators\Panomatic.nsh"
  !include "CPGenerators\Autopano.nsh"
  
SectionGroupEnd

;--------------------------------
;Descriptions  
  ;Assign language strings to sections
  !insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecHugin} $(DESC_SecHugin)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecHuginDoc} $(DESC_SecHuginDoc)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecCPGenerators} $(DESC_SecCPGenerators)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecPanomatic} $(DESC_SecPanomatic)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAutopano} $(DESC_SecAutopano)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecAutopanoSIFTC} $(DESC_SecAutopanoSIFTC)
    !insertmacro MUI_DESCRIPTION_TEXT ${SecMatchNShift} $(DESC_SecMatchNShift)
    
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

Section /o "un.$(TEXT_UN_SecCleanRegistrySettings)"
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


; Write registry settings for CP generators
; R0 = Type
; R1 = Option
; R2 = Program
; R3 = Arguments
; R4 = Description
; R5 = [...]
Function ControlPointRegistryAdd
  ReadRegDWORD $0 HKCU "Software\hugin\AutoPano" "AutoPanoCount"
  ; Make sure it's an Integer (empty string if AutoPanoCount doesn't exists)
  IntOp $0 $0 + 0
  
  ; MessageBox MB_OK "Adding $R2: '$R4' ($R3) into AutoPano_$0"

  ; Writing settings
  WriteRegDWORD HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Type" "$R0"
  WriteRegDWORD HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Option" "$R1"
  WriteRegStr   HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Program" "$R2"
  WriteRegStr   HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Arguments" "$R3"
  WriteRegStr   HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Description" "$R4"

  IntOp $0 $0 + 1
  ; MessageBox MB_OK "How many CP now?? $0"
  WriteRegDWORD HKCU "Software\hugin\AutoPano" "AutoPanoCount" $0
FunctionEnd

; Write registry settings for CP Stacked
; R0 = Type [Matcher|Stack]
; R1 = Autopano_ID (-1 for last setting present)
; R2 = Program
; R3 = Arguments
Function ControlPointRegistryAddMulti
  IntCmp $R1 -1 0 ok
    ReadRegDWORD $0 HKCU "Software\hugin\AutoPano" "AutoPanoCount"
    ; Make sure it's an Integer (empty string if AutoPanoCount doesn't exists)
    IntOp $0 $0 - 1
    StrCpy $R1 $0
  ok:
  
  MessageBox MB_OK "Adding $R0 as $R2: '$R4' ($R3) into AutoPano_$0"

  ; Writing settings
  WriteRegStr   HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Program$R0" "$R2"
  WriteRegStr   HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Arguments$R0" "$R3"
FunctionEnd


; Return on top of stack the total size of the selected (installed) sections, formated as DWORD
; Assumes no more than 256 sections are defined
Var GetInstalledSize.total
Function GetInstalledSize
	Push $0
	Push $1
	StrCpy $GetInstalledSize.total 0
	${ForEach} $1 0 256 + 1
		${if} ${SectionIsSelected} $1
			SectionGetSize $1 $0
			IntOp $GetInstalledSize.total $GetInstalledSize.total + $0
		${Endif}
	${Next}
	Pop $1
	Pop $0
	IntFmt $GetInstalledSize.total "0x%08X" $GetInstalledSize.total
	Push $GetInstalledSize.total
FunctionEnd

; Add Align_Image_stack control point generator settings
Function AddCPAlignImageStack
  StrCpy $R0 1 ; R0 = Type
  StrCpy $R1 1 ; R1 = Option
  StrCpy $R2 "$INSTDIR\bin\align_image_stack.exe" ; R2 = Program
  StrCpy $R3 "-f %v -v -p %o %i" ; R3 = Arguments
  StrCpy $R4 "Align image stack";  R4 = Description
  Call ControlPointRegistryAdd
FunctionEnd

; Add Align_Image_stack control point generator settings
Function AddCPAutoPanoSiftCStacked
  StrCpy $R0 4 ; R0 = Type
  StrCpy $R1 1 ; R1 = Option
  StrCpy $R2 "$INSTDIR\bin\generatekeys.exe" ; R2 = Program
  StrCpy $R3 "%i %k 800" ; R3 = Arguments
  StrCpy $R4 "Autopano-SIFT-C (multirow/stacked)";  R4 = Description
  Call ControlPointRegistryAdd
  
  StrCpy $R0 "Matcher" ; R0 = Type [Matcher|Stack]
  StrCpy $R1 "-1"; R1 = Autopano_ID (-1 for last setting present)
  StrCpy $R2 "$INSTDIR\bin\autopano.exe" ; R2 = Program
  StrCpy $R3 "--maxmatches %p %o %k"; R3 = Arguments
  Call ControlPointRegistryAddMulti
  
  StrCpy $R0 "Stack" ; R0 = Type [Matcher|Stack]
  StrCpy $R1 "-1"; R1 = Autopano_ID (-1 for last setting present)
  StrCpy $R2 "$INSTDIR\bin\align_image_stack.exe" ; R2 = Program
  StrCpy $R3 "-f %v -v -p %o %i"; R3 = Arguments
  Call ControlPointRegistryAddMulti  
  
FunctionEnd


; Write uninstall informations into Windows Register
Function WriteUninstallRegistry
  ; Write uninstall information to the registry
  ; as in http://nsis.sourceforge.net/Add_uninstall_information_to_Add/Remove_Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "DisplayName" "Hugin ${HUGIN_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "DisplayIcon" "$INSTDIR\bin\hugin.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "DisplayVersion" "${HUGIN_VERSION} ${HUGIN_VERSION_BUILD}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "Publisher" "Hugin Group Inc."
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "URLInfoAbout" "http://hugin.sourceforge.net"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "HelpLink" "http://groups.google.com/group/hugin-ptx"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "InstallLocation" "$INSTDIR"
  ; No Repair/Modify options
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "NoRepair" 1
  ; Setting Install Size
  Call GetInstalledSize
  Pop $0
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "EstimatedSize" $0

FunctionEnd

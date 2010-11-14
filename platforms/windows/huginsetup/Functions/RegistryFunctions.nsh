; Common functions for Hugin Setup
; @Author: thePanz @gmail.com

; Write Hugin uninstall informations into Windows Register
Function WriteUninstallRegistry
  ; Write uninstall information to the registry
  ; as in http://nsis.sourceforge.net/Add_uninstall_information_to_Add/Remove_Programs
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "DisplayName" "Hugin ${HUGIN_VERSION}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "DisplayIcon" "$INSTDIR\bin\hugin.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "DisplayVersion" "${HUGIN_VERSION} ${HUGIN_VERSION_BUILD}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Hugin" "Publisher" "The Hugin Development Team"
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

; Write registry settings for CP generators
; R0 = Type
; R1 = Option
; R2 = Program
; R3 = Arguments
; R4 = Description
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
  WriteRegDWORD HKCU "Software\hugin\AutoPano" "Default" $0
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
  
  ; MessageBox MB_OK "Adding $R0 as $R2: '$R4' ($R3) into AutoPano_$0"

  ; Writing settings
  WriteRegStr   HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Program$R0" "$R2"
  WriteRegStr   HKCU "Software\Hugin\AutoPano\AutoPano_$0" "Arguments$R0" "$R3"
FunctionEnd

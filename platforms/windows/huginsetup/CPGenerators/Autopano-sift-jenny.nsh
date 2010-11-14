; Autopano CP Generator
!define AUTOPANO_JENNY_VERSION "1.03 (A.Jenny)"
!define AUTOPANO_JENNY_SIZE 700
!define AUTOPANO_JENNY_DL_URL "http://autopano.kolor.com/autopano_v103.zip"

; Autopano download and install
Section /o "Autopano ${AUTOPANO_JENNY_VERSION}" SecAutopano_Jenny
  AddSize ${AUTOPANO_JENNY_SIZE} ; Autopano size
  SetOutPath "$INSTDIR\CPG\Autopano_Jenny"
  
  DetailPrint "$(TEXT_CPDownloading) Autopano ${AUTOPANO_JENNY_VERSION}"
  ; SetDetailsPrint both
  Dialer::AttemptConnect
  NSISdl::download /TIMEOUT=30000 ${AUTOPANO_JENNY_DL_URL} "$TEMP\autopano.zip"
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    MessageBox MB_OK "$(TEXT_ERROR_DownloadFailed): Autopano SIFT ${AUTOPANO_JENNY_VERSION}" 
    Goto end
    
  success:
      DetailPrint "Autopano ${AUTOPANO_JENNY_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Autopano ${AUTOPANO_JENNY_VERSION}"
      ; File /nonfatal "autopano.zip"
      nsisunz::UnzipToLog /noextractpath /file "autopano_v103zip/autopano.exe" "$TEMP\autopano.zip" "$INSTDIR\CPG\Autopano_Jenny" 
      Delete "$TEMP\autopano.zip"
      
      StrCpy $R0 0 ; R0 = Type
      StrCpy $R1 1 ; R1 = Option
      StrCpy $R2 "$INSTDIR\CPG\Autopano_Jenny\autopano.exe" ; R2 = Program
      StrCpy $R3 "/allinone /path:%d /keys:%p /project:oto /name:%o /size:1024 /f %i" ; R3 = Arguments
      StrCpy $R4 "Autopano ${AUTOPANO_JENNY_VERSION}";  R4 = Description
      Call ControlPointRegistryAdd
  
  end:
SectionEnd
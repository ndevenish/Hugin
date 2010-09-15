; Autopano CP Generator
!define AUTOPANO_VERSION "1.03"
!define AUTOPANO_SIZE 700
!define AUTOPANO_DL_URL "http://autopano.kolor.com/autopano_v103.zip"
!define AUTOPANO_LICENSE_FILE "Licenses/Autopano.txt"
; Autopano download and install
Section /o "Autopano ${AUTOPANO_VERSION}" SecAutopano
  AddSize ${AUTOPANO_SIZE} ; Autopano size
  
  DetailPrint "$(TEXT_CPDownloading) Autopano ${AUTOPANO_VERSION}"
  ; SetDetailsPrint both
  Dialer::AttemptConnect
  NSISdl::download /TIMEOUT=30000 ${AUTOPANO_DL_URL} "$TEMP\autopano.zip"
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    Abort
  success:
      DetailPrint "Autopano ${AUTOPANO_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Autopano ${AUTOPANO_VERSION}"
      ; File /nonfatal "autopano.zip"
      nsisunz::UnzipToLog /noextractpath /file "autopano_v${AUTOPANO_VERSION}zip/autopano.exe" "$TEMP\autopano.zip" "$INSTDIR\bin" 
      Delete "$TEMP\autopano.zip"
      
      StrCpy $R0 0 ; R0 = Type
      StrCpy $R1 1 ; R1 = Option
      StrCpy $R2 "$INSTDIR\bin\autopano.exe" ; R2 = Program
      StrCpy $R3 "/allinone /path:%d /keys:%p /project:oto /name:%o /size:1024 /f %i" ; R3 = Arguments
      StrCpy $R4 "Autopano ${AUTOPANO_VERSION}";  R4 = Description
      Call ControlPointRegistryAdd
  
SectionEnd
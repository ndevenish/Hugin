
; Panomatic CP Generator
; TODO: find a way to use sourceforge mirrors system..
!define PANOMATIC_VERSION "0.9.4"
!define PANOMATIC_SIZE 800
!define PANOMATIC_DL_URL "http://netcologne.dl.sourceforge.net/project/panomatic/panomatic/0.9.4/panomatic-0.9.4-win32.zip"
!define PANOMATIC_LICENSE_FILE "Licenses/Panomatic.txt"

; Panomatic download and install
Section /o "Panomatic ${PANOMATIC_VERSION}" SecPanomatic
  AddSize ${PANOMATIC_SIZE}  ; Panomatic size
  SetOutPath "$INSTDIR\bin"
  
  DetailPrint "$(TEXT_CPDownloading) Panomatic ${PANOMATIC_VERSION}"
  ; SetDetailsPrint listonly
  
  ; SetDetailsPrint both
  Dialer::AttemptConnect
  NSISdl::download /TIMEOUT=30000 ${PANOMATIC_DL_URL} "$TEMP\panomatic.zip"
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    Abort
  success:
      DetailPrint "Panomatic ${PANOMATIC_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Panomatic ${PANOMATIC_VERSION}"
      nsisunz::UnzipToLog "$TEMP\panomatic.zip" "$INSTDIR\bin"
      Delete "$TEMP\panomatic.zip"
      
      StrCpy $R0 1 ; R0 = Type
      StrCpy $R1 1 ; R1 = Option
      StrCpy $R2 "$INSTDIR\bin\panomatic.exe" ; R2 = Program
      StrCpy $R3 "-o %o %i" ; R3 = Arguments
      StrCpy $R4 "Panomatic ${PANOMATIC_VERSION}";  R4 = Description
      Call ControlPointRegistryAdd
SectionEnd
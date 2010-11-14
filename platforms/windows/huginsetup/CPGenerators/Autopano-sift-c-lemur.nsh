; Autopano-sift-c
!define AUTOPANO_SIFTC_LEMUR_VERSION "2.5.2 (Lemur)"
!define AUTOPANO_SIFTC_LEMUR_SIZE 5000
; !define AUTOPANO_SIFTC_LEMUR_DL_URL "ftp://tksftp:TKSpwd1@tksharpless.net/autopano-sift-c.exe"
!define AUTOPANO_SIFTC_LEMUR_DL_URL "http://dl.dropbox.com/u/2070182/hugin/autopano-sift-c-lemur_2.5.2.7z"

Section /o "Autopano-SIFT-C Multirow/Stacked ${AUTOPANO_SIFTC_LEMUR_VERSION}" SecAutopano_SIFTC_Lemur
  ; SectionIn RO
  AddSize ${AUTOPANO_SIFTC_LEMUR_SIZE}
  SetOutPath "$INSTDIR\CPG\Autopano_SiftC_lemur"
  
  DetailPrint "$(TEXT_CPDownloading) Autopano-sift-C (Multirow/Stacked) ${AUTOPANO_SIFTC_LEMUR_VERSION}"
  
  Dialer::AttemptConnect
  NSISdl::download /TIMEOUT=30000 ${AUTOPANO_SIFTC_LEMUR_DL_URL} "$TEMP\autopano-sift-c-lemur.7z"
  
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    MessageBox MB_OK "$(TEXT_ERROR_DownloadFailed): Autopano-SIFT-C ${AUTOPANO_SIFTC_LEMUR_VERSION}" 
    Goto end

  success:
      DetailPrint "Autopanp-SIFT-C ${AUTOPANO_SIFTC_LEMUR_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Autopano-SIFT-C ${AUTOPANO_SIFTC_LEMUR_VERSION}"
      ; File /nonfatal "autopano.zip"
      Nsis7z::Extract "$TEMP\autopano-sift-c-lemur.7z"
      
      Delete "$TEMP\autopano-sift-c.7z"
      
      ; Add the multirow/Stacked settings
      Call AddCPAutoPanoSiftCStacked

  end:
SectionEnd

; Add Autopano-Sift-C STACKED settings
; @todo: check if Autopano-sift-c is installed (with generatekeys.exe and align_image_stack.exe)
Function AddCPAutoPanoSiftCStacked
  StrCpy $R0 4 ; R0 = Type
  StrCpy $R1 1 ; R1 = Option
  StrCpy $R2 "$INSTDIR\CPG\Autopano_SiftC_lemur\generatekeys.exe" ; R2 = Program
  StrCpy $R3 "%i %k 800" ; R3 = Arguments
  StrCpy $R4 "Autopano-SIFT-C (multirow/stacked)";  R4 = Description
  Call ControlPointRegistryAdd
  
  StrCpy $R0 "Matcher" ; R0 = Type [Matcher|Stack]
  StrCpy $R1 "-1"; R1 = Autopano_ID (-1 for last setting present)
  StrCpy $R2 "$INSTDIR\CPG\Autopano_SiftC_lemur\autopano.exe" ; R2 = Program
  StrCpy $R3 "--maxmatches %p %o %k"; R3 = Arguments
  Call ControlPointRegistryAddMulti
  
  StrCpy $R0 "Stack" ; R0 = Type [Matcher|Stack]
  StrCpy $R1 "-1"; R1 = Autopano_ID (-1 for last setting present)
  StrCpy $R2 "$INSTDIR\bin\align_image_stack.exe" ; R2 = Program
  StrCpy $R3 "-f %v -v -p %o %i"; R3 = Arguments
  Call ControlPointRegistryAddMulti  
  
FunctionEnd
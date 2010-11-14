; Autopano-sift-c
!define AUTOPANO_SIFTC_VERSION "2.5.2 (TKSharpless)"
!define AUTOPANO_SIFTC_SIZE 1500
; !define AUTOPANO_SIFTC_DL_URL "ftp://tksftp:TKSpwd1@tksharpless.net/autopano-sift-c.exe"
!define AUTOPANO_SIFTC_DL_URL "http://dl.dropbox.com/u/2070182/hugin/autopano-sift-c_2.5.2.7z"

Section /o "Autopano-SIFT-C ${AUTOPANO_SIFTC_VERSION}" SecAutopano_SIFTC
  ; SectionIn RO
  AddSize ${AUTOPANO_SIFTC_SIZE}
  SetOutPath "$INSTDIR\CPG\Autopano_SIFTC"
  
  DetailPrint "$(TEXT_CPDownloading) Autopano-sift-C ${AUTOPANO_SIFTC_VERSION}"
  
  Dialer::AttemptConnect
  NSISdl::download /TIMEOUT=30000 ${AUTOPANO_SIFTC_DL_URL} "$TEMP\autopano-sift-c.7z"
  
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    MessageBox MB_OK "$(TEXT_ERROR_DownloadFailed): Autopano-SIFT-C ${AUTOPANO_SIFTC_VERSION}" 
    Goto end

  success:
      DetailPrint "Autopanp-SIFT-C ${AUTOPANO_SIFTC_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Autopano-SIFT-C ${AUTOPANO_SIFTC_VERSION}"
      ; File /nonfatal "autopano.zip"
      Nsis7z::Extract "$TEMP\autopano-sift-c.7z"
      
      Delete "$TEMP\autopano-sift-c.7z"
      
      StrCpy $R0 1 ; R0 = Type
      StrCpy $R1 1 ; R1 = Option
      StrCpy $R2 "$INSTDIR\CPG\Autopano_SIFTC\autopano-sift-c.exe" ; R2 = Program
      StrCpy $R3 "--maxmatches %p %o %i" ; R3 = Arguments
      StrCpy $R4 "Autopano-SIFT-C ${AUTOPANO_SIFTC_VERSION}";  R4 = Description
      Call ControlPointRegistryAdd

  end:
SectionEnd
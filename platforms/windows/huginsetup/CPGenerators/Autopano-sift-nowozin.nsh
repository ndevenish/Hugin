; Autopanp-SIFT (Nowozin build: http://user.cs.tu-berlin.de/~nowozin/autopano-sift/ )
!define AUTOPANO_NOWOZIN_VERSION "2.4 (S. Nowozin)"
!define AUTOPANO_NOWOZIN_SIZE 1000
!define AUTOPANO_NOWOZIN_DL_URL "http://dl.dropbox.com/u/2070182/hugin/autopano-sift-2.4.zip"

Section /o "Autopano-SIFT ${AUTOPANO_NOWOZIN_VERSION}" SecAutopano_Nowozin
  ; SectionIn RO
  AddSize ${AUTOPANO_NOWOZIN_SIZE}
  SetOutPath "$INSTDIR\bin"
  
  DetailPrint "$(TEXT_CPDownloading) Autopano-SIFT ${AUTOPANO_NOWOZIN_VERSION}"
  
  Dialer::AttemptConnect
  NSISdl::download /TIMEOUT=30000 ${AUTOPANO_NOWOZIN_DL_URL} "$TEMP\autopano-sift-2.4.zip"
  
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    MessageBox MB_OK "$(TEXT_ERROR_DownloadFailed): Autopano-SIFT ${AUTOPANO_NOWOZIN_VERSION}" 
    Goto end

  success:
      DetailPrint "Autopanp-SIFT ${AUTOPANO_NOWOZIN_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Autopano-SIFT ${AUTOPANO_NOWOZIN_VERSION}"
      ; File /nonfatal "autopano.zip"
       nsisunz::UnzipToLog /noextractpath /file "autopano-sift-2.4/bin/autopano.exe" "$TEMP\autopano-sift-2.4.zip" "$INSTDIR\bin"
       nsisunz::UnzipToLog /noextractpath /file "autopano-sift-2.4/bin/libsift.dll" "$TEMP\autopano-sift-2.4.zip" "$INSTDIR\bin"
       nsisunz::UnzipToLog /noextractpath /file "autopano-sift-2.4/bin/generatekeys-sd.exe" "$TEMP\autopano-sift-2.4.zip" "$INSTDIR\bin\"
      
      Delete "$TEMP\autopano-sift-2.4.zip"
      
      StrCpy $R0 1 ; R0 = Type
      StrCpy $R1 1 ; R1 = Option
      StrCpy $R2 "$INSTDIR\bin\autopano.exe" ; R2 = Program
      StrCpy $R3 "--maxmatches %p --projection %f,%v %o %i" ; R3 = Arguments
      StrCpy $R4 "Autopano-SIFT ${AUTOPANO_NOWOZIN_VERSION}";  R4 = Description
      Call ControlPointRegistryAdd

  end:
SectionEnd
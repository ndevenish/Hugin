; Match-and-shift 
!define MATCHNSHIFT_VERSION "0.13 (2008-02-19)"
!define MATCHNSHIFT_SIZE 1400
; !define MATCHNSHIFT_DL_URL "http://bugbear.blackfish.org.uk/~bruno/misc/Panotools-Script/match-n-shift-win32.2008-02-15.zip" ; (http://db.tt/MANEHq5)
; !define MATCHNSHIFT_DL_URL "http://bugbear.blackfish.org.uk/~bruno/misc/Panotools-Script/match-n-shift-win32.2008-02-19.zip" ; (http://db.tt/Qy3ri88)
!define MATCHNSHIFT_DL_URL "http://db.tt/Qy3ri88"
Section "Match-n-Shift ${MATCHNSHIFT_VERSION}" SecMatchNShift
  AddSize ${MATCHNSHIFT_SIZE}
  SetOutPath "$INSTDIR\bin"
  
  DetailPrint "$(TEXT_CPDownloading) Match-n-Shift ${MATCHNSHIFT_VERSION}"
  ; SetDetailsPrint listonly
  
  ; SetDetailsPrint both
  Dialer::AttemptConnect
  NSISdl::download /TIMEOUT=30000 ${MATCHNSHIFT_DL_URL} "$TEMP\match-n-shift.zip"
  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    Abort
  success:
      DetailPrint "Match-n-Shift ${MATCHNSHIFT_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Match-n-Shift ${MATCHNSHIFT_VERSION}"
      nsisunz::UnzipToLog /file "match-n-shift.exe" "$TEMP\match-n-shift.zip" "$INSTDIR\bin"
      nsisunz::UnzipToLog /file "perl58.dll" "$TEMP\match-n-shift.zip" "$INSTDIR\bin"
      Delete "$TEMP\match-n-shift.zip"
  
      StrCpy $R0 1 ; R0 = Type
      StrCpy $R1 1 ; R1 = Option
      StrCpy $R2 "$INSTDIR\bin\match-n-shift.exe" ; R2 = Program
      StrCpy $R3 "-b -a -f %f -v %v -c -p %p -o %o %i" ; R3 = Arguments
      StrCpy $R4 "Match-n-Shift ${MATCHNSHIFT_VERSION}";  R4 = Description
      Call ControlPointRegistryAdd
SectionEnd

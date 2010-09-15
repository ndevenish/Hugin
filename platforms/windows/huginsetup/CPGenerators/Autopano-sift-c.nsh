; Autopano-sift-c
!define AUTOPANOSIFTC_VERSION "2.5.2"
!define AUTOPANOSIFTC_SIZE 3000
Section "Autopano-SIFT-C ${AUTOPANOSIFTC_VERSION}" SecAutopanoSIFTC
  ; SectionIn RO
  AddSize ${AUTOPANOSIFTC_SIZE}
  
  StrCpy $R0 1 ; R0 = Type
  StrCpy $R1 1 ; R1 = Option
  StrCpy $R2 "$INSTDIR\bin\autopano-sift-c.exe" ; R2 = Program
  StrCpy $R3 "--maxmatches %p --projection %f,%v %o %i" ; R3 = Arguments
  StrCpy $R4 "Autopano-SIFT-C ${AUTOPANOSIFTC_VERSION}";  R4 = Description
  Call ControlPointRegistryAdd
SectionEnd
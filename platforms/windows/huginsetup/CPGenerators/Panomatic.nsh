; Panomatic CP Generator
!define PANOMATIC_VERSION "0.9.4"
!define PANOMATIC_SIZE 800

; Panomatic download and install
Section /o "Panomatic ${PANOMATIC_VERSION}" SecPanomatic
  AddSize ${PANOMATIC_SIZE}  ; Panomatic size
  SetOutPath "$INSTDIR\CPG\Panomatic"
  
  DetailPrint "$(TEXT_CPDownloading) Panomatic ${PANOMATIC_VERSION}"
  ; SetDetailsPrint listonly

	; Downloads panomatic from a max of 10 mirrors
	Dialer::AttemptConnect
	
	Push "http://netcologne.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://softlayer.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://cdnetworks-us-1.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://voxel.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://easynews.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://umn.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://kent.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://surfnet.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://heanet.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push "http://switch.dl.sourceforge.net/project/panomatic/panomatic/${PANOMATIC_VERSION}/panomatic-${PANOMATIC_VERSION}-win32.zip"
	Push 10
	Push "$TEMP\panomatic.zip"
	Call DownloadFromRandomMirror

  Pop $0
  StrCmp $0 success success
    SetDetailsView show
    DetailPrint $(TEXT_ERROR_DownloadFailed)
    MessageBox MB_OK "$(TEXT_ERROR_DownloadFailed): Panomatic ${PANOMATIC_VERSION}"
    Goto end
    
  success:
      DetailPrint "Panomatic ${PANOMATIC_VERSION} Downloaded!"
      DetailPrint "$(TEXT_CPExtracting) Panomatic ${PANOMATIC_VERSION}"
      nsisunz::UnzipToLog "$TEMP\panomatic.zip" "$INSTDIR\CPG\Panomatic"
      Delete "$TEMP\panomatic.zip"
      
      StrCpy $R0 1 ; R0 = Type
      StrCpy $R1 1 ; R1 = Option
      StrCpy $R2 "$INSTDIR\CPG\Panomatic\panomatic.exe" ; R2 = Program
      StrCpy $R3 "-o %o %i" ; R3 = Arguments
      StrCpy $R4 "Panomatic ${PANOMATIC_VERSION}";  R4 = Description
      Call ControlPointRegistryAdd
  
  end:
SectionEnd
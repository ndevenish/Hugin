@echo off
rem ************* No user editable area!
        rem This batch file uses enfuse_droplet.bat to enfuse images 
        rem grouped by bracketed series.
        rem If you need to adjust parameters, please do so in enfuse_droplet.bat 
echo enfuse found bracketed series droplet batch file version 0.4.0
echo copyright (c) 2008 Erik Krause - http://www.erik-krause.de
echo licensed under GPL v2
rem ************* Check working environment
        rem we need command extensions enabled
verify OTHER 2>nul
setlocal ENABLEEXTENSIONS
if ERRORLEVEL 1 goto :No_Extensions_Error
        rem ExifTool is mandatory here
if not exist exiftool.exe (
  echo ExifTool not found - sorry, we need this to find bracketed series!
  goto :ready
)  
        rem if called directly (no command line arguments) display some usage hints
if "%~1" == "" goto :print_help
echo %1|findstr ! >nul
if not errorlevel 1 goto :illegal_character
rem ************* Check command line
        rem check if command line argument is a folder 
pushd "%~1" 2>nul       
popd
if errorlevel 1 (
  echo Error:
  echo Sorry, this seems to be no folder!
  echo.
  goto :print_usage
)  
echo call without parameters for usage hints.
echo.
call :checkFiles "%~1" JPG
call :checkFiles "%~1" TIF
goto :ende
rem ************* Subroutines
:checkFiles
  if not exist "%~1\*.%2" (
    echo no %2s found in "%~1\"...
    goto :eof
  )  
  echo looking for %2s in "%~1\"...
  set FirstEV=-1000
  set enfuseFiles=
  set enfuseFileNames=
  for /F "usebackq delims=" %%I in (`DIR /B /O:N "%~1\*.%2"`) do (
    echo skip '%%I' | findstr _enfused 
    if errorlevel 1 call :checkEXIF "%~1\%%I" '%%I'
  )  
  call $$$_enfuse_temp_$$$.bat
  del $$$_enfuse_temp_$$$.bat
goto :eof
:checkEXIF
  echo EXIFTool working on %2
        rem let ExifTool deliver EV, display and enfuse if equal to first file
  for /F "usebackq" %%j IN (`exiftool -m -fast -p $LightValue %1`) DO if %%j==%FirstEV% (
    call $$$_enfuse_temp_$$$.bat
    del $$$_enfuse_temp_$$$.bat
    set enfuseFiles=%1
    set enfuseFileNames=%2
  ) else (
    if %FirstEV%==-1000 set FirstEV=%%j
    echo echo enfuse %enfuseFileNames% %2>$$$_enfuse_temp_$$$.bat
    echo call enfuse_droplet.bat %enfuseFiles% %1>>$$$_enfuse_temp_$$$.bat
    set enfuseFileNames=%enfuseFileNames% %2
    set enfuseFiles=%enfuseFiles% %1
  )
goto :eof
:No_Extensions_Error
echo.
echo Unable to enable command extensions
echo.
echo either you have a very old command interpreter 
echo or you use a too old windows version
echo Sorry, this won't work on your machine...
echo.
pause
goto :ready
:print_help
echo.
echo Usage:
echo.
echo Create a shortcut for this batch file on the desktop or in any folder.
echo and give it a speaking name (if you clicked this from the desktop, 
echo there most likely is a shortcut already)
echo The batchfile itself must reside in the same folder as enfuse.exe
echo and can not be used directly.
:print_usage
echo.
echo How to use this droplet:
echo    drag and drop a folder on the droplet. The folder should contain 
echo    only bracketed series of images, each series starting at the same 
echo    EV. The droplet will enfuse each series. 
echo    The result will be .tif files named like the first image of each 
echo    series plus _enfused added to the file name.
echo.
echo Have fun
echo Erik Krause
echo. 
:ready
pause
:ende

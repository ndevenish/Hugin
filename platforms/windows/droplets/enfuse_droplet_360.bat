@echo off
        rem Specify additional parameters you want to pass to enfuse in the next line 
        rem (must be one line)
set enfuse_additional_parameters= -w --wExposure=1 --wSaturation=1 --wContrast=1
echo enfuse droplet batch file version 0.2.1
echo copyright 2008 Erik Krause - http://www.erik-krause.de
        rem we need command extensions enabled
verify OTHER 2>nul
setlocal ENABLEEXTENSIONS
if ERRORLEVEL 1 goto :No_Extensions_Error
if not exist enfuse.exe goto :No_enfuse_Error
        rem if called directly (no command line arguments) display some usage hints
if "%~1" == "" goto :print_help
echo call without parameters for usage hints.
        rem check if command line argument is a folder 
pushd "%~1" 2>nul       
popd
if not errorlevel 1 goto :IsDir
        rem apparently these are files: fusion them.
enfuse.exe %enfuse_additional_parameters% -o "%~dpn1_fusioned.tif" %*
        rem that's all
goto :ready
        rem we have a folder
:IsDir
        rem we need delayed environment variable expansion in order to count files
setlocal ENABLEDELAYEDEXPANSION
echo.        
        rem Inform user ...
echo Processed directory: %1
echo. 
echo Please specify number of files in one bracketed series 
echo or "A" for all files in the directory, then press Enter
        rem and ask what to do
set /P enfuse_number_of_files=Your input: 
        rem use set enfuse_number_of_files=3 to set the number fixed to 3 f.e.
        rem init variables
set enfuse_files=
set CNT=0
        rem init temporary batch file. 
        rem we need this in order to avoid that the created files are included as well
echo @echo off > $$$_enfuse_temp_$$$.bat
        rem loop over all jpg and tif files in the respective folder 
for %%I in ("%~1\*.jpg" "%~1\*.tif") do call :loop "%%I"
        rem skip subroutine
goto skip_loop
        rem a subroutine
:loop
        rem increment count
  set /A CNT+=1
        rem remember the first file name of each series  
  if !CNT! EQU 1 set enfuse_first_file=%~dpn1
        rem as long as number not reached...
  if !CNT! LSS %enfuse_number_of_files% ( 
        rem  add files to the command line    
    set enfuse_files=!enfuse_files! "%~1"
  ) else (
        rem if number reached write the result to the temp batch file. 
        rem remember to add current file!
    echo enfuse.exe %enfuse_additional_parameters% -o "%enfuse_first_file%_fusioned.tif" %enfuse_files% "%~1">> $$$_enfuse_temp_$$$.bat 
        rem init variables
    set enfuse_files=
    set CNT=0
  )
goto :eof
        rem end of subroutine  
:skip_loop
        rem if there are still files left, add them as well
if !CNT! NEQ 0 echo enfuse.exe %enfuse_additional_parameters% -o "%enfuse_first_file%_fusioned.tif" %enfuse_files%>> $$$_enfuse_temp_$$$.bat
        rem call temporary batch file to execute enfuse
call $$$_enfuse_temp_$$$.bat
        rem this file stays in the enfuse folder and is overwritten the next time
endlocal
        rem that's it
goto :ready
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
:No_enfuse_Error
echo.
echo Unable to find enfuse.exe
echo.
echo Make sure this batch file is in the same folder as enfuse.exe
echo.
pause
goto :ready
:print_help
echo.
echo Usage:
echo.
echo Create a shortcut for this batch file on the desktop or in any folder.
echo The batchfile itself must reside in the same folder as enfuse.exe
echo.
echo There are two ways to use this droplet:
echo 1. drag and drop some images (a bracketed series) on the droplet to
echo    fusion them. 
echo    The result will be a .tif file named like one image of the series 
echo    plus _fusioned added to the file name.
echo 2. drag and drop a folder on the droplet. You will be asked to
echo    specify how many images are in a bracketed series or whether 
echo    you want to fusion all images in the folder.
echo    The result will be .tif files named like the first image of each 
echo    series plus _fusioned added to the file name.
echo.
echo Have fun
echo Erik Krause
echo. 
pause
:ready
endlocal
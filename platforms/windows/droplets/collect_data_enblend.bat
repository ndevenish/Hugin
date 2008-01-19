@echo off
if "%~1" == "" (
  echo.
  echo helper batch file for the enblend droplets
  echo.
  echo not to be used stand alone
  echo.
  pause
  goto :eof
)
set exiftool_LighValues=
set exiftool_ImageNames=
:next_one
set exiftool_ImageNames=%exiftool_ImageNames% '%~nx1'
rem for /F "usebackq" %%i IN (`exiftool -m -fast -p $LightValue %1`) DO set exiftool_LighValues=%exiftool_LighValues% %%i 
shift
if "%~1"=="" goto :ready
goto next_one
:ready
echo created by enblend from images %exiftool_ImageNames% 

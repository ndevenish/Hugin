@echo off
if "%~1" == "" (
  echo.
  echo helper batch file for the enfuse and enblend droplets
  echo.
  echo not to be used stand alone
  echo.
  pause
  goto :eof
)
set TempResultName=%~1
set CNT=0
:loop
if not exist "%TempResultName%" goto :ready
set /A CNT+=1 > nul
set TempResultName=%~dpn1_%CNT%.tif
goto :loop
:ready
echo %TempResultName%
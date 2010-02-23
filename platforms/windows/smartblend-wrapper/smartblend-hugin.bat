@echo off & setlocal

set SMARTBLEND=%~dp0\smartblend.exe
set SMARTBLENDARGS=

rem Hugin sets -w, -o and --compression automatically. Smartblend does not
rem understand --compression (and its parameter), so we have to drop those.
rem The -w argument is only partly understood as a "compatibility" feature
rem for Enblend users (see the readme.txt file which came with Smartblend).
:paramstrip
set arg=%1
if not "%arg%"=="" (
	if "%arg%"=="--compression" (
		rem Skip compression parameter and its argument
		shift
	) else if "%arg:~0,2%"=="-o" (
		rem Set output parameter
		set SMARTBLENDARGS=%SMARTBLENDARGS% -o %2
		shift
	) else (
		rem Copy other parameters
		set SMARTBLENDARGS=%SMARTBLENDARGS% %1
	)
	shift
	goto :paramstrip
)

echo.
echo Executing smartblend.exe %SMARTBLENDARGS%
echo.

"%SMARTBLEND%" %SMARTBLENDARGS%

endlocal
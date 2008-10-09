@echo off & setlocal

set SMARTBLEND=%~dp0\smartblend.exe
set SMARTBLENDARGS=

:paramstrip
set arg=%1
if not "%arg%"=="" (
	if "%arg%"=="--compression" (
		rem Skip compression parameter and its argument
		shift
	) else if "%arg:~0,2%"=="-f" (
		rem Skip ...
	) else if "%arg:~0,2%"=="-l" (
		rem Skip ...
	) else if "%arg:~0,2%"=="-o" (
		rem Reformat output parameter: add "smartblend-" prefix
		set SMARTBLENDARGS=%SMARTBLENDARGS% -o smartblend-%2
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

%SMARTBLEND% %SMARTBLENDARGS%

endlocal
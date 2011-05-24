@echo off & setlocal

set SMARTBLEND=%~dp0\smartblend.exe
set SMARTBLENDARGS=

rem This is where we strip out arguments which Smartblend does not understand.
rem Hugin automatically sets a few (-w, -o, --compression, -f) but out of
rem these, Smartbland can only handle -o and -w (the latter only partly).
rem See the readme.txt bundled with Smartblend for details.
:paramstrip
rem Get argument without surrounding quotes
set arg=%~1
if not "%arg%"=="" (
	if "%arg%"=="--compression" (
		echo [smartblend-wrapper] Skipping compression argument and its parameter: %1 %2
		shift
	) else if "%arg:~0,2%"=="-f" (
		echo [smartblend-wrapper] Skipping crop argument: %1
	) else if "%arg:~0,2%"=="--" (
		echo [smartblend-wrapper] Skipping argument separator: %1
	) else if "%arg:~0,2%"=="-o" (
		echo [smartblend-wrapper] Output file: %2
		set SMARTBLENDARGS=%SMARTBLENDARGS% -o %2
		shift
	) else (
		rem Copy other arguments (including any surrounding quotes)
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

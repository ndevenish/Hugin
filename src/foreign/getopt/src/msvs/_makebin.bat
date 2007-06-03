@echo off
if not "%VSINSTALLDIR%"=="" goto msvs7
if not "%MSDevDir%"=="" goto msvs6
echo Neither MSVS6 nor MSVS7(1) was found.
goto end
:msvs6
msdev getopt.dsw /MAKE "getopt - Win32 Release"
msdev getopt.dsw /MAKE "getopt - Win32 Debug"
goto end
:msvs7
devenv getopt.sln /build Release /project getopt
devenv getopt.sln /build Debug /project getopt
:end

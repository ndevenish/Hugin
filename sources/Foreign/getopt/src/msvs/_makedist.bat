@echo off
if not "%1%"=="" goto begin
call _makedist 6
call _makedist 71
goto end
:begin
if exist dist%1 rd /q/s dist%1
md dist%1
md dist%1\include
md dist%1\lib
copy Release%1\getopt.lib dist%1\lib
copy Debug%1\getopt_debug.lib dist%1\lib
copy ..\..\include\getopt.h dist%1\include
copy ..\..\LICENSE dist%1
copy README.THIS dist%1
cd dist%1
if exist ..\getopt-msvs%1.zip del ..\getopt-msvs%1.zip
zip -q -r -9 -S -! ..\getopt-msvs%1.zip *
cd ..
:end

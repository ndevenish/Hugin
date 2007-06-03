@echo off
if not "%1%"=="" goto begin
call _makeclean 6
call _makeclean 71
goto end
:begin
rd /q/s dist%1
attrib -h *.suo
del /f/q *.ncb *.suo *.opt *.plg *.positions
rd /q/s Release%1
rd /q/s Debug%1
del /f/q *.plg
:end

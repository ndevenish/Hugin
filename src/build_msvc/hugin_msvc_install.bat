
rem create installation directory

set DESTDIR=L:\daten\hugin\vc\hugin_install
set SRCDIR=L:\daten\hugin\vc\hugin\src

rmdir /s /q %DESTDIR%
mkdir %DESTDIR%

rem
rem copy files for hugin
rem

copy %SRCDIR%\"hugin\Release Unicode\hugin.exe" %DESTDIR%\

rem
rem copy files for nona
rem

copy %SRCDIR%\"nona_gui\Release Unicode\nona_gui.exe" %DESTDIR%\

copy_common %SRCDIR% %DESTDIR%



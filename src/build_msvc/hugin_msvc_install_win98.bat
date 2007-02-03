
rem create installation directory

set DESTDIR=L:\daten\hugin\vc\hugin_install_win98
set SRCDIR=L:\daten\hugin\vc\hugin\src

rmdir /s /q %DESTDIR%

mkdir %DESTDIR%

rem
rem copy files for hugin
rem

copy %SRCDIR%\"hugin\Release\hugin.exe" %DESTDIR%\

rem
rem copy files for nona
rem

copy %SRCDIR%\"nona_gui\Release\nona_gui.exe" %DESTDIR%\

copy_common %SRCDIR% %DESTDIR%



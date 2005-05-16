
rem create installation directory

set DESTDIR=D:\hugin\vc\hugin_install
set SRCDIR=D:\hugin\vc\hugin\src

set LINGUAS=de fr pl it
set NLINGUAS=de pl fr

set MSGFMT=C:\Programme\poEdit\bin\msgfmt.exe

rmdir /s /q %DESTDIR%

mkdir %DESTDIR%
mkdir %DESTDIR%\xrc
mkdir %DESTDIR%\xrc\data
mkdir %DESTDIR%\locale

for %%l in ( %LINGUAS% ) do mkdir %DESTDIR%\locale\%%l

rem copy panoglview
copy %SRCDIR%\..\..\panoglview\panoglview.exe %DESTDIR%\

rem copy installer script
copy %SRCDIR%\..\utils\hugin.nsi %DESTDIR%\


rem copy licence and other text documents
copy %SRCDIR%\..\LICENCE %DESTDIR%\LICENCE.txt
copy %SRCDIR%\foreign\VIGRA_LICENSE %DESTDIR%\VIGRA_LICENSE.txt
copy %SRCDIR%\..\README_WINDOWS %DESTDIR%\README_WINDOWS.txt
copy %SRCDIR%\..\AUTHORS %DESTDIR%\AUTHORS.txt


rem
rem copy files for hugin
rem

copy %SRCDIR%\"hugin\Release Unicode\hugin.exe" %DESTDIR%\
xcopy %SRCDIR%\hugin\xrc %DESTDIR%\xrc\
del %DESTDIR%\xrc\.cvsignore

xcopy %SRCDIR%\hugin\xrc\data %DESTDIR%\xrc\data
del %DESTDIR%\xrc\data\.cvsignore

rem language files for hugin. They need to be rebuild by hand..
for %%l in ( %LINGUAS% ) do %MSGFMT% -c -o %DESTDIR%\locale\%%l\hugin.mo %SRCDIR%\hugin\po\%%l.po 

rem
rem copy files for nona
rem

copy %SRCDIR%\"nona_gui\Release Unicode\nona_gui.exe" %DESTDIR%\
copy %SRCDIR%\tools\Release\nona.exe %DESTDIR%\

rem language files for nona.
for %%l in ( %NLINGUAS% ) do %MSGFMT% -c -o %DESTDIR%\locale\%%l\nona_gui.mo %SRCDIR%\nona_gui\po\%%l.po 


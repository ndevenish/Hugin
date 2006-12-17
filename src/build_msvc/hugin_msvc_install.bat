
rem create installation directory

set DESTDIR=L:\daten\hugin\vc\hugin_install
set SRCDIR=L:\daten\hugin\vc\hugin\src

set LINGUAS=ca_ES cs_CZ de fr hu it ja nl pl pt_BR ru uk zh_CN
set NLINGUAS=de pl fr nl pt_BR zh_CN hu ru

set MSGFMT=C:\Programme\poEdit\bin\msgfmt.exe

rmdir /s /q %DESTDIR%

mkdir %DESTDIR%
mkdir %DESTDIR%\doc
mkdir %DESTDIR%\xrc
mkdir %DESTDIR%\xrc\data
mkdir %DESTDIR%\locale

for %%l in ( %LINGUAS% ) do mkdir %DESTDIR%\locale\%%l

rem copy panoglview 
copy %SRCDIR%\..\..\panoglview\panoglview.exe %DESTDIR%\

rem copy doc
copy %SRCDIR%\..\doc\nona.txt %DESTDIR%\doc
copy %SRCDIR%\..\doc\fulla.html %DESTDIR%\doc

rem copy panotools
rem copy "%SRCDIR%\..\..\libs\libpano\pano12\Release\pano12.dll" %DESTDIR%\
copy "%SRCDIR%\..\..\libs\libpano\pano13\tools\Release\*.exe" %DESTDIR%\
rem copy %SRCDIR%\..\..\panotools\pano12_for_usage_with_ptstitcher.dll %DESTDIR%\


rem copy enblend
copy %SRCDIR%\..\..\enblend\enblend.exe %DESTDIR%\
copy %SRCDIR%\..\..\enblend\AUTHORS %DESTDIR%\ENBLEND_AUTHORS.txt
copy %SRCDIR%\..\..\enblend\README %DESTDIR%\ENBLEND_README.txt
copy %SRCDIR%\..\..\enblend\NEWS %DESTDIR%\ENBLEND_NEWS.txt


rem copy installer script
copy %SRCDIR%\..\utils\hugin.nsi %DESTDIR%\


rem copy licence and other text documents
copy %SRCDIR%\..\LICENCE %DESTDIR%\LICENCE.txt
copy %SRCDIR%\..\LICENCE_VIGRA %DESTDIR%\LICENCE_VIGRA.txt
copy %SRCDIR%\..\LICENCE_JHEAD %DESTDIR%\LICENCE_JHEAD.txt
copy %SRCDIR%\..\README_WINDOWS %DESTDIR%\README_WINDOWS.txt
copy %SRCDIR%\..\AUTHORS %DESTDIR%\AUTHORS.txt
copy %SRCDIR%\..\NEWS %DESTDIR%\NEWS.txt

rem copy fulla
copy %SRCDIR%\"tools\Release\fulla.exe" %DESTDIR%\

rem
rem copy files for hugin
rem

copy %SRCDIR%\"hugin\Release Unicode\hugin.exe" %DESTDIR%\
xcopy /s %SRCDIR%\hugin\xrc %DESTDIR%\xrc\
del %DESTDIR%\xrc\.cvsignore
del %DESTDIR%\xrc\data\.cvsignore
del %DESTDIR%\xrc\data\help_en_EN\.cvsignore
del %DESTDIR%\xrc\data\help_fr_FR\.cvsignore

rem language files for hugin. They need to be rebuild by hand..
for %%l in ( %LINGUAS% ) do %MSGFMT% -c -o %DESTDIR%\locale\%%l\hugin.mo %SRCDIR%\hugin\po\%%l.po 



rem
rem copy files for nona
rem

copy %SRCDIR%\"nona_gui\Release Unicode\nona_gui.exe" %DESTDIR%\
copy %SRCDIR%\tools\Release\nona.exe %DESTDIR%\

rem language files for nona.
for %%l in ( %NLINGUAS% ) do %MSGFMT% -c -o %DESTDIR%\locale\%%l\nona_gui.mo %SRCDIR%\nona_gui\po\%%l.po 


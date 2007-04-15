
rem create installation directory

set SRCDIR=%1
set DESTDIR=%2
echo %SRCDIR%
echo %DESTDIR%

set LINGUAS=de pl fr it ja ru pt_BR nl zh_CN hu ca_ES cs_CZ uk sv sk es
set NLINGUAS=de pl fr nl pt_BR zh_CN hu ru sk

set MSGFMT=C:\Programme\poEdit\bin\msgfmt.exe

mkdir %DESTDIR%\doc
mkdir %DESTDIR%\xrc
mkdir %DESTDIR%\xrc\data
mkdir %DESTDIR%\locale
mkdir %DESTDIR%\panotools
mkdir %DESTDIR%\panotools\doc
mkdir %DESTDIR%\enblend

for %%l in ( %LINGUAS% ) do mkdir %DESTDIR%\locale\%%l

rem copy panoglview 
copy %SRCDIR%\..\..\panoglview\panoglview.exe %DESTDIR%\

rem copy doc
type %SRCDIR%\..\doc\nona.txt | find /V "" > %DESTDIR%\doc\nona.txt
type %SRCDIR%\..\doc\fulla.html | find /V "" > %DESTDIR%\doc\fulla.html
copy %SRCDIR%\..\doc\nona.txt %DESTDIR%\doc
copy %SRCDIR%\..\doc\fulla.html %DESTDIR%\doc

rem copy panotools
copy "%SRCDIR%\..\..\libs\libpano\pano13\tools\Release\*.exe" %DESTDIR%\Panotools

set txtfiles=AUTHORS ChangeLog README TODO doc\PTblender.readme doc\PTmender.readme doc\Optimize.txt doc\stitch.txt
for %%n in ( %txtfiles% ) do type "%SRCDIR%\..\..\libs\libpano\pano13\%%n" | find /V "" > "%DESTDIR%\Panotools\%%n.txt"


rem copy enblend
xcopy /s %SRCDIR%\..\..\enblend %DESTDIR%\enblend\

rem copy installer script
copy %SRCDIR%\..\utils\hugin.nsi %DESTDIR%\


rem copy licence and other text documents
set txtfiles=LICENCE LICENCE_VIGRA LICENCE_JHEAD README_WINDOWS AUTHORS NEWS
for %%n in ( %txtfiles% ) do type "%SRCDIR%\..\%%n" | find /V "" > "%DESTDIR%\%%n.txt"

rem copy fulla
copy %SRCDIR%\"tools\Release\fulla.exe" %DESTDIR%\

rem copy autooptimiser
copy %SRCDIR%\"tools\Release\autooptimiser.exe" %DESTDIR%\

rem copy vig_optimize
copy %SRCDIR%\"tools\Release\vig_optimize.exe" %DESTDIR%\


rem
rem copy files for hugin
rem

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
copy %SRCDIR%\tools\Release\nona.exe %DESTDIR%

rem language files for nona.
for %%l in ( %NLINGUAS% ) do %MSGFMT% -c -o %DESTDIR%\locale\%%l\nona_gui.mo %SRCDIR%\nona_gui\po\%%l.po 


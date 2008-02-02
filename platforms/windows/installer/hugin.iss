; Hugin InnoSetup Installer File
; if possible, let the Make process edit AppVerName to have a proper, automated SVN numbering
; make sure that the Make process copies this file from platforms/windows/msi to INSTALL/
; and that it also copes the files win_installer_readme.txt and win_release_notes.txt to INSTALL/
; prior to running it with the Inno Setup Compiler 5.2.2

[Setup]
AppName=Hugin
; ApId is checked to determine wheter to append to a particular existing uninstall log
; keep it the same for updates of the same installation
AppId=Hugin_alpha_builds
AppVerName=Hugin 0.7.0.@HUGIN_WC_REVISION@SVN-Yuv
AppPublisher=Yuv
AppPublisherURL=http://panospace.wordpress.com/
AppVersion=0.7.0.@HUGIN_WC_REVISION@SVN-Yuv
AppSupportURL=http://tech.groups.yahoo.com/group/PanoToolsNG/
AppUpdatesURL=http://groups.google.com/group/hugin-ptx
; necessary to create file associations
ChangesAssociations=yes
; necessary to edit system path
ChangesEnvironment=yes
; should default to yes, but reports indicate that the entry was not created by the previous installer
CreateUninstallRegKey=yes
; default yes, but for updaters better no. play with it
UpdateUninstallLogAppName=yes
DefaultDirName={pf}\Hugin
DefaultGroupName=Hugin
; AllowNoIcons does not affect entries in the Tasks section that have their own checkboxes
AllowNoIcons=yes
; show list of types only to reduce complexity unless customizing
AlwaysShowComponentsList=no
LicenseFile=installer_license.txt
PrivilegesRequired=poweruser
UsePreviousAppDir=yes
; HOOK to run a VBS (theoretically to check if an app is running: AppMutex. See Documentation if you want to use it
; Uninstall stuff made nicer
UninstallDisplayIcon={app}\bin\hugin.exe,0
; cosmetic stuff
AppCopyright=Copyright (C) 2004-2008 Pablo d'Angelo
FlatComponentsList=No
; might not work as the example was with a file.ico
SetupIconFile=hugin.ico
WizardSmallImageFile=smallimage.bmp
WizardImageStretch=no
WizardImageFile=wizardimage.bmp


[Types]
Name: "default";           Description: "Default installation (w/o Autopano)"
Name: "full";              Description: "Full installation"
Name: "enblend";           Description: "Enblend/Enfuse and helper files/droplets only"
Name: "photoshop";         Description: "Photoshop Plugins only (not available yet)"
Name: "gimp";              Description: "Gimp Plugins only (not available yet)"
Name: "custom";            Description: "Custom installation (recommended for testing)"; Flags: iscustom

[Components]
Name: "core";              Description: "Hugin";                                        Types: default full custom; Flags: fixed
Name: "translations";      Description: "Hugin Language Pack";                          Types: default full custom;
Name: "enblend";           Description: "Enblend/Enfuse";                               Types: default full enblend custom;
Name: "autopano";          Description: "Autopano-SIFT-C (Patent issues in the USA!)";  Types: full custom;
Name: "panotools";         Description: "Panotools CLI Tools (N/A yet)";                Types: default full custom;
Name: "photoshop";         Description: "Photoshop Plugins (N/A yet)";                  Types: full photoshop custom;
Name: "gimp";              Description: "Gimp Plugins (N/A yet)";                       Types: full gimp custom;

; not necessary (if the directory is not empty) but clean
[Dirs]
; bin is not read-only because we may want to customize the droplets
Name: "{app}\bin"
Name: "{app}\doc";   Attribs: readonly;
Name: "{app}\share"; Attribs: hidden readonly;

; for wildcards to apply to subfolders, set flag recursesubdirs
; initially copied only the there subfolders recursively
; but to assign files to different components I need a finer grained description
[Files]
; hugin executables
Source: "FILES\bin\align_image_stack.exe";     DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\autooptimiser.exe";         DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\basename.exe";              DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\cp.exe";                    DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\echo.exe";                  DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\exiftool.exe";              DestDir: "{app}\bin";           Components: core enblend; Flags: overwritereadonly ignoreversion
Source: "FILES\bin\fulla.exe";                 DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\hugin.exe";                 DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\hugin_hdrmerge.exe";        DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\hugin_stitch_project.exe";  DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\make.exe";                  DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\nona.exe";                  DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\nona_gui.exe";              DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\rm.exe";                    DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\sh.exe";                    DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\uname.exe";                 DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
Source: "FILES\bin\vig_optimize.exe";          DestDir: "{app}\bin";           Components: core;         Flags: overwritereadonly ignoreversion
; autopano-sift-c executables
Source: "FILES\bin\autopano.exe";              DestDir: "{app}\bin";           Components: autopano;     Flags: overwritereadonly ignoreversion
Source: "FILES\bin\autopano-c-complete.vbs";   DestDir: "{app}\bin";           Components: autopano;     Flags: overwritereadonly ignoreversion
Source: "FILES\bin\generatekeys.exe";          DestDir: "{app}\bin";           Components: autopano;     Flags: overwritereadonly ignoreversion
; enblend/enfuse executables
Source: "FILES\bin\collect_data_enblend.bat";  DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\collect_data_enfuse.bat";   DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\enblend.exe";               DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\enblend_droplet.bat";       DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\enblend_droplet_360.bat";   DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\enfuse.exe";                DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\enfuse_align_droplet.bat";  DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\enfuse_droplet.bat";        DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\enfuse_droplet_360.bat";    DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\exiftool_enblend_args.txt"; DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\exiftool_enfuse_args.txt";  DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
Source: "FILES\bin\unique_filename.bat";       DestDir: "{app}\bin";           Components: enblend;      Flags: overwritereadonly ignoreversion
; panotools executables
Source: "FILES\bin\PTblender.exe";             DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTcrop.exe";                DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTinfo.exe";                DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTmasker.exe";              DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTmender.exe";              DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTOptimizer.exe";           DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTroller.exe";              DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTtiff2psd.exe";            DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTtiffdump.exe";            DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
Source: "FILES\bin\PTuncrop.exe";              DestDir: "{app}\bin";           Components: panotools;    Flags: overwritereadonly ignoreversion
; install counter
Source: "url.txt";                             DestDir: "{app}";               DestName: "test.url";     Flags: ignoreversion;   Attribs: hidden
; documentation
; check why fulla.html and nona.txt are in doc and not in doc/hugin
Source: "FILES\doc\enblend\*";                 DestDir: "{app}\doc\enblend";         Components: enblend;      Flags: overwritereadonly recursesubdirs ignoreversion
Source: "FILES\doc\hugin\*";                   DestDir: "{app}\doc\hugin";           Components: core;         Flags: overwritereadonly recursesubdirs ignoreversion
Source: "FILES\doc\panotools\*";               DestDir: "{app}\doc\panotools";       Components: panotools;    Flags: overwritereadonly recursesubdirs ignoreversion
Source: "FILES\doc\autopano-sift-C\*";         DestDir: "{app}\doc\autopano-sift-C"; Components: autopano;     Flags: overwritereadonly recursesubdirs ignoreversion
; hugin's UI and languages
Source: "FILES\share\hugin\*";                 DestDir: "{app}\share\hugin";   Components: core;         Attribs: hidden; Flags: overwritereadonly recursesubdirs ignoreversion
; hugin's translations
Source: "FILES\share\locale\*";                DestDir: "{app}\share";         Components: translations; Attribs: hidden; Flags: overwritereadonly recursesubdirs ignoreversion
; release notes for this installer
Source: "Release_Notes.txt";                   DestDir: "{app}\doc";           DestName: "Release Notes.txt"; Components: core;         Flags: isreadme overwritereadonly ignoreversion

[InstallDelete]
; processed as first step of installation
; remove the old release notes files if it exists
Type: files; Name: "{app}\doc\win_release_notes.txt";

[Registry]
; enable WSH - HKLM worked for me, documentation said to try HKCU too. needed for autopano-c-complete.vbs
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows Script Host\Settings"; ValueType: dword; ValueName: "Enabled"; ValueData: "1"; Flags: noerror
Root: HKCU; Subkey: "SOFTWARE\Microsoft\Windows Script Host\Settings"; ValueType: dword; ValueName: "Enabled"; ValueData: "1"; Flags: noerror
; file associations
; register extension .pto with internal name HuginProject (must be unique)
Root: HKCR; Subkey: ".pto"; ValueType: string; ValueName: ""; ValueData: "HuginProject"; Flags: uninsdeletevalue
; set Hugin Project File to be the file type as shown in Explorer
Root: HKCR; Subkey: "HuginProject"; ValueType: string; ValueName: ""; ValueData: "Hugin Project File"; Flags: uninsdeletevalue
; set the icon
Root: HKCR; Subkey: "HuginProject\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\hugin.exe,0"
; specify the command to execute on double-click
Root: HKCR; Subkey: "HuginProject\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\bin\hugin.exe"" ""%1"""
; check if it is worth it to register other App Paths
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Hugin.exe"; ValueType: string; ValueName: ""; ValueData: "{app}\bin\hugin.exe"; Flags: uninsdeletekey
; delete registry entry completely on uninstall
Root: HKCU; Subkey: "Software\hugin"; Flags: deletekey; Tasks: "delete_settings"
; could add here default preference, one registry key at a time.


; by itself a task does nothing, it needs ot be linked to other installation entries
[Tasks]

Name: "modifypath";              Description: "Add application directory to your system path";   GroupDescription: "System:";
Name: "delete_settings";         Description: "Clean Slate? (Deletes previous settings)";        GroupDescription: "System:";           Flags: unchecked;
Name: "quicklaunch";             Description: "Create Quick Launch Icon";                        GroupDescription: "Additional icons:"; Flags: unchecked
Name: "desktop";                 Description: "Create desktop icons";                            GroupDescription: "Additional icons:";
Name: "desktop\user";            Description: "only fur the current user";                       GroupDescription: "Additional icons:"; Flags: exclusive unchecked
Name: "desktop\user\hugin";      Description: "Hugin";                                           GroupDescription: "Additional icons:"; Flags: unchecked
Name: "desktop\user\enblend";    Description: "Enblend Droplets";                                GroupDescription: "Additional icons:"; Flags: unchecked
Name: "desktop\user\enfuse";     Description: "Enfuse Droplets";                                 GroupDescription: "Additional icons:"; Flags: unchecked
Name: "desktop\common";          Description: "for all users";                                   GroupDescription: "Additional icons:"; Flags: exclusive
Name: "desktop\common\hugin";    Description: "Hugin";                                           GroupDescription: "Additional icons:";
Name: "desktop\common\enblend";  Description: "Enblend Droplets";                                GroupDescription: "Additional icons:";
Name: "desktop\common\enfuse";   Description: "Enfuse Droplets";                                 GroupDescription: "Additional icons:";
Name: "programfiles";            Description: "Create additional entries in Program Files menu"; GroupDescription: "Additional icons:";

[Icons]
; desktop for current user
Name: "{userdesktop}\Hugin";          Filename: "{app}\bin\hugin.exe"; Tasks: desktop\user\hugin
; desktop for all users
Name: "{commondesktop}\Hugin";        Filename: "{app}\bin\hugin.exe"; Tasks: desktop\common\hugin
; quick-launch
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Hugin"; Filename: "{app}\bin\hugin.exe"; Tasks: quicklaunch
; Program Files Group
Name: "{group}\Hugin";                Filename: "{app}\bin\hugin.exe"
Name: "{group}\Homepage";             Filename: "http://hugin.sf.net/";                Tasks: programfiles
Name: "{group}\News";                 Filename: "http://panospace.wordpress.com/";     Tasks: programfiles
Name: "{group}\nona_gui";             Filename: "{app}\bin\nona_gui.exe";              Tasks: programfiles
Name: "{group}\Hugin_stitch_project"; Filename: "{app}\bin\hugin_stitch_project.exe";  Tasks: programfiles
Name: "{group}\uninstall";            Filename: "{uninstallexe}";                      Tasks: programfiles
Name: "{group}\Documentation";        Filename: "{app}\doc";
; Enfuse droplets for current user
Name: "{userdesktop}\Enfuse";         Filename: "{app}\bin\enfuse_droplet.bat";        WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\user\enfuse
Name: "{userdesktop}\Enfuse360";      Filename: "{app}\bin\enfuse_droplet_360.bat";    WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\user\enfuse
Name: "{userdesktop}\EnfuseAlign";    Filename: "{app}\bin\enfuse_align_droplet.bat";  WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\user\enfuse
; Enblend droplets for current user
Name: "{userdesktop}\Enblend";        Filename: "{app}\bin\enblend_droplet.bat";       WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\user\enblend
Name: "{userdesktop}\Enblend360";     Filename: "{app}\bin\enblend_droplet_360.bat";   WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\user\enblend
; Enfuse droplets for all users
Name: "{commondesktop}\Enfuse";       Filename: "{app}\bin\enfuse_droplet.bat";        WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\common\enfuse
Name: "{commondesktop}\Enfuse360";    Filename: "{app}\bin\enfuse_droplet_360.bat";    WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\common\enfuse
Name: "{commondesktop}\EnfuseAlign";  Filename: "{app}\bin\enfuse_align_droplet.bat";  WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\common\enfuse
; Enblend droplets for all users
Name: "{commondesktop}\Enblend";      Filename: "{app}\bin\enblend_droplet.bat";       WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\common\enblend
Name: "{commondesktop}\Enblend360";   Filename: "{app}\bin\enblend_droplet_360.bat";   WorkingDir: "{app}\bin"; Components: enblend; Tasks: desktop\common\enblend

[Code]
function ModPathDir(): TArrayOfString;
var
  Dir: TArrayOfString;
begin
  setArrayLength(Dir,1)
  Dir[0] := ExpandConstant('{app}') + '\bin\';
  Result := Dir;
end;
#include "modpath.iss"

[Run]
; install counter
Filename: "{app}\test.url"; Flags: shellexec
; here it would also be possible to launch the execution of post-install VBS scripts
Filename: "{app}\bin\hugin.exe"; Description: "Launch hugin"; Flags: nowait postinstall skipifsilent unchecked


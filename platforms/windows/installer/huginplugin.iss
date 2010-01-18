;Example of a plugin installer for hugin on 32 bit Windows
;replace the [insert] parts by the proper names, cut out the brackets too
;Copyright Allard Katan 2010
;This file is licenced under the GPL V2.
;Currently, all this does is install a specified executable into a specified subdirectory of hugin

[Setup]
AppName=[insert plugin name here]
;The following line defines this program as part of hugin. The AppId should be the same as that used for the hugin installation script
;Two appID's are currently defined: Hugin_release for releases and Hugin_prerelease for snapshots and betas

AppId=Hugin_release
AppVerName=[insert plugin name here, this is what the installer will tell the user it's installing after opening]
UsePreviousAppDir=no
;the following line tells the setup to look in the registry for the install path of hugin. Supported on 2009.4 and later installers of hugin. You can specify a subdirectory of the hugin root to place the plugin if you want
DefaultDirName={reg:HKLM\Software\hugin\settings,HuginInstallPath|{pf}\Hugin}\[insert subdirectory of hugin install root here]
Compression=lzma
SolidCompression=yes
CreateUninstallRegKey=no
UpdateUninstallLogAppName=no
OutputBaseFilename=[insert the name for the installer here]



[Files]
Source: [insert plugin filename/path here]; DestDir: {app}/[insert subdirectory of hugin install root here]; Flags: overwritereadonly
Source: [insert readme filename/path here]; DestDir: {app}; Flags: isreadme

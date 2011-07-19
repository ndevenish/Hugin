#!/bin/sh

# $Id: create_stripped_apps_for_installer.sh 2011 2011-06-05 HvdW $

installerfolder="$TARGET_BUILD_DIR/InstallerFolder"
HuginApp="$installerfolder/Hugin.app/Contents"
PTBatcherGuiApp="$installerfolder/PTBatcherGui.app/Contents"
lens_calibrate_guiApp="$installerfolder/lens_calibrate_gui.app/Contents"
relHuginApp="../../Hugin.app/Contents"

#------------------------------------------------

# First PTBatcherGui
PTBGbins="align_image_stack autooptimiser celeste_standalone checkpto cpclean cpfind hugin_hdrmerge icpfind nona pano_modify PTblender PTmasker PTmender PToptimizer PTroller enfuse enblend gnumake open"
cd "$PTBatcherGuiApp/MacOS"
for binary in $PTBGbins
do 
	rm $binary
	ln -s "../$relHuginApp/MacOS/$binary"
done

cd "$PTBatcherGuiApp"
# Remove frameworks, Libraries and Resources and create symbolic links
# PTBatcherGui.icns are nowadays also installed in Hugin.app
rm -rf Frameworks Libraries Resources
ln -s "$relHuginApp/Frameworks"
ln -s "$relHuginApp/Libraries"
ln -s "$relHuginApp/Resources"
cd $installerfolder

# Do about the same for the calibrate_lens_gui app
cd "$lens_calibrate_guiApp"
rm -rf Frameworks Libraries Resources
ln -s "$relHuginApp/Frameworks"
ln -s "$relHuginApp/Libraries"
ln -s "$relHuginApp/Resources"
cd $installerfolder
## That should do it

#Finally create a symbolic link to PTBatcherGui inside Hugin.app for simpler call to straight binary
cd "$HuginApp/MacOS"
ln -s ../../../PTBatcherGui.app/Contents/MacOS/PTBatcherGui PTBatcherGui
 


#!/bin/sh

# $Id: complete-embed.sh $

dylib_install_loc="Libraries"

HuginApp="$TARGET_BUILD_DIR/Hugin.app"
StitcherApp="$HuginApp/Contents/Resources/HuginStitchProject.app"

#------------------------------------------------

rm -Rf "$StitcherApp/Contents/$dylib_install_loc"
ln -s "../../../$dylib_install_loc" "$StitcherApp/Contents/$dylib_install_loc"
rm -Rf "$StitcherApp/Contents/Frameworks"
ln -s "../../../Frameworks" "$StitcherApp/Contents/Frameworks"
rm -Rf "$StitcherApp/Contents/Resources"
ln -s "../../../Resources" "$StitcherApp/Contents/Resources"
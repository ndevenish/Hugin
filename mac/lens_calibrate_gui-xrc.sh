#!/bin/sh

# $Id: lens_calibrate_gui-xrc.sh 2011 2011-06-05 HvdW $

huginVer="$HUGIN_PACKAGE_VERSION"
huginBuilder="$HUGIN_BUILDER"
resdir="$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/Resources"
huginsrcdir="../src/hugin1/hugin"
xrcsrcdir="$huginsrcdir/xrc"
celeste_data="../src/celeste/data"
icpfind_default="../src/hugin1/icpfind/default.mac"

# First create a Resources folder as lens_calibrate_gui-xrc doesn't have that by default
rm -fR $resdir/xrc $resdir
mkdir $resdir
echo "copying xrc folder to $resdir/xrc"
cp -R $xrcsrcdir $resdir/

echo "removing extra files from xrc folder"
for DIR in $resdir/xrc "$resdir/xrc/??*" "$resdir/xrc/??*/??*"
do 
 rm -fR $DIR/.hg*
 rm -f $DIR/.??*
 rm -fR $DIR/CVS
 rm -f $DIR/Makefil*
 rm -f $DIR/CMake*
done

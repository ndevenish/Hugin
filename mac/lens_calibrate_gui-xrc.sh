#!/bin/sh

# $Id: localised.sh 2004 2007-05-11 00:17:50Z ippei $

huginVer="$HUGIN_PACKAGE_VERSION"
huginBuilder="$HUGIN_BUILDER"
resdir="$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/Resources"
huginsrcdir="../src/hugin1/hugin"
xrcsrcdir="$huginsrcdir/xrc"
celeste_data="../src/celeste/data"
icpfind_default="../src/hugin1/icpfind/default.mac"

rm -fR $resdir/xrc
echo "copying xrc folder to $resdir/xrc"
cp -R $xrcsrcdir $resdir/
# This file is needed for the new tabbed About panel
cp ../COPYING $resdir/xrc/data/
echo "removing extra files from xrc folder"
for DIR in $resdir/xrc "$resdir/xrc/??*" "$resdir/xrc/??*/??*"
do 
 rm -fR $DIR/.svn
 rm -f $DIR/.??*
 rm -fR $DIR/CVS
 rm -f $DIR/Makefil*
 rm -f $DIR/CMake*
done

echo "patching $resdir/xrc/main_frame.xrc to have no border around the tab control."
mv $resdir/xrc/main_frame.xrc $resdir/xrc/main_frame.xrc-bk
sed -e s/wxALL// $resdir/xrc/main_frame.xrc-bk > $resdir/xrc/main_frame.xrc
rm $resdir/xrc/main_frame.xrc-bk

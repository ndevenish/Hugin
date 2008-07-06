#!/bin/sh

# $Id: localised.sh 2004 2007-05-11 00:17:50Z ippei $

huginVer="$HUGIN_PACKAGE_VERSION"
resdir="$TARGET_BUILD_DIR/Hugin.app/Contents/Resources"
huginsrcdir="../src/hugin1/hugin"
xrcsrcdir="$huginsrcdir/xrc"

rm -fR $resdir/xrc
echo copying xrc folder to $resdir/xrc
cp -R $xrcsrcdir $resdir/
echo removing extra files from xrc folder
for DIR in $resdir/xrc "$resdir/xrc/??*" "$resdir/xrc/??*/??*"
do 
 rm -fR $DIR/.svn
 rm -f $DIR/.??*
 rm -fR $DIR/CVS
 rm -f $DIR/Makefil*
 rm -f $DIR/CMake*
done

echo patching $resdir/xrc/cp_editor_panel.xrc to use wxChoice instead of wxNotebook
mv $resdir/xrc/cp_editor_panel.xrc $resdir/xrc/cp_editor_panel.xrc-bk
sed -e s/wxNotebook/wxChoice/ -e s/cp_editor_left_tab/cp_editor_left_choice/ -e s/cp_editor_right_tab/cp_editor_right_choice/ $resdir/xrc/cp_editor_panel.xrc-bk > $resdir/xrc/cp_editor_panel.xrc
rm $resdir/xrc/cp_editor_panel.xrc-bk

echo patching $resdir/xrc/main_frame.xrc to have no border around the tab control.
mv $resdir/xrc/main_frame.xrc $resdir/xrc/main_frame.xrc-bk
sed -e s/wxALL// $resdir/xrc/main_frame.xrc-bk > $resdir/xrc/main_frame.xrc
rm $resdir/xrc/main_frame.xrc-bk

echo "generating about.htm from about.htm.in"
sed -e "s/\${HUGIN_PACKAGE_VERSION}/$huginVer/g" $resdir/xrc/data/about.htm.in > $resdir/xrc/data/about.htm
rm $resdir/xrc/data/about.htm.in

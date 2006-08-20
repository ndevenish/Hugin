#!/bin/sh
#

if test $# -ne 2; then
  echo "install2bundle.sh usage: install2bundle.sh installed_dir bundle_dir"
  echo 
  echo "install2bundle.sh will produce a hugin and a nona_gui application"
  echo "bundle in the bundle_dir:  bundle_dir/hugin.app and bundle_dir/nona_gui.app."
  echo
  echo "installed_dir should point the the --prefix argument specifed during configure."
  echo "This script has to be run from the within the mac subdirectory of the hugin"
  echo "source tree."
  return 1
fi

INSTALLED_DIR=$1
TARGET_BUNDLE_HUGIN="$2/hugin.app"
TARGET_BUNDLE_NONA="$2/nona_gui.app"

mkdir -p $TARGET_BUNDLE_HUGIN

#wxDir="../../ExternalPrograms/wxWidgets-2.7.0-1"
#wxDir="/opt/local/"


echo "=== Building hugin bundle ==="
mkdir -p $TARGET_BUNDLE_HUGIN/Contents

echo "Copy hugin executable"
cp HuginOSX-Info.plist $TARGET_BUNDLE_HUGIN/Contents/Info.plist

echo "APPLHgin" > $TARGET_BUNDLE_HUGIN/Contents/PkgInfo

mkdir -p  $TARGET_BUNDLE_HUGIN/Contents/MacOS
cp $INSTALLED_DIR/bin/hugin  $TARGET_BUNDLE_HUGIN/Contents/MacOS/huginOSX
strip $TARGET_BUNDLE_HUGIN/Contents/MacOS/huginOSX

resdir="$TARGET_BUNDLE_HUGIN/Contents/Resources"
xrcsrcdir="$INSTALLED_DIR/share/hugin/xrc"

mkdir -p $resdir

cp HuginFiles.icns HuginOSX.icns $resdir/

rm -fR $resdir/xrc
echo copying xrc folder to $resdir/xrc
cp -R $xrcsrcdir $resdir/

echo patching $resdir/xrc/cp_editor_panel-2.5.xrc to use wxChoice instead of wxNotebook
mv $resdir/xrc/cp_editor_panel-2.5.xrc $resdir/xrc/cp_editor_panel-2.5.xrc-bk
sed -e s/wxNotebook/wxChoice/ -e s/cp_editor_left_tab/cp_editor_left_choice/ -e s/cp_editor_right_tab/cp_editor_right_choice/ $resdir/xrc/cp_editor_panel-2.5.xrc-bk > $resdir/xrc/cp_editor_panel-2.5.xrc

echo patching $resdir/xrc/main_frame-2.5.xrc to have no border around the tab control.
mv $resdir/xrc/main_frame-2.5.xrc $resdir/xrc/main_frame-2.5.xrc-bk
sed -e s/wxALL// $resdir/xrc/main_frame-2.5.xrc-bk > $resdir/xrc/main_frame-2.5.xrc


for lang in "en" `cat ../src/hugin/po/LINGUAS|grep -v "#.*"`
do
 
 echo 
 echo "Language: $lang"

 localisedresdir="$resdir/$lang.lproj"
 localedir="$localisedresdir/locale"

 rm -fR $localisedresdir
 echo "deleting $localisedresdir"

 mkdir -p "$localisedresdir"
 echo "making $localisedresdir"
 mkdir -p "$localedir"
 echo "making $localedir"

 helplang="$lang"
 if [ $lang = "en" ]
 then
  helplang="en_EN"
 fi
 if [ $lang = "fr" ]
 then
  helplang="fr_FR"
 fi
 
 if [ -d "$xrcsrcdir/data/help_$helplang" ]
 then
  echo "moving help_$helplang to $localisedresdir/help"
  mkdir -p  "$localisedresdir/help"
  cp -R "$resdir/xrc/data/help_$helplang" "$localisedresdir/help"
  for file in `ls $localisedresdir/help | grep .html`
  do
   echo  rewriting \'src=\"../help_common\' to \'src=\"../../xrc/data/help_common\'
   sed s/src\=\"..\\/help_common/src\=\"..\\/..\\/xrc\\/data\\/help_common/ "$localisedresdir/help/$file" > $localisedresdir/help/$file-copy
   mv $localisedresdir/help/$file-copy $localisedresdir/help/$file
  done
 fi
 
 if [ $lang = "en" ]
 then
  continue
 fi
 
 echo "$lang/hugin.mo from $lang.po"
 msgfmt -v -o "$localedir/hugin.mo" "../src/hugin/po/$lang.po"
 
done

echo "=== Building nona_gui bundle ==="
mkdir -p $TARGET_BUNDLE_NONA/Contents

echo "Copy nona executable"
cp nona_gui-Info.plist $TARGET_BUNDLE_NONA/Contents/Info.plist

echo "APPLNona" > $TARGET_BUNDLE_NONA/Contents/PkgInfo

mkdir -p  $TARGET_BUNDLE_NONA/Contents/MacOS
cp $INSTALLED_DIR/bin/nona_gui  $TARGET_BUNDLE_NONA/Contents/MacOS/nona_gui
strip $TARGET_BUNDLE_NONA/Contents/MacOS/nona_gui

resdir="$TARGET_BUNDLE_NONA/Contents/Resources"

cp HuginFiles.icns HuginOSX.icns $resdir/

for lang in "en" `cat ../src/nona_gui/po/LINGUAS|grep -v "#.*"`
do
 
 echo 
 echo "Language: $lang"

 localisedresdir="$resdir/$lang.lproj"
 localedir="$localisedresdir/locale"

 rm -fR $localisedresdir
 echo "deleting $localisedresdir"

 mkdir -p "$localisedresdir"
 echo "making $localisedresdir"
 mkdir -p "$localedir"
 echo "making $localedir"

 if [ $lang = "en" ]
 then
  continue
 fi
 
 echo "$lang/nona_gui.mo from $lang.po"
 msgfmt -v -o "$localedir/nona_gui.mo" "../src/nona_gui/po/$lang.po"
 
done


#!/bin/sh

# $Id: localised.sh 2004 2007-05-11 00:17:50Z ippei $

huginVer='0.7svn\ \(experimental\ by\ ippei\)'
wxDir="./ExternalPrograms/wxMac-2.8.7"
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

for lang in "en" `cat $huginsrcdir/po/LINGUAS|grep -v "#.*"`
do
 
 echo 
 echo "Language: $lang"

 localisedresdir="$resdir/$lang.lproj"
 localedir="$localisedresdir/locale"

 rm -fR $localisedresdir
 echo "deleting $localisedresdir"

 mkdir "$localisedresdir"
 echo "making $localisedresdir"
 mkdir "$localedir"
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
 msgfmt -v -o "$localedir/hugin.mo" "$huginsrcdir/po/$lang.po"
 
 echo "$lang/wxstd.mo from $wxDir/locale/$lang.po"
 if [ -f "$wxDir/locale/$lang.po" ]
 then
  msgfmt -v -o "$localedir/wxstd.mo" "$wxDir/locale/$lang.po"
 else
  echo "$lang.po not found;"
  parentLang=`echo $lang|sed s/_.*//`
  echo "$lang/wxstd.mo from $wxDir/locale/$parentLang.po"
  msgfmt -v -o "$localedir/wxstd.mo" "$wxDir/locale/$parentLang.po"
 fi
 
 for file in `ls $xrcsrcdir/data | grep _$lang.htm`
 do
  echo copying $file to $localisedresdir/`echo $file|sed s/_$lang//`
  echo  rewriting \'src=\"\' to \'src=\"../xrc/data/\'
  sed s/src\=\"/src\=\"..\\/xrc\\/data\\// "$xrcsrcdir/data/$file" > $localisedresdir/`echo $file|sed s/_$lang//`
 done

 for file in `ls $xrcsrcdir/data | grep _$lang.html`
 do
  echo copying $file to $localisedresdir/`echo $file|sed s/_$lang//`
  echo  rewriting \'src=\"\' to \'src=\"../xrc/data/\'
  sed s/src\=\"/src\=\"..\\/xrc\\/data\\// "$xrcsrcdir/data/$file" > $localisedresdir/`echo $file|sed s/_$lang//`
 done

# for file in `ls $xrcsrcdir/data | grep _$lang-UTF8.txt`
# do
#  echo copying $file to $localisedresdir/`echo $file|sed s/_$lang//`
#  cp "$xrcsrcdir/data/$file" $localisedresdir/`echo $file|sed s/_$lang//`
# done

done
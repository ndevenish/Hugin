#!/bin/sh

wxDir="../../ExternalPrograms/wxMac-2.6.3"
resdir="$TARGET_BUILD_DIR/HuginOSX.app/Contents/Resources"
xrcsrcdir="../src/hugin/xrc"

rm -fR $resdir/xrc
echo copying xrc folder to $resdir/xrc
cp -R $xrcsrcdir $resdir/
echo removing extra files from xrc folder
rm -f $resdir/xrc/.??*
rm -fR $resdir/xrc/CVS
rm -f $resdir/xrc/Makefil*
rm -f $resdir/xrc/data/.??*
rm -fR $resdir/xrc/data/CVS
rm -f $resdir/xrc/data/Makefil*


#for xrcfile in `ls $resdir/xrc | grep mac.xrc`
#do
#  echo using $resdir/xrc/$xrcfile instead of $resdir/xrc/`echo $xrcfile|sed s/-mac.xrc/.xrc/`
#  mv -f $resdir/xrc/$xrcfile $resdir/xrc/`echo $xrcfile|sed s/-mac.xrc/.xrc/`
#done

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

 mkdir "$localisedresdir"
 echo "making $localisedresdir"
 mkdir "$localedir"
 echo "making $localedir"

 longlang="$lang"
 if [ $lang = "en" ]
 then
  longlang="en_EN"
 fi
 if [ $lang = "fr" ]
 then
  longlang="fr_FR"
 fi

 for helpfolder in `ls $xrcsrcdir/data | grep help_$longlang`
 do
  echo removing extra files from $xrcsrcdir/data/$helpfolder
  rm -f $xrcsrcdir/data/$helpfolder/.??*
  rm -fR $xrcsrcdir/data/$helpfolder/CVS
  rm -f $xrcsrcdir/data/$helpfolder/Makefil*
  
  echo copying $helpfolder to $localisedresdir/help
  cp -fR "$xrcsrcdir/data/$helpfolder" "$localisedresdir/help"
 done
 
 if [ $lang = "en" ]
 then
  continue
 fi
 
 echo "$lang/hugin.mo from $lang.po"
 msgfmt -v -o "$localedir/hugin.mo" "../src/hugin/po/$lang.po"
 
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
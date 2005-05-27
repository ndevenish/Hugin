#!/bin/sh

for lang in `cat languages`
do
 
 echo 
 echo "Language: $lang"

 resdir="build/HuginOSX.app/Contents/Resources/$lang.lproj"
 localedir="$resdir/locale"
 datasrcdir="../src/hugin/xrc/data"

 rm -fR $resdir
 echo "deleting $resdir"

 mkdir "$resdir"
 echo "making $resdir"
 mkdir "$localedir"
 echo "making $localedir"

 if [ $lang = "en" ]
 then
  continue
 fi

 echo "wxstd.mo to $lang.po"
 msgfmt -v -o "$localedir/wxstd.mo" "../../wxMac-2.6.0/locale/$lang.po"
 echo "hugin.mo to $lang.po"
 msgfmt -v -o "$localedir/hugin.mo" "../src/hugin/po/$lang.po"

 for file in `ls $datasrcdir | grep _$lang.htm`
 do
  echo copying $file to $resdir/`echo $file|sed s/_$lang//`
  echo  rewriting \'src=\"\' to \'src=\"../xrc/data/\'
  sed s/src\=\"/src\=\"..\\/xrc\\/data\\// "$datasrcdir/$file" > $resdir/`echo $file|sed s/_$lang//`
 done

 for file in `ls $datasrcdir | grep _$lang.html`
 do
  echo copying $file to $resdir/`echo $file|sed s/_$lang//`
  echo  rewriting \'src=\"\' to \'src=\"../xrc/data/\'
  sed s/src\=\"/src\=\"..\\/xrc\\/data\\// "$datasrcdir/$file" > $resdir/`echo $file|sed s/_$lang//`
 done

 for file in `ls $datasrcdir | grep _$lang-UTF8.txt`
 do
  echo copying $file to $resdir/`echo $file|sed s/_$lang//`
  cp "$datasrcdir/$file" $resdir/`echo $file|sed s/_$lang//`
 done

done


for xrcfile in `ls | grep mac.xrc`
do
  echo copying $xrcfile to build/HuginOSX.app/Contents/Resources/xrc/`echo $xrcfile|sed s/-mac.xrc/.xrc/`
 cp -f $xrcfile build/HuginOSX.app/Contents/Resources/xrc/`echo $xrcfile|sed s/-mac.xrc/.xrc/`
done
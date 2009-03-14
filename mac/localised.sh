#!/bin/sh

# $Id: localised.sh 2004 2007-05-11 00:17:50Z ippei $

wxWidgetsLocaleDir="$WX_LOCALE_DIR"
resdir="$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/Resources"
huginsrcdir="../src/hugin1/hugin"
xrcsrcdir="$huginsrcdir/xrc"
translationsdir="../src/translations"

mkdir -p "$resdir"

for lang in "en" $(ls $translationsdir/*.po | sed -e "s/^.*\///g" -e "s/\.po//g")
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
 
 if [ $lang = "en" ]
 then
  continue
 else
   if [ $lang != "fr" ] && [ $lang != "it" ]
   then
       ln -s "$resdir/en.lproj/help" "$localisedresdir/help"
   fi
 fi
 
 echo "$lang/hugin.mo from $lang.po"
 msgfmt -v -o "$localedir/hugin.mo" "$translationsdir/$lang.po"
 
 echo "$lang/wxstd.mo from $wxWidgetsLocaleDir/$lang.po"
 if [ -f "$wxWidgetsLocaleDir/$lang.po" ]
 then
  msgfmt -v -o "$localedir/wxstd.mo" "$wxWidgetsLocaleDir/$lang.po"
 else
  echo "$lang.po not found;"
  parentLang=`echo $lang|sed s/_.*//`
  echo "$lang/wxstd.mo from $wxWidgetsLocaleDir/$parentLang.po"
  if [ -f "$wxWidgetsLocaleDir/$parentLang.po" ]
  then
   msgfmt -v -o "$localedir/wxstd.mo" "$wxWidgetsLocaleDir/$parentLang.po"
  else
   echo "$parentLang.po not found;"
  fi
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

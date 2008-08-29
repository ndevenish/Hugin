#!/bin/sh
# 03-localise-bundle.sh
# Harry van der Wolf, 2008
# This file is almost completely copied from Ippei Ukai's localisation scripts

# Set correct environment
source ../ExternalPrograms/scripts/SetEnv-universal.txt
source ./bundle_constants.txt

 # We do not want any interruption from pkg-configs, wx-configs and so on
 # from MacPorts or Fink, who's paths were added to the PATH
 #export PATH=/bin:/sbin:/usr/bin:/usr/sbin:$REPOSITORYDIR/bin

 # If you are on Tiger and you have dowloaded the binary subversion from
 # Martin Ott's webpage, you also need to add /usr/local/bin
 # In that case uncomment the line below and outcomment the above PATH statement
 export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:$REPOSITORYDIR/bin



# Set some variables
H_app="build/Hugin.app"
wxWidgetsLocaleDir="$WX_LOCALE_DIR"
huginBuilder="$HUGIN_BUILDER"
resdir="$H_app/Contents/Resources"
huginsrcdir="../../src/hugin1/hugin"
xrcsrcdir="$huginsrcdir/xrc"
translationsdir="../../src/translations"



#######
# First patch xrc

rm -fR $resdir/xrc
echo ""
echo "copying xrc folder to $resdir/xrc"
cp -R $xrcsrcdir $resdir/
echo "removing extra files from xrc folder"
for DIR in $resdir/xrc "$resdir/xrc/??*" "$resdir/xrc/??*/??*"
do 
 rm -fR $DIR/.svn
 rm -f $DIR/.??*
 rm -fR $DIR/CVS
 rm -f $DIR/Makefil*
 rm -f $DIR/CMake*
done

echo "patching $resdir/xrc/cp_editor_panel.xrc to use wxChoice instead of wxNotebook"
mv $resdir/xrc/cp_editor_panel.xrc $resdir/xrc/cp_editor_panel.xrc-bk
sed -e s/wxNotebook/wxChoice/ -e s/cp_editor_left_tab/cp_editor_left_choice/ -e s/cp_editor_right_tab/cp_editor_right_choice/ $resdir/xrc/cp_editor_panel.xrc-bk > $resdir/xrc/cp_editor_panel.xrc
rm $resdir/xrc/cp_editor_panel.xrc-bk

echo "patching $resdir/xrc/main_frame.xrc to have no border around the tab control."
mv $resdir/xrc/main_frame.xrc $resdir/xrc/main_frame.xrc-bk
sed -e s/wxALL// $resdir/xrc/main_frame.xrc-bk > $resdir/xrc/main_frame.xrc
rm $resdir/xrc/main_frame.xrc-bk

#echo "generating about.htm from about.htm.in"
#sed -e "s/\${HUGIN_PACKAGE_VERSION}/$huginVer/g" \
#    -e "s/\${HUGIN_BUILDER}/$huginBuilder/g" \
#    $resdir/xrc/data/about.htm.in > $resdir/xrc/data/about.htm
#rm $resdir/xrc/data/about.htm.in

cp $REPOSITORYDIR/share/hugin/xrc/data/about.htm $resdir/xrc/data/about.htm


########
# Add localisation
echo ""
echo "Adding localisation files to the bundle"
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


done

########
# Install and "correct" help files
echo ""
echo "Install help files and change HTML src paths"
for helplang in "en_EN fr_FR"
do
 
  localisedresdir="$resdir/$(echo en_EN  | grep -o '^[^_]*').lproj"
 
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
 
done

echo ""
echo "All done. You should now have a working and fully portable Universal bundle."
echo ""
echo ""


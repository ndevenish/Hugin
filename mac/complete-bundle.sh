#!/bin/sh

# $Id: complete-bundle.sh $

dylib_dir="../mac/ExternalPrograms/repository/lib"
old_install_name_dirname="/Users/Shared/development/hugin_related/hugin/mac/ExternalPrograms/repository/lib"
dylib_install_loc="Libraries"
new_install_name_dirname="@executable_path/../$dylib_install_loc"

#[hack]
if [ ! $PRODUCT_NAME = "Hugin" ]
then
 exit 0;
fi

App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"

archs="ppc i386"
libs="libwx_macu-$WX_MAJOR_VERSOIN libpano13 libboost_thread-mt libboost_thread-mt-1_34_1 libpng libtiff libjpeg libIex libImath libIlmImf libIlmThread libHalf libexpat liblcms"

#[hack]
#binaries="$App/Contents/$dylib_install_loc/*.dylib $App/Contents/MacOS/* $App/Contents/Frameworks/Hugin*.framework/Hugin*"
binaries="$App/Contents/$dylib_install_loc/*.dylib $App/Contents/MacOS/* $App/Contents/Frameworks/Hugin*.framework/Hugin* $App/Contents/Resources/Hugin*.app/Contents/MacOS/*"

#------------------------------------------------

rm -Rf "$App/Contents/$dylib_install_loc"
mkdir -p "$App/Contents/$dylib_install_loc"
for dylib in $libs
do
 cp -Rf $dylib_dir/$dylib.*ylib "$App/Contents/$dylib_install_loc/"
done

#replace any "$old_install_name_dirname[/*]/" in install_name to "$new_install_name_dirname/"
 
for exec_file in $binaries
do
 
 echo "Processing: $exec_file"
 
 if [[ $exec_file = *.dylib ]]
 then
  for lib in $(otool -D $exec_file | grep $old_install_name_dirname | sed -e 's/ (.*$//' -e 's/^.*\///')
  do
   echo " Changing own install name."
   install_name_tool -id "$new_install_name_dirname/$lib" $exec_file
  done
 fi
 
 for lib in $(otool -L $exec_file | grep $old_install_name_dirname | sed -e 's/ (.*$//' -e 's/^.*\///')
 do
  echo " Changing install name for: $lib"
  install_name_tool -change "$old_install_name_dirname/$lib" "$new_install_name_dirname/$lib" $exec_file
 done

done

#!/bin/sh

# $Id: complete-bundle.sh $

dylib_dir="$REPOSITORY_DIR/lib"
old_install_name_dirname="$REPOSITORY_ABSOLUTE_PATH/lib"
dylib_install_loc="Libraries"
new_install_name_dirname="@executable_path/../$dylib_install_loc"

App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"

archs="ppc i386 x86_64"
libs="libwx_macu-$WX_MAJOR_VERSION libwx_macu_gl-$WX_MAJOR_VERSION libpano13 $BOOST_THREAD_LIB-$BOOST_VER $BOOST_DATE_TIME_LIB-$BOOST_VER $BOOST_FILESYSTEM_LIB-$BOOST_VER $BOOST_IOSTREAMS_LIB-$BOOST_VER $BOOST_REGEX_LIB-$BOOST_VER $BOOST_SYSTEM_LIB-$BOOST_VER $BOOST_SIGNALS_LIB-$BOOST_VER libpng libtiff libjpeg libIex libImath libIlmImf libIlmThread libHalf libexpat liblcms libintl libgettextsrc-$GETTEXT_VERSION libgettextpo libgettextlib-$GETTEXT_VERSION libasprintf libexiv2 libGLEW libxmi libiconv" 

#binaries="$App/Contents/$dylib_install_loc/*.dylib $App/Contents/MacOS/* $App/Contents/Frameworks/Hugin*.framework/Hugin* $App/Contents/Frameworks/icpfind.framework/icpfind* $App/Contents/Frameworks/local*.framework/local* $App/Contents/Frameworks/hugin_python*.framework/hugin* $App/Contents/Resources/align_image_stack $App/Contents/Resources/cpfind"
# temporary remove python stuff
binaries="$App/Contents/$dylib_install_loc/*.dylib $App/Contents/MacOS/* $App/Contents/Frameworks/Hugin*.framework/Hugin* $App/Contents/Frameworks/icpfind.framework/icpfind* $App/Contents/Frameworks/local*.framework/local* $App/Contents/Resources/align_image_stack $App/Contents/Resources/cpfind"


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

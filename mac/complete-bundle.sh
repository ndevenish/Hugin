#!/bin/sh

# $Id: complete-bundle.sh $

dylib_dir="$REPOSITORY_DIR/lib"
old_install_name_dirname="$REPOSITORY_ABSOLUTE_PATH/lib"
dylib_install_loc="Libraries"
old_usr_install_name_dirname="/usr/local/lib"
new_install_name_dirname="@executable_path/../$dylib_install_loc"

App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"

libs="libwx_osx_cocoau-$WX_VERSION libwx_osx_cocoau_gl-$WX_VERSION libpano13 $BOOST_THREAD_LIB-$BOOST_VER $BOOST_DATE_TIME_LIB-$BOOST_VER $BOOST_FILESYSTEM_LIB-$BOOST_VER $BOOST_IOSTREAMS_LIB-$BOOST_VER $BOOST_REGEX_LIB-$BOOST_VER $BOOST_SYSTEM_LIB-$BOOST_VER $BOOST_SIGNALS_LIB-$BOOST_VER libpng libtiff libtiffxx libjpeg libIex libImath libIlmImf libIlmThread libHalf libexpat liblcms libintl libgettextsrc-$GETTEXT_VERSION libgettextpo libgettextlib-$GETTEXT_VERSION libasprintf libexiv2 libGLEW libxmi libiconv libffi libglib-2 libgio-2 libgobject-2 libgmodule-2 libgthread-2 liblensfun" 
# python version
#libs="libwx_macu-$WX_VERSION libwx_macu_gl-$WX_VERSION libpano13 $BOOST_THREAD_LIB-$BOOST_VER $BOOST_DATE_TIME_LIB-$BOOST_VER $BOOST_FILESYSTEM_LIB-$BOOST_VER $BOOST_IOSTREAMS_LIB-$BOOST_VER $BOOST_REGEX_LIB-$BOOST_VER $BOOST_SYSTEM_LIB-$BOOST_VER $BOOST_SIGNALS_LIB-$BOOST_VER libpng libtiff libtiffxx libjpeg libIex libImath libIlmImf libIlmThread libHalf libexpat liblcms libintl libgettextsrc-$GETTEXT_VERSION libgettextpo libgettextlib-$GETTEXT_VERSION libasprintf libexiv2 libGLEW libxmi libiconv libffi libglib-2 libgio-2 libgobject-2 libgmodule-2 libgthread-2 liblensfun libpython2.7" 
openmp_libs_dylib_dir="/usr/local/lib"
lion_required_openmp_libs="libgcc_s.1 libgomp.1 libstdc++.6"

binaries="$App/Contents/$dylib_install_loc/*.dylib $App/Contents/MacOS/* $App/Contents/Frameworks/Hugin*.framework/Hugin* $App/Contents/Frameworks/icpfind.framework/icpfind* $App/Contents/Frameworks/local*.framework/local*"


#------------------------------------------------

rm -Rf "$App/Contents/$dylib_install_loc"
mkdir -p "$App/Contents/$dylib_install_loc"
for dylib in $libs
do
 cp -Rf $dylib_dir/$dylib*.*ylib "$App/Contents/$dylib_install_loc/"
done
# Now do the same for the openmp required gcc-4.6 libraries
for dylib in $lion_required_openmp_libs
do
 cp -Rf $openmp_libs_dylib_dir/$dylib.*ylib "$App/Contents/$dylib_install_loc/"
done


#replace any "$old_install_name_dirname[/*]/" in install_name to "$new_install_name_dirname/" 
for exec_file in $binaries
do
 
 echo "Processing: $exec_file"
 
 echo "First do the $old_install_name_dirname"
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

 echo "Repeat this all for $old_usr_install_name_dirname for new openmp lion builds"
  if [[ $exec_file = *.dylib ]]
 then
  for lib in $(otool -D $exec_file | grep $old_usr_install_name_dirname | sed -e 's/ (.*$//' -e 's/^.*\///')
  do
   echo " Changing own install name."
   install_name_tool -id "$new_install_name_dirname/$lib" $exec_file
  done
 fi
 
 for lib in $(otool -L $exec_file | grep $old_usr_install_name_dirname | sed -e 's/ (.*$//' -e 's/^.*\///')
 do
  echo " Changing install name for: $lib"
  install_name_tool -change "$old_usr_install_name_dirname/$lib" "$new_install_name_dirname/$lib" $exec_file
 done

done


# fix issue with libboost libraries where the symlink is not copied
cd $TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/Libraries
ln -s libboost_date_time-$BOOST_VER.dylib libboost_date_time.dylib
ln -s libboost_filesystem-$BOOST_VER.dylib libboost_filesystem.dylib
ln -s libboost_iostreams-$BOOST_VER.dylib libboost_iostreams.dylib
ln -s libboost_regex-$BOOST_VER.dylib libboost_regex.dylib
ln -s libboost_thread-$BOOST_VER.dylib libboost_thread.dylib
ln -s libboost_system-$BOOST_VER.dylib libboost_system.dylib


#!/bin/sh
# $Id: complete-bundle.sh $

dylib_dir="$REPOSITORY_DIR/lib"
old_install_name_dirname="$REPOSITORY_ABSOLUTE_PATH/lib"
dylib_install_loc="Frameworks"
old_usr_install_name_dirname="/usr/local/lib"
new_install_name_dirname="@executable_path/../$dylib_install_loc"

App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"

openmp_libs_dylib_dir="$GCC_MP_LOCATION"
lion_required_openmp_libs="libgcc_s.1 libgomp.1 libstdc++.6"

binaries="$App/Contents/MacOS/*"
libraries="$App/Contents/$dylib_install_loc/*.dylib"

#------------------------------------------------
# Copy External Libraries in .app/Contents/Libraries
mkdir -p "$App/Contents/$dylib_install_loc"
libs="libwx_osx_cocoau-$WX_VERSION libwx_osx_cocoau_gl-$WX_VERSION libpano13 \
  $BOOST_THREAD_LIB-$BOOST_VER $BOOST_DATE_TIME_LIB-$BOOST_VER  \
  $BOOST_FILESYSTEM_LIB-$BOOST_VER $BOOST_IOSTREAMS_LIB-$BOOST_VER  \
  $BOOST_REGEX_LIB-$BOOST_VER $BOOST_SYSTEM_LIB-$BOOST_VER  \
  $BOOST_SIGNALS_LIB-$BOOST_VER libpng libtiff libtiffxx libjpeg libIex  \
  libIexMath libImath libIlmImf libIlmThread libHalf libexpat liblcms  \
  libintl libgettextsrc-$GETTEXT_VERSION libgettextpo \
  libgettextlib-$GETTEXT_VERSION libasprintf libexiv2 libgsl libgslcblas \
  libGLEW libiconv libffi libglib-2 libgio-2 libgobject-2 libgmodule-2 \
  libgthread-2 liblensfun libvigraimpex"
for dylib in $libs; do
    cp -Rf $dylib_dir/$dylib*.*ylib "$App/Contents/$dylib_install_loc/"
done

# Now do the same for the openmp required gcc >= 4.6 libraries
for dylib in $lion_required_openmp_libs; do
    cp -Rf $openmp_libs_dylib_dir/$dylib.*ylib "$App/Contents/$dylib_install_loc/"
done

# replace any "$old_install_name_dirname[/*]/" in install_name to "$new_install_name_dirname/"
echo "Processing in $App/Contents/MacOS"
for exec_file in $binaries
do
    echo "Processing: $(basename $exec_file)"
    for lib in $(otool -L $exec_file | grep $old_install_name_dirname | sed -e 's/ (.*$//' -e 's/^.*\///'); do
        echo "    Changing install name for: $lib"
        install_name_tool -change "$old_install_name_dirname/$lib" "$new_install_name_dirname/$lib" $exec_file
    done
done

echo "Processing in $App/Contents/$dylib_install_loc"
for dylib_file in $libraries; do
    echo "Processing : $(basename $dylib_file)"
    echo "    Changing own install name."
    install_name_tool -id "$new_install_name_dirname/$(basename $dylib_file)" $dylib_file
    for lib in $(otool -L $dylib_file|grep $old_install_name_dirname|sed -e 's/ (.*//;s,^.*/,,'); do
        echo "   Changing install name for : $lib"
        install_name_tool -change "$old_install_name_dirname/$lib" \
            @loader_path/$lib \
            $dylib_file
    done

    for lib in $(otool -L $dylib_file|grep $new_install_name_dirname|grep -v $(basename $dylib_file)|sed -e 's/ (.*//;s,^.*/,,'); do
        echo "   Changing install name for : $lib"
        install_name_tool -change "$new_install_name_dirname/$lib" \
            @loader_path/$lib \
            $dylib_file
    done
done

# change OpenMP libraries install_name and related binaries
for lib in $lion_required_openmp_libs; do
    install_name_tool -id $new_install_name_dirname/$lib.dylib $App/Contents/$dylib_install_loc/$lib.dylib
done

for lib1 in $lion_required_openmp_libs; do
    for lib2 in $lion_required_openmp_libs; do
        install_name_tool -change $openmp_libs_dylib_dir/$lib2.dylib \
            $new_install_name_dirname/$lib2.dylib \
            $App/Contents/$dylib_install_loc/$lib1.dylib
    done
done

xxxblends="enblend enfuse multiblend"
for bin in $xxxblends; do
    for lib in $lion_required_openmp_libs; do
	if [ -f $App/Contents/MacOS/$bin ]; then
	    install_name_tool -change $openmp_libs_dylib_dir/$lib.dylib \
            $new_install_name_dirname/$lib.dylib \
            $App/Contents/MacOS/$bin
	fi
    done
done

for lib in $lion_required_openmp_libs; do
    install_name_tool -change \
        $openmp_libs_dylib_dir/$lib.dylib \
        $new_install_name_dirname/$lib.dylib \
        $App/Contents/$dylib_install_loc/libvigraimpex.${VIGRA_IMPEX_VER}.dylib
done

# fix issue with libboost libraries where the symlink is not copied
cd $TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/$dylib_install_loc
ln -sf libboost_date_time-$BOOST_VER.dylib libboost_date_time.dylib
ln -sf libboost_filesystem-$BOOST_VER.dylib libboost_filesystem.dylib
ln -sf libboost_iostreams-$BOOST_VER.dylib libboost_iostreams.dylib
ln -sf libboost_regex-$BOOST_VER.dylib libboost_regex.dylib
ln -sf libboost_thread-$BOOST_VER.dylib libboost_thread.dylib
ln -sf libboost_system-$BOOST_VER.dylib libboost_system.dylib

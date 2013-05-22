#!/bin/sh
App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"
dylib_install_loc="Frameworks"
old_install_name_dirname="$REPOSITORY_ABSOLUTE_PATH/lib"
new_install_name_dirname="@executable_path/../$dylib_install_loc"
Resources="$TARGET_BUILD_DIR/$PRODUCT_NAME.app/Contents/Resources"
PythonLib="$App/Contents/Frameworks/Python27.framework/Versions/Current/lib/python2.7"
PythonBin="$App/Contents/Frameworks/Python27.framework/Versions/Current/bin"
PythonRes="$App/Contents/Frameworks/Python27.framework/Resources"

install_name_tool -change $old_install_name_dirname/libintl.8.dylib \
    @loader_path/../../../../../../libintl.8.dylib \
    $PythonLib/lib-dynload/_locale.so

# Change install_name paths in _hsi
install_name_tool -change $old_install_name_dirname/libboost_system-1_46.dylib \
    @loader_path/../../../../../../libboost_system.dylib \
    $PythonLib/site-packages/_hsi.so

install_name_tool -change $old_install_name_dirname/libpano13.2.dylib \
    @loader_path/../../../../../../libpano13.2.dylib \
    $PythonLib/site-packages/_hsi.so

install_name_tool -change $REPOSITORY_ABSOLUTE_PATH/Frameworks/Python27.framework/Versions/2.7/Python27 \
    @loader_path/../../../Python27 \
    $PythonLib/site-packages/_hsi.so

install_name_tool -change $new_install_name_dirname/libhugin_base-${HUGIN_VERSION_MAJOR}.${HUGIN_VERSION_MINOR}.dylib \
    @loader_path/../../../../../../libhugin_base-${HUGIN_VERSION_MAJOR}.${HUGIN_VERSION_MINOR}.dylib \
    $PythonLib/site-packages/_hsi.so

install_name_tool -change $new_install_name_dirname/libmakefile-${HUGIN_VERSION_MAJOR}.${HUGIN_VERSION_MINOR}.dylib \
    @loader_path/../../../../../../libmakefile-${HUGIN_VERSION_MAJOR}.${HUGIN_VERSION_MINOR}.dylib \
    $PythonLib/site-packages/_hsi.so

#change install name path in hpi
install_name_tool -change $REPOSITORY_ABSOLUTE_PATH/Frameworks/Python27.framework/Versions/2.7/Python27 \
    @loader_path/../Frameworks/Python27.framework/Versions/Current/Python27 \
    $App/Contents/Frameworks/libhpi-${HUGIN_VERSION_MAJOR}.${HUGIN_VERSION_MINOR}.dylib

# change install name paths for python executables
install_name_tool -change $REPOSITORY_ABSOLUTE_PATH/Frameworks/Python27.framework/Versions/2.7/Python27 \
    @loader_path/../Python27 \
    $PythonBin/python2.7

install_name_tool -change $REPOSITORY_ABSOLUTE_PATH/Frameworks/Python27.framework/Versions/2.7/Python27 \
    @loader_path/../Python27 \
    $PythonBin/pythonw2.7

install_name_tool -change $REPOSITORY_ABSOLUTE_PATH/Frameworks/Python27.framework/Versions/2.7/Python27 \
    @loader_path/../../../../Python27 \
    $PythonRes/Python.app/Contents/MacOS/Python27

# do the same for wxPython
wx_python_libs="_animate.so _aui.so _calendar.so _combo.so _controls_.so \
  _core_.so _dataview.so _gdi_.so _gizmos.so _glcanvas.so _grid.so _html.so \
  _html2.so _media.so _misc_.so _propgrid.so _richtext.so _stc.so _webkit.so \
  _windows_.so _wizard.so _xrc.so"
wx_libs="libwx_osx_cocoau libwx_osx_cocoau_gl"

for w in $wx_python_libs; do
    for l in $wx_libs; do
	install_name_tool -change $old_install_name_dirname/"$l"-"$WX_VERSION".dylib \
	    @loader_path/../../../../../../../../"$l"-"$WX_VERSION".dylib \
	    $PythonLib/site-packages/wx-$WX_VERSION-osx_cocoa/wx/$w
    done
done

# lib/python2.7/test/ is not needed
rm -rf $PythonLib/test
rm -rf $PythonLib/lib2to3/tests/

# some other things are not needed
rm -f $PythonLib/config/libpython2.7.{a,dylib}

# zip python standard library
cd $PythonLib
if [ -f site.py ]; then
    rm -rf ../python27.zip
    zip -r ../python27.zip *.py* bsddb compiler ctypes curses distutils email  \
        encodings hotshot idlelib importlib json logging multiprocessing \
        pydoc_data sqlite3 unittest wsgiref xml 1> /dev/null
    rm -rf                 *.py* bsddb compiler ctypes curses distutils email \
        encodings hotshot idlelib importlib json logging multiprocessing \
        pydoc_data sqlite3 unittest wsgiref xml
fi

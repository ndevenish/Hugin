#
# This is the first try at a script that can compile hugin under OSX using the
# standard autoconf build system. 
#
# This requires that all dependencies have been installed (use wxWidgets 2.7,
# on intel Mac).
#
# The settings below are valid if the dependencies were installed using
# darwinports. (see build_using_cmdline.txt for details)
#
# If you are using fink or build the dependencies by hand,
# some changes are needed, especially for the ./configure call.
#
# The programs are installed into a temporary folder. The install2bundle.sh
# script then builds a proper Mac OS bundle out of these files.
#

cd ..
TPREFIX=`pwd`
LDFLAGS=-L/opt/local/lib CPPFLAGS="-I/opt/local/include -I/Developer/Headers/FlatCarbon" ./configure --prefix=`pwd`/OSX_BUILD --with-unicode --with-wx-config=mac-unicode-release-2.7

make
make install

cd mac

# create hugin and nona_gui bundles in /Applications
./install2bundle.sh ../OSX_BUILD /Applications


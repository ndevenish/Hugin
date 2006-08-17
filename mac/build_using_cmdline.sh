#
# This is the first try at a script that can compile hugin under OSX using the standard autoconf
# build system.
#
# This requires that all dependencies have been installed (use wxWidgets 2.7, on intel Mac)
# Currently the script will fail while compiling nona_gui. However, one can still use
# hugin and the command line tools
#  
# The programs are installed into a temporary folder. The install2bundle.sh script then
# builds a proper Mac OS bundle out of the installed files.
#

cd ..
TPREFIX=`pwd`
LDFLAGS=-L/opt/local/lib CPPFLAGS="-I/opt/local/include -I/Developer/Headers/FlatCarbon" ./configure --prefix=`pwd`/OSX_BUILD --enable-debug --with-unicode

make
make install

cd mac
./install2bundle.sh


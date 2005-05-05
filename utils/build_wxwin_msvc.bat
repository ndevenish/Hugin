rem build wxwindows
rem

echo "In order to use the msvc project files, download wxWidgets 2.6,"
echo "and create this source tree:"
echo ""
echo "hugin              <- hugin source code"
echo "libs/wxWidgets     <- wxWidgets 2.6 source (rename wxWidgets-2.6.0 to wxWidgets)"
echo "libs/boost_1_31_0  <- boost library, only the headers are needed"
echo "libs/libpano       <- pano12.dll"
echo "Press return to continue"

pause

copy ../src/include/config_msvc.h ../src/include/config.h

cd ../../libs/wxWidgets/build/msw

nmake -f makefile.vc BUILD=debug UNICODE=0 SHARED=0 RUNTIME_LIBS=static
nmake -f makefile.vc BUILD=release UNICODE=0 SHARED=0 RUNTIME_LIBS=static
nmake -f makefile.vc BUILD=release UNICODE=1 SHARED=0 RUNTIME_LIBS=static


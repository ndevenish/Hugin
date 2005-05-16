rem build wxwindows
rem

@echo "In order to use the msvc project files, please read",
@echo "windows_DOT_NET_compile.txt"
@echo ""

copy ../src/include/config_msvc.h ../src/include/config.h

cd ../../libs/wxWidgets/build/msw

nmake -f makefile.vc BUILD=debug UNICODE=0 SHARED=0 RUNTIME_LIBS=static
nmake -f makefile.vc BUILD=release UNICODE=0 SHARED=0 RUNTIME_LIBS=static
nmake -f makefile.vc BUILD=release UNICODE=1 SHARED=0 RUNTIME_LIBS=static

cd ../../../../hugin/utils

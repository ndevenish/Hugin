# ------------------
#     wxMac 2.9
# ------------------
# $Id: wxmac29.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007-2008, Ippei Ukai

# 2009-12-04.0 Remove unneeded arguments to make and make install; made make single threaded


# prepare

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
#               Works Intel: 10.5, 10.6 & Powerpc 10.4, 10.5
# 20100624.0 hvdw More robust error checking on compilation
# 20120415.0 hvdw upgrade to 2.8.12
# 20120526.0 hvdw upgrade to 2.9.3 and rename to wxmac29.sh
# 20121010.0 hvdw upgrade to 2.9.4
# 20130205.0 hvdw temporarily downgrade to 2.9.3 again
# -------------------------------

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


uname_release=$(uname -r)
uname_arch=$(uname -p)
[ $uname_arch = powerpc ] && uname_arch="ppc"
os_dotvsn=${uname_release%%.*}
os_dotvsn=$(($os_dotvsn - 4))
os_sdkvsn=10.$os_dotvsn

NATIVE_SDKDIR="/Developer/SDKs/MacOSX$os_sdkvsn.sdk"
NATIVE_OSVERSION="10.$os_dotvsn"
NATIVE_ARCH=$uname_arch
NATIVE_OPTIMIZE=""

WX_MAJOR_VERSION="2.9"
WXVERSION="2.9.3"
WXVER_FULL="$WXVERSION.0.0" 

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# compile

# This will be the most simplified script.
# We absolutely only want to build the 64-bit version against Cocoa
ARCH="x86_64" 

 mkdir -p "osx-$ARCH-build";
 cd "osx-$ARCH-build";

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
 
 ARCHARGs=$(echo $ARCHARGs | sed 's/-ftree-vectorize//')
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip -I$REPOSITORYDIR/include" \
  OBJCFLAGS="-arch $ARCH" \
  OBJCXXFLAGS="-arch $ARCH" \
  LDFLAGS="-L$REPOSITORYDIR/lib -arch $ARCH -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  ../configure --prefix="$REPOSITORYDIR" --exec-prefix=$REPOSITORYDIR --disable-dependency-tracking \
    --host="$TARGET" --with-macosx-sdk=$MACSDKDIR --with-macosx-version-min=$OSVERSION \
    --enable-monolithic --enable-unicode --with-opengl --disable-graphics_ctx \
    --with-libiconv-prefix=$REPOSITORYDIR --with-libjpeg --with-libtiff --with-libpng --with-zlib \
    --with-cocoa --without-sdl --disable-sdltest \
    --enable-shared --disable-debug --enable-aui || fail "configure step for $ARCH";

# need to find out where setup.h was created. This seems to vary if building on powerpc and
# is different under 10.4 and 10.5
 whereIsSetup=$(find . -name setup.h -print | grep $ARCH)
 whereIsSetup=${whereIsSetup#./}

# echo '#ifndef wxMAC_USE_CORE_GRAPHICS'    >> $whereIsSetup
# echo ' #define wxMAC_USE_CORE_GRAPHICS 0' >> $whereIsSetup
# echo '#endif'                             >> $whereIsSetup
# echo ''                                   >> $whereIsSetup
#fi

 make clean;

#hack
 cp utils/wxrc/Makefile utils/wxrc/Makefile-copy;
 echo "all: " > utils/wxrc/Makefile;
 echo "" >> utils/wxrc/Makefile;
 echo "install: " >> utils/wxrc/Makefile;
#~hack

dylib_name="dylib1.10.5.o"
cp $NATIVE_SDKDIR/usr/lib/$dylib_name $REPOSITORYDIR/lib/

# Need to build single-threaded. libwx_osx_cocoau-2.9.dylib needs to be built before libwx_osx_cocoau_gl-2.9 to avoid a link error.
# This is only problematic for Intel builds, where jobs can be >1
 make --jobs=1 || fail "failed at make step of $ARCH";
 make install || fail "make install step of $ARCH";

 rm $REPOSITORYDIR/lib/$dylib_name;

 cd ../;


# We will no longer build universal but only 64bit, so merging of libs is not required anymore


if [ -f "$REPOSITORYDIR/lib/libwx_osx_cocoau-$WXVER_FULL.dylib" ]
then
 install_name_tool \
  -id "$REPOSITORYDIR/lib/libwx_osx_cocoau-$WXVERSION.dylib" \
  "$REPOSITORYDIR/lib/libwx_osx_cocoau-$WXVER_FULL.dylib";
 ln -sfn "libwx_osx_cocoau-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_osx_cocoau-$WXVERSION.dylib";
fi
if [ -f "$REPOSITORYDIR/lib/libwx_osx_cocoau_gl-$WXVER_FULL.dylib" ]
then
 install_name_tool \
  -id "$REPOSITORYDIR/lib/libwx_osx_cocoau_gl-$WXVERSION.dylib" \
  "$REPOSITORYDIR/lib/libwx_osx_cocoau_gl-$WXVER_FULL.dylib";
 for ARCH in $ARCHS
 do
  install_name_tool \
   -change "$REPOSITORYDIR/arch/$ARCH/lib/libwx_osx_cocoau-$WXVERSION.dylib" \
   "$REPOSITORYDIR/lib/libwx_osx_cocoau-$WXVERSION.dylib" \
   "$REPOSITORYDIR/lib/libwx_osx_cocoau_gl-$WXVER_FULL.dylib";
 done
 ln -sfn "libwx_osx_cocoau_gl-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_osx_cocoau_gl-$WXVERSION.dylib";
fi


# Make softlink to include folder
whereIsSetup=$(find $REPOSITORYDIR/lib/wx -name setup.h -print)
wxincludedir=`dirname $whereIsSetup`
wxincludedir=${wxincludedir%/*}
wxincludedironelevelup=${wxincludedir%/*}
ln -s $wxincludedir $wxincludedironelevelup/osx_cocoa-unicode-$WX_MAJOR_VERSION

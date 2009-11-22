# ------------------
#     openexr
# ------------------
# $Id: openexr.sh 2004 2007-05-11 00:17:50Z ippei $
# Copyright (c) 2007, Ippei Ukai


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
# ppcTARGET="powerpc-apple-darwin7" \
# i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  ppc64ONLYARG="-mcpu=G5 -mtune=G5 -ftree-vectorize" \
#  OTHERARGs="";



EXRVER_M="6"
EXRVER_FULL="$EXRVER_M.0.0"

NATIVE_LIBHALF_DIR="$REPOSITORYDIR/lib"

uname_release=$(uname -r)
uname_arch=$(uname -p)
os_dotvsn=${uname_release%%.*}
os_dotvsn=$(($os_dotvsn - 4))
NATIVE_SDKDIR="/Developer/SDKs/MacOSX10.$os_dotvsn.sdk"
NATIVE_OSVERSION="10.$os_dotvsn"
NATIVE_ARCH=$uname_arch
NATIVE_OPTIMIZE=""

# init

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";

NATIVE_CXXFLAGS="-isysroot $NATIVE_SDK -arch $NATIVE_ARCH $NATIVE_OPTIMIZE \
	-mmacos-version-min=$NATIVE_OSVERSION -D_THREAD_SAFE -O2 -dead_strip";

g++ -DHAVE_CONFIG_H -I./IlmImf -I./config \
	-I$REPOSITORYDIR/include/OpenEXR -D_THREAD_SAFE \
	-I. -I./config  -I$REPOSITORYDIR/include \
	-I/usr/include -arch $NATIVE_ARCH $NATIVE_OPTIMIZE -ftree-vectorize \
	-mmacosx-version-min=$NATIVE_OSVERSION -O2 -dead_strip  -L"$NATIVE_LIBHALF_DIR" -lHalf \
	-o "./IlmImf/b44ExpLogTable-native" ./IlmImf/b44ExpLogTable.cpp

if [ -f "./IlmImf/b44ExpLogTable-native" ] ; then
  echo "Created b44ExpLogTable-native"
else
  echo " Error Failed to create b44ExpLogTable-native"
  exit 1
fi

if [ -f "./IlmImf/Makefile.in-original" ]; then
  echo "original already exists!";
else
  mv "./IlmImf/Makefile.in" "./IlmImf/Makefile.in-original"
fi
sed -e 's/\.\/b44ExpLogTable/\.\/b44ExpLogTable-native/' \
    "./IlmImf/Makefile.in-original" > "./IlmImf/Makefile.in"

# compile

for ARCH in $ARCHS
do

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 TARGET=""
 ARCHARGs=""
 MACSDKDIR=""
 CC=""
 CXX=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  TARGET="$i386TARGET"
  MACSDKDIR="$i386MACSDKDIR"
  ARCHARGs="$i386ONLYARG"
  CC="$i386CC";
  CXX="$i386CXX";
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET="$ppcTARGET"
  MACSDKDIR="$ppcMACSDKDIR"
  ARCHARGs="$ppcONLYARG"
  CC="$ppcCC";
  CXX="$ppcCXX";
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET="$ppc64TARGET"
  MACSDKDIR="$ppc64MACSDKDIR"
  ARCHARGs="$ppc64ONLYARG"
  CC="$ppc64CC";
  CXX="$ppc64CXX";
 elif [ $ARCH = "x86_64" ]
 then
  TARGET="$x64TARGET"
  MACSDKDIR="$x64MACSDKDIR"
  ARCHARGs="$x64ONLYARG"
  CC="$x64CC";
  CXX="$x64CXX";
 fi

  # Configure is looking for a specific version of crt1.o based on what the compiler was built for
  # This library isn't in the search path, so copy it to lib
  crt1obj=lib/crt1.$NATIVE_OSVERSION.o 
  [ -f $REPOSITORYDIR/$crt1obj ] || cp $NATIVE_SDK/usr/$crt1obj $REPOSITORYDIR/$crt1obj ;
  # File exists for 10.5 and 10.6. 10.4 is a problem
  [ -f $REPOSITORYDIR/$crt1obj ] || exit 1 ;

  # Patch configure to eliminate the -Wno-long-double
  mv "configure" "configure-bk"
  sed 's/-Wno-long-double//g' "configure-bk" > "configure"
  chmod +x configure

  env \
    CC="$CC" CXX="$CXX" \
    CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
    CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
    CPPFLAGS="-I$REPOSITORYDIR/include" \
    LDFLAGS="-L$REPOSITORYDIR/lib -isysroot $MACSDKDIR -arch $ARCH -dead_strip -prebind" \
    NEXT_ROOT="$MACSDKDIR" \
    PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig" \
    ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
      --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
      --enable-shared --enable-static;

  [ -f "libtool-bk" ] || mv "libtool" "libtool-bk"; # just move it once, fix it many times
  sed -e "s#-dynamiclib#-dynamiclib -arch $ARCH -isysroot $MACSDKDIR#g" "libtool-bk" > "libtool";
  chmod +x libtool;

  [ -f $REPOSITORYDIR/$crt1obj ] && rm  $REPOSITORYDIR/$crt1obj;
  make clean;
  make $OTHERMAKEARGs all;
  make install;

done


# merge

for liba in lib/libIlmImf.a lib/libIlmImf.$EXRVER_FULL.dylib
do

 if [ $NUMARCH -eq 1 ] ; then
   mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
   #Power programming: if filename ends in "a" then ...
   [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";
   continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
   LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

  lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
  #Power programming: if filename ends in "a" then ...
  [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";

done

if [ -f "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_FULL.dylib" ] ; then
  install_name_tool -id "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_M.dylib" "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_FULL.dylib";
  ln -sfn "libIlmImf.$EXRVER_FULL.dylib" "$REPOSITORYDIR/lib/libIlmImf.$EXRVER_M.dylib";
  ln -sfn "libIlmImf.$EXRVER_FULL.dylib" "$REPOSITORYDIR/lib/libIlmImf.dylib";
fi

#pkgconfig

for ARCH in $ARCHS
do
  mkdir -p $REPOSITORYDIR/lib/pkgconfig
  sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' $REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/OpenEXR.pc > $REPOSITORYDIR/lib/pkgconfig/OpenEXR.pc
  break;
done

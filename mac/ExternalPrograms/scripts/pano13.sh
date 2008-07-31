# ------------------
#     pano13
# ------------------
# $Id: pano13.sh 1904 2007-02-05 00:10:54Z ippei $
# Copyright (c) 2007, Ippei Ukai

# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  ppc64ONLYARG="-mcpu=G5 -mtune=G5 -ftree-vectorize" \
#  OTHERARGs="";
  
# init

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# compile

for ARCH in $ARCHS
do

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  TARGET=$i386TARGET
  MACSDKDIR=$i386MACSDKDIR
  ARCHARGs="$i386ONLYARG"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET=$ppcTARGET
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET=$ppc64TARGET
  MACSDKDIR=$ppc64MACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
 elif [ $ARCH = "x86_64" ]
 then
  TARGET=$x64TARGET
  MACSDKDIR=$x64MACSDKDIR
  ARCHARGs="$x64ONLYARG"
 fi
 
 env CFLAGS="-gfull -isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-gfull -isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --without-java \
  --with-zlib=/usr \
  --with-png=$REPOSITORYDIR \
  --with-jpeg=$REPOSITORYDIR \
  --with-tiff=$REPOSITORYDIR \
  --enable-shared --enable-static;

 #Stupid libtool...
 mv "libtool" "libtool-bk";
 sed -e "s/-dynamiclib/-dynamiclib -arch $ARCH -isysroot $(echo $MACSDKDIR | sed 's/\//\\\//g')/g" "libtool-bk" > "libtool";
 
 make clean;
 make install;

done


# merge libpano13

GENERATED_DYLIB_NAME="libpano13.0.0.0.dylib";
GENERATED_DYLIB_INSTALL_NAME="libpano13.0.dylib";

for liba in lib/libpano13.a lib/$GENERATED_DYLIB_NAME
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
  if [[ $liba == *.a ]]
  then 
   ranlib "$REPOSITORYDIR/$liba";
  fi
  continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 if [[ $liba == *.a ]]
 then 
  ranlib "$REPOSITORYDIR/$liba";
 fi

done

mv $REPOSITORYDIR/lib/$GENERATED_DYLIB_NAME $REPOSITORYDIR/lib/libpano13.dylib;

for libname in pano13
do
 if [ -f "$REPOSITORYDIR/lib/lib$libname.dylib" ]
 then
  install_name_tool -id "$REPOSITORYDIR/lib/lib$libname.dylib" "$REPOSITORYDIR/lib/lib$libname.dylib";
 fi
done


# merge execs

for program in bin/panoinfo bin/PTblender bin/PTcrop bin/PTinfo bin/PTmasker bin/PTmender bin/PToptimizer bin/PTroller bin/PTtiff2psd bin/PTtiffdump bin/PTuncrop
do

 LIPOARGs=""

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
  install_name_tool \
    -change "$REPOSITORYDIR/arch/$ARCHS/lib/$GENERATED_DYLIB_INSTALL_NAME" "$REPOSITORYDIR/lib/libpano13.dylib" \
    "$REPOSITORYDIR/$program";
  continue
 fi

 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";
 
 for ARCH in $ARCHS
 do
  install_name_tool \
    -change "$REPOSITORYDIR/arch/$ARCH/lib/$GENERATED_DYLIB_INSTALL_NAME" "$REPOSITORYDIR/lib/libpano13.dylib" \
    "$REPOSITORYDIR/$program";
 done

done



# ------------------
#   panomatic 
# ------------------
# $Id: enblend3.sh 1908 2007-02-05 14:59:45Z ippei $
# Copyright (c) 2007, Ippei Ukai
# Script skeleton Ippei Ukai
# panomatic config Harry van der Wolf

# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin7" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
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

 OTHERARGs="-fast -ffast-math"

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  TARGET=$i386TARGET
  MACSDKDIR=$i386MACSDKDIR
  ARCHARGs="$i386ONLYARG"
  OTHERCARGS="-mmacosx-version-min=10.4"
  OTHERLDARGS="-mmacosx-version-min=10.4"
  export CC=$I386CC;
  export CXX=$I386CXX;
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET=$ppcTARGET
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
  OTHERCARGS="-mmacosx-version-min=10.4"
  OTHERLDARGS="-mmacosx-version-min=10.4"
  export CC="$ppcCC -arch $ARCH";
  export CXX="$ppcCXX -arch $ARCH";
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET=$ppc64TARGET
  MACSDKDIR=$ppc64MACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
  OTHERCARGS=""
  OTHERLDARGS=""
  export CC=$ppc64CC;
  export CXX=$ppc64CXX;
 elif [ $ARCH = "x86_64" ]
 then
  TARGET=$x64TARGET
  MACSDKDIR=$x64MACSDKDIR
  ARCHARGs="$x64ONLYARG"
  OTHERCARGS="-mtune=nocona -mfpmath=sse -msse3 -m64"
  OTHERLDARGS=""
  export CC=$x64CC;
  export CXX=$x64CXX;
 fi

 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $OTHERARGs $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $OTHERARGs $ARCHARGs $OTHERARGs -dead_strip" \
  CPPFLAGS="-isysroot $MACSDKDIR -arch $ARCH $OTHERARGs $OTHERCARGS -I$REPOSITORYDIR/include -I$REPOSITORYDIR/include/OpenEXR" \
  LDFLAGS="-isysroot $MACSDKDIR $OTHERLDARGS -arch $ARCH -L$REPOSITORYDIR/lib -dead_strip" \
  NEXT_ROOT="$MACSDKDIR" \
  ;


  rm config.status;

  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking  \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH  \
  --with-boost=$REPOSITORYDIR/include;

 make clean;
 make -j 4;
 make  install;

done


# merge execs

for program in bin/panomatic
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCH/$program" "$REPOSITORYDIR/$program";
  strip "$REPOSITORYDIR/$program";
  continue
 fi

 LIPOARGs=""

 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";

 strip "$REPOSITORYDIR/$program";

done

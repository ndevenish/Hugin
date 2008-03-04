# --------------------
#   autopano-sift-C-pano13
# --------------------
# $Id: autopano-sift-C.sh 1905 2007-02-05 00:11:26Z ippei $
# script skeleton by Ippei Ukai
# cmake part Harry van der Wolf

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

 mkdir -p $ARCH;
 cd $ARCH;
 rm CMakeCache.txt;

export CMAKE_INCLUDE_PATH="$REPOSITORYDIR/include"
export CMAKE_LIBRARY_PATH="$REPOSITORYDIR/lib"

cmake  \
  -DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR" \
  -DCMAKE_OSX_ARCHITECTURES="$TARGET" \
  -DCMAKE_OSX_SYSROOT="$MACSDKDIR"\
  -DPANO13_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DPANO13_LIBRARIES="$REPOSITORYDIR/lib/libpano13.dylib" \
  -DCMAKE_C_FLAGS="-arch $ARCH -DHAS_PANO13 -O2 -dead_strip" \
  -DCMAKE_CXX_FLAGS="-arch $ARCH -DHAS_PANO13 -O2 -dead_strip" \
  ..;


  make;
  cp ./autopano ./generatekeys ./APSCpp/autopano-sift-c $REPOSITORYDIR/arch/$ARCH/bin;
  cd ..;

done


# merge execs

for program in bin/autopano bin/generatekeys bin/autopano-sift-c
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
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

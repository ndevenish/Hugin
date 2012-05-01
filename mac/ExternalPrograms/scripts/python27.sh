# ------------------
#      python2.7
# ------------------
# $Id:  $
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

# -------------------------------
# 20120418.0 hvdw build python as part of Hugin
# -------------------------------

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


let NUMARCH="0"

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";

# compile
# For python we have a completely different approach


 mkdir -p "$REPOSITORYDIR/bin";
 mkdir -p "$REPOSITORYDIR/lib";
 mkdir -p "$REPOSITORYDIR/include";

 ARCHARGs=""
 MACSDKDIR=""

#Use the settings of the x86_64 build
TARGET=$x64TARGET
MACSDKDIR=$x64MACSDKDIR
ARCHARGs="$x64ONLYARG"
OSVERSION="$x64OSVERSION"
CC=$x64CC
CXX=$x64CXX

 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch i386 -arch x86_64 $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch i386 -arch x86_64 $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
#  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
#   --exec-prefix=$REPOSITORYDIR/ --enable-framework=$REPOSITORYDIR \
#   --enable-toolbox-glue --enable-ipv6 --enable-unicode \
#   --with-universal-archs="intel" --enable-universalsdk=$MACSDKDIR \
#   --with-cxx-main=$CXX \
#   || fail "configure step for python 2.7 multi arch";

  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
   --exec-prefix=$REPOSITORYDIR/ --enable-shared \
   --enable-toolbox-glue --enable-ipv6 --enable-unicode \
   --with-universal-archs="intel" --enable-universalsdk=$MACSDKDIR \
   --with-cxx-main=$CXX \
   || fail "configure step for python 2.7 multi arch";

 make clean;
 make || fail "failed at make step of python 2.7 multi arch";
 #make install || fail "make install step of python 2.7 multi arch";

done

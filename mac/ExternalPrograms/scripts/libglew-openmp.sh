# ------------------
#     libglew
# ------------------
# $Id: libglew.sh 1908 2007-02-05 14:59:45Z ippei $
# Copyright (c) 2007, Ippei Ukai

# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  ppc64ONLYARG="-mcpu=G5 -mtune=G5 -ftree-vectorize" \
#  OTHERARGs="";

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100110.0 sg Script enhanced to copy dynamic lib also
# 20100624.0 hvdw More robust error checking on compilation
# 20120414.0 hvdw updated to 1.7
# 20120428.0 hvdw use gcc 4.6 for x86_64 for openmp compatibility on lion an up
# -------------------------------

GLEW_MAJOR=1
GLEW_MINOR=7
GLEW_REV=0

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


# init
ORGPATH=$PATH

# patch 1.7 for gcc 4.6
cp config/Makefile.darwin config/Makefile.darwin.org
cp config/Makefile.darwin-x86_64 config/Makefile.darwin-x86_64.org
sed 's/-no-cpp-precomp//' config/Makefile.darwin.org > config/Makefile.darwin
sed 's/-no-cpp-precomp//' config/Makefile.darwin-x86_64.org > config/Makefile.darwin-x86_64


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

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
   TARGET=$i386TARGET
   MACSDKDIR=$i386MACSDKDIR
   ARCHARGs="$i386ONLYARG"
   CC=$i386CC
   CXX=$i386CXX
   myPATH=$ORGPATH
   ARCHFLAG="-m32"
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
#   CC=$x64CC
#   CXX=$x64CXX
   CC="gcc-4.6"
   CXX="g++-4.6"
   ARCHFLAG="-m64"
   myPATH=/usr/local/bin:$PATH
 fi

 env PATH=$myPATH;
 make clean;
 make install \
  GLEW_DEST="$REPOSITORYDIR/arch/$ARCH" \
  CC="$CC -isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs -O3 -dead_strip" \
  CXX="$CC -isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs -O3 -dead_strip" \
  LD="$CC -isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs -O3" \
  || fail "failed at make step of $ARCH";

done


# merge libs

for liba in lib/libGLEW.a lib/libGLEW.$GLEW_MAJOR.$GLEW_MINOR.$GLEW_REV.dylib
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$liba ] ; then
		 echo "Moving arch/$ARCHS/$liba to lib/$liba"
  	 mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
	   #Power programming: if filename ends in "a" then ...
	   [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";
  	 continue
	 else
		 echo "Program arch/$ARCHS/$liba not found. Aborting build";
		 exit 1;
	 fi
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
	if [ -f $REPOSITORYDIR/arch/$ARCH/$liba ] ; then
		echo "Adding arch/$ARCH/$liba to bundle"
		LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
	else
		echo "File arch/$ARCH/$liba was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";

done

if [ -f "$REPOSITORYDIR/lib/libGLEW.$GLEW_MAJOR.$GLEW_MINOR.$GLEW_REV.dylib" ] ; then
  install_name_tool \
    -id "$REPOSITORYDIR/lib/libGLEW.$GLEW_MAJOR.$GLEW_MINOR.$GLEW_REV.dylib" \
    "$REPOSITORYDIR/lib/libGLEW.$GLEW_MAJOR.$GLEW_MINOR.$GLEW_REV.dylib";
	  ln -sfn "libGLEW.$GLEW_MAJOR.$GLEW_MINOR.$GLEW_REV.dylib" "$REPOSITORYDIR/lib/libGLEW.$GLEW_MAJOR.$GLEW_MINOR.dylib";
	  ln -sfn "libGLEW.$GLEW_MAJOR.$GLEW_MINOR.$GLEW_REV.dylib" "$REPOSITORYDIR/lib/libGLEW.dylib";
fi


# install includes

cp -R include/GL $REPOSITORYDIR/include/;

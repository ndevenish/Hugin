# --------------------
#   autopano-sift-C
# --------------------
# $Id: autopano-sift-C.sh 1905 2007-02-05 00:11:26Z ippei $

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
# 20091206.0 sg Script tested and works with svn-4750
# 20091208.0 sg Replace --jobs=$PROCESSNUM (not exported) with $OTHERMAKEARGs
# -------------------------------

# init

let NUMARCH="0"

# remove 64-bit archs from ARCHS
ARCHS_TMP=$ARCHS
ARCHS=""
for ARCH in $ARCHS_TMP
do
 if [ $ARCH = "i386" -o $ARCH = "i686" -o $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
   NUMARCH=$(($NUMARCH + 1))
   if [ "$ARCHS" = "" ] ; then
     ARCHS="$ARCH"
   else
     ARCHS="$ARCHS $ARCH"
   fi
 fi
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
   OSVERSION=$i386OSVERSION
   OPTIMIZE=$i386OPTIMIZE
   export CC=$i386CC
   export CXX=$i386CXX
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
   TARGET=$ppcTARGET
   MACSDKDIR=$ppcMACSDKDIR
   OSVERSION=$ppcOSVERSION
   OPTIMIZE=$ppcOPTIMIZE
   export CC=$ppcCC
   export CXX=$ppcCXX
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ] ; then
   TARGET=$ppc64TARGET
   MACSDKDIR=$ppc64MACSDKDIR
   OSVERSION=$ppc64OSVERSION
   OPTIMIZE=$ppc64OPTIMIZE
   export CC=$ppc64CC
   export CXX=$ppc64CXX
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   OSVERSION=$x64OSVERSION
   OPTIMIZE=$x64OPTIMIZE
   export CC=$x64CC
   export CXX=$x64CXX
 fi


 # Why do I have to go through this much work!?
 # Some things like library names are implicit for reasons.
 # I'm not fond of software that asks to make it explicit only to put it back implicit.
 # And what's wrong with just having autoconf's good old "--prefix=" behaviour?
 # Stupid CMake...
 
 if [ -f "$REPOSITORYDIR/lib/libjpeg.dylib" ] ; then
  JPEG_EXT="dylib"
 else
  JPEG_EXT="a"
 fi
 
 if [ -f "$REPOSITORYDIR/lib/libpng.dylib" ] ; then
  PNG_EXT="dylib"
 else
  PNG_EXT="a"
 fi
 
 if [ -f "$REPOSITORYDIR/lib/libtiff.dylib" ] ; then
  TIFF_EXT="dylib"
 else
  TIFF_EXT="a"
 fi
 
 if [ -f "$REPOSITORYDIR/lib/libpano13.dylib" ] ; then
  PANO13_EXT="dylib"
 else
  PANO13_EXT="a"
 fi
 
 rm CMakeCache.txt;
 
 cmake \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL="ON" \
  -DCMAKE_INSTALL_PREFIX:PATH="$REPOSITORYDIR/arch/$ARCH" \
  -DCMAKE_BUILD_TYPE:STRING="Release" \
  -DCMAKE_C_FLAGS_RELEASE:STRING="-arch $ARCH -mmacosx-version-min=$OSVERSION -isysroot $MACSDKDIR -DNDEBUG -O2 $OPTIMIZE" \
  -DCMAKE_CXX_FLAGS_RELEASE:STRING="-arch $ARCH -mmacosx-version-min=$OSVERSION -isysroot $MACSDKDIR -DNDEBUG -O2 $OPTIMIZE" \
  -DPKGCONFIG_EXECUTABLE="" \
  -DJPEG_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DJPEG_LIBRARIES="$REPOSITORYDIR/lib/libjpeg.$JPEG_EXT" \
  -DPNG_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DPNG_LIBRARIES="$REPOSITORYDIR/lib/libpng.$PNG_EXT" \
  -DTIFF_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DTIFF_LIBRARIES="$REPOSITORYDIR/lib/libtiff.$TIFF_EXT" \
  -DPANO13_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DPANO13_LIBRARIES="$REPOSITORYDIR/lib/libpano13.$PANO13_EXT" \
  -DLIBXML2_INCLUDE_DIR="/usr/include/libxml2" \
  -DLIBXML2_LIBRARIES="/usr/lib/libxml2.dylib";

 make clean;
 make all $OTHERMAKEARGs;
 make install;
 
done


# merge execs

for program in bin/autopano bin/generatekeys bin/autopano-sift-c
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$program ] ; then
		 echo "Moving arch/$ARCHS/$program to $program"
  	 mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
  	 strip "$REPOSITORYDIR/$program";
  	 continue
	 else
		 echo "Program arch/$ARCHS/$program not found. Aborting build";
		 exit 1;
	 fi
 fi

 LIPOARGs=""

 for ARCH in $ARCHS
 do
  if [ -f $REPOSITORYDIR/arch/$ARCH/$program ] ; then
		echo "Adding arch/$ARCH/$program to bundle"
  	LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
	else
		echo "File arch/$ARCH/$program was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";
 strip "$REPOSITORYDIR/$program";

done

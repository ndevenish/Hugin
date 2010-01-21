# --------------------
#  panomatic-lib 
# --------------------
# $Id: panomatic-lib.sh 1905 2007-02-05 00:11:26Z ippei $

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
# 20100121.0 HvdW Initial version of panomatic-lib
# This is the only script that need to be run after Hugin is built so that
# the huginvigraimpex.framework is available.
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
 
 mkdir -p $ARCH
 cd $ARCH
 rm CMakeCache.txt;
 
 cmake \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL="ON" \
  -DCMAKE_INSTALL_PREFIX:PATH="$REPOSITORYDIR/arch/$ARCH" \
  -DCMAKE_BUILD_TYPE:STRING="Release" \
  -DCMAKE_C_FLAGS_RELEASE:STRING="-arch $ARCH -mmacosx-version-min=$OSVERSION -isysroot $MACSDKDIR -DNDEBUG -O2 $OPTIMIZE -L$REPOSITORYDIR/lib" \
  -DCMAKE_CXX_FLAGS_RELEASE:STRING="-arch $ARCH -mmacosx-version-min=$OSVERSION -isysroot $MACSDKDIR -DNDEBUG -O2 $OPTIMIZE -L$REPOSITORYDIR/lib" \
  -DCMAKE_CPP_FLAGS_RELEASE:STRING="-arch $ARCH -I$REPOSITORYDIR/include -I$REPOSITORYDIR/include/vigra -I$REPOSITORYDIR/include/OpenEXR" \
  -DCMAKE_LD_FLAGS_RELEASE:STRING="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
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
  -DLIBXML2_LIBRARIES="/usr/lib/libxml2.dylib" \
  -DVIGRA_INCLUDE_DIR="$REPOSITORYDIR/../../hugin/src/foreign/vigra" \
  -DVIGRA_LIBRARIES="$REPOSITORYDIR/../../hugin/mac/build/Release/HuginVigraImpex.framework/HuginVigraImpex" \
  .. ;

 

 make
 # panomatic does not have a make install yet
 # make install;

 # Set install_name for liblocalfeatures.dylib inside panomatic and keypoints correctly
 PWD=`pwd`
 for bina in panomatic/panomatic keypoints/keypoints
 do
   echo " Changing install name for: liblocalfeatures.dylib inside : $bina"
   install_name_tool -change "$PWD/localfeatures/liblocalfeatures.dylib" "$REPOSITORYDIR/lib/liblocalfeatures.dylib" $bina
 done
 
 # now copy files and rename panomatic to patfree-panomatic
 mkdir -p bin lib
 cp panomatic/panomatic bin/patfree-panomatic
 cp keypoints/keypoints bin/keypoints
 cp localfeatures/liblocalfeatures.dylib lib/liblocalfeatures.dylib
 cd .. 

done


# merge execs

for program in bin/patfree-panomatic bin/keypoints
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $ARCiH/$program ] ; then
		 echo "Moving $ARCH/$program to $program"
  	 mv "$ARCH/$program" "$REPOSITORYDIR/$program";
  	 strip "$REPOSITORYDIR/$program";
  	 continue
	 else
		 echo "Program $ARCH/$program not found. Aborting build";
		 exit 1;
	 fi
 fi

 LIPOARGs=""

 for ARCH in $ARCHS
 do
  if [ -f $ARCH/$program ] ; then
		echo "Adding $ARCH/$program to bundle"
  	LIPOARGs="$LIPOARGs $ARCH/$program"
	else
		echo "File $ARCH/$program was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";
 strip "$REPOSITORYDIR/$program";

done

# merge liblocalfeatures.dylib

for liba in lib/liblocalfeatures.dylib
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $ARCH/$liba ] ; then
		 echo "Moving $ARCHS/$liba to $liba"
  	 mv "$ARCHS/$liba" "$REPOSITORYDIR/$liba";
	   #Power programming: if filename ends in "a" then ...
	   [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";
  	 continue
	 else
		 echo "Program $ARCHS/$liba not found. Aborting build";
		 exit 1;
	 fi
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
	if [ -f $ARCH/$liba ] ; then
		echo "Adding $ARCH/$liba to bundle"
		LIPOARGs="$LIPOARGs $ARCH/$liba"
	else
		echo "File $ARCH/$liba was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 #Power programming: if filename ends in "a" then ...
 [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";

done


if [ -f "$REPOSITORYDIR/lib/liblocalfeatures.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/liblocalfeatures.dylib" "$REPOSITORYDIR/lib/liblocalfeatures.dylib"
fi

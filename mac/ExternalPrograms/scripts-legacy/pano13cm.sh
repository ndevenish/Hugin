# ------------------
#     pano13cm
# ------------------
# $Id: pano13cm.sh 1904 2007-02-05 00:10:54Z ippei $
# Copyright skeleton (c) 2007, Ippei Ukai
# script modifications and cmake, Harry van der Wolf

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
# 20091206.0 sg Script NOT tested but uses std boilerplate
# -------------------------------

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
  OSVERSION=$i386OSVERSION
  OPTIMIZE=$i386OPTIMIZE
  export CC=$i386CC;
  export CXX=$i386CXX;
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET=$ppcTARGET
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
  OSVERSION=$ppcOSVERSION
  OPTIMIZE=$ppcOPTIMIZE
  export CC=$ppcCC;
  export CXX=$ppcCXX;
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET=$ppc64TARGET
  MACSDKDIR=$ppc64MACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
  OSVERSION=$ppc64OSVERSION
  OPTIMIZE=$ppc64OPTIMIZE
  export CC=$ppc64CC;
  export CXX=$ppc64CXX;
 elif [ $ARCH = "x86_64" ]
 then
  TARGET=$x64TARGET
  MACSDKDIR=$x64MACSDKDIR
  ARCHARGs="$x64ONLYARG"
  OSVERSION=$x64OSVERSION
  OPTIMIZE=$x64OPTIMIZE
  export CC=$x64CC;
  export CXX=$x64CXX;
 fi
 
 mkdir $ARCH;
 cd $ARCH;
 rm CMakeCache.txt;

 cmake \
  -DCMAKE_VERBOSE_MAKEFILE:BOOL="ON" \
  -DCMAKE_INSTALL_PREFIX:PATH="$REPOSITORYDIR/arch/$ARCH" \
  -DCMAKE_BUILD_TYPE:STRING="Release" \
  -DCMAKE_C_FLAGS_RELEASE:STRING="-arch $ARCH -mmacosx-version-min=$OSVERSION -isysroot $MACSDKDIR -DNDEBUG -O3 $OPTIMIZE" \
  -DCMAKE_CXX_FLAGS_RELEASE:STRING="-arch $ARCH -mmacosx-version-min=$OSVERSION -isysroot $MACSDKDIR -DNDEBUG -O3 $OPTIMIZE" \
  -DJPEG_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DJPEG_LIBRARIES="$REPOSITORYDIR/lib/libjpeg.dylib" \
  -DPNG_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DPNG_LIBRARIES="$REPOSITORYDIR/lib/libpng.dylib" \
  -DTIFF_INCLUDE_DIR="$REPOSITORYDIR/include" \
  -DTIFF_LIBRARIES="$REPOSITORYDIR/lib/libtiff.dylib" \
  -DZLIB_INCLUDE_DIR="/usr/include" \
  -DZLIB_LIBRARIES="/usr/lib/libz.dylib" \
  ..;

 make;
 make install;
 
 cd ..
done


# merge libpano13

GENERATED_DYLIB_NAME="libpano13.2.0.0.dylib";
GENERATED_DYLIB_INSTALL_NAME="libpano13.2.dylib";

for liba in lib/libpano13.a lib/$GENERATED_DYLIB_NAME
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$liba ] ; then
		 echo "Moving arch/$ARCHS/$liba to $liba"
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
 #Power programming: if filename ends in "a" then ...
 [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";

done

mv $REPOSITORYDIR/lib/$GENERATED_DYLIB_NAME $REPOSITORYDIR/lib/libpano13.dylib;

for libname in pano13
do
 if [ -f "$REPOSITORYDIR/lib/lib$libname.dylib" ] ; then
   install_name_tool -id "$REPOSITORYDIR/lib/lib$libname.dylib" "$REPOSITORYDIR/lib/lib$libname.dylib";
 fi
done

# merge execs

for program in bin/panoinfo bin/PTblender bin/PTcrop bin/PTinfo bin/PTmasker bin/PTmender bin/PToptimizer bin/PTroller bin/PTtiff2psd bin/PTtiffdump bin/PTuncrop
do

  if [ $NUMARCH -eq 1 ] ; then
    mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
    install_name_tool \
      -change "$REPOSITORYDIR/arch/$ARCHS/lib/$GENERATED_DYLIB_INSTALL_NAME" "$REPOSITORYDIR/lib/libpano13.dylib" \
      "$REPOSITORYDIR/$program";
    continue
  fi

 LIPOARGs=""
 for ARCH in $ARCHS
 do
   LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
 done
 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";
 
 for ARCH in $ARCHS
 do
  # Why are we doing this?
  install_name_tool \
    -change "$REPOSITORYDIR/arch/$ARCH/lib/$GENERATED_DYLIB_INSTALL_NAME" "$REPOSITORYDIR/lib/libpano13.dylib" \
    "$REPOSITORYDIR/$program";
 done
done


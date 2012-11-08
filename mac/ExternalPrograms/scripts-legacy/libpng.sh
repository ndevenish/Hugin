# ------------------
#     libpng
# ------------------
# $Id: libpng.sh 1902 2007-02-04 22:27:47Z ippei $
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
# 20100121.0 sg Script updated for 1.2.40
# 201005xx.0 hvdw Adapted for 1.2.43
# 20100624.0 hvdw More robust error checking on compilation
# 20100831.0 hvdw upgrade to 1.2.44
# 20120422.0 hvdw upgrade to 1.5.10
# 20120427.0 hvdw use gcc 4.6 for x86_64 for openmp compatibility on lion an up
# 20120430.0 hvdw downgrade from 1.5.10 to 1.4.11 as enblend's vigra  can't work with 1.5.10 even after patching
# -------------------------------

#libraries created:
# libpng.3.1.2.42 <- (libpng.3, libpng)
# libpng12.12.1.2.42 <- (libpng12.12, libpng12)
# libpng12.a <- libpng.a
#PNGVER_M="15"
#PNGVER_FULL="$PNGVER_M.15"
PNGVER_M="14"
PNGVER_FULL="$PNGVER_M.14"

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}

ORGPATH=$PATH

let NUMARCH="0"
for i in $ARCHS ; do
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
   OSVERSION="$i386OSVERSION"
   CC=$i386CC
   CXX=$i386CXX
   myPATH=$ORGPATH
   ARCHFLAG="-m32"
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
   CC="gcc-4.6"
   CXX="g++-4.6"
   ARCHFLAG="-m64"
   myPATH=/usr/local/bin:$PATH
 fi

 # makefile.darwin
  includes hack for libpng bug #2009836
sed -e 's/-dynamiclib/-dynamiclib \$\(GCCLDFLAGS\)/g' \
    scripts/makefile.darwin > makefile;

 env PATH=$myPATH;

 make clean;
 make $OTHERMAKEARGs install-static install-shared \
  prefix="$REPOSITORYDIR" \
  ZLIBLIB="$MACSDKDIR/usr/lib" \
  ZLIBINC="$MACSDKDIR/usr/include" \
  CC="$CC" CXX="$CXX" \
  CFLAGS="-isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  OBJCFLAGS="$ARCHFLAG" \
  OBJCXXFLAGS="$ARCHFLAG" \
  LDFLAGS="-L$REPOSITORYDIR/lib -L. -L$ZLIBLIB -lz -mmacosx-version-min=$OSVERSION" \
  NEXT_ROOT="$MACSDKDIR" \
  LIBPATH="$REPOSITORYDIR/arch/$ARCH/lib" \
  BINPATH="$REPOSITORYDIR/arch/$ARCH/bin" \
  GCCLDFLAGS="-isysroot $MACSDKDIR $ARCHFLAG $ARCHARGs $OTHERARGs" \
   || fail "failed at make step of $ARCH";

  # This libpng.dylib is installed as libpng15.15..dylib, so we have to rename them
  # I assume this will be corrected in 1.5.11 or so
  mv -v $REPOSITORYDIR/arch/$ARCH/lib/libpng$PNGVER_FULL..dylib $REPOSITORYDIR/arch/$ARCH/lib/libpng$PNGVER_FULL.dylib
done


# merge libpng

for liba in lib/libpng$PNGVER_M.a lib/libpng$PNGVER_FULL.dylib 
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

if [ -f "$REPOSITORYDIR/lib/libpng$PNGVER_M.a" ] ; then
  ln -sfn libpng$PNGVER_M.a $REPOSITORYDIR/lib/libpng.a;
fi
if [ -f "$REPOSITORYDIR/lib/libpng$PNGVER_FULL.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libpng$PNGVER_FULL.dylib" "$REPOSITORYDIR/lib/libpng$PNGVER_FULL.dylib"
 ln -sfn libpng$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng.dylib;
 ln -sfn libpng$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng$PNGVER_M.dylib;
fi
#if [ -f "$REPOSITORYDIR/lib/libpng.$PNGVER_FULL.dylib" ] ; then
# install_name_tool -id "$REPOSITORYDIR/lib/libpng.$PNGVER_FULL.dylib" "$REPOSITORYDIR/lib/libpng.$PNGVER_FULL.dylib"
# ln -sfn libpng.$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng.$PNGVER_M.dylib;
# ln -sfn libpng.$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng.dylib;
#fi

#pkgconfig

for ARCH in $ARCHS
do
  mkdir -p $REPOSITORYDIR/lib/pkgconfig
  sed "s+arch/$ARCH/++" $REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/libpng$PNGVER_M.pc > $REPOSITORYDIR/lib/pkgconfig/libpng$PNGVER_M.pc
  break;
done

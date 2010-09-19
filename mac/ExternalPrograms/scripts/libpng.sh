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
# -------------------------------

#libraries created:
# libpng.3.1.2.42 <- (libpng.3, libpng)
# libpng12.12.1.2.42 <- (libpng12.12, libpng12)
# libpng12.a <- libpng.a
PNGVER_M="12"
PNGVER="1.2.44"
PNGVER_FULL="$PNGVER_M.1.2.44"

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


let NUMARCH="0"
for i in $ARCHS ; do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# patch

# pngconf.h
#if [ -f pngconf-bk.h ]
#then
# mv -f pngconf-bk.h pngconf.h
#fi
#cp pngconf.h pngconf-bk.h
#patch < ../scripts/pngconf_h.patch


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
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
   TARGET=$ppcTARGET
   MACSDKDIR=$ppcMACSDKDIR
   ARCHARGs="$ppcONLYARG"
   OSVERSION="$ppcOSVERSION"
   CC=$ppcCC
   CXX=$ppcCXX
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ] ; then
   TARGET=$ppc64TARGET
   MACSDKDIR=$ppc64MACSDKDIR
   ARCHARGs="$ppc64ONLYARG"
   OSVERSION="$ppc64OSVERSION"
   CC=$ppc64CC
   CXX=$ppc64CXX
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
 fi

 # makefile.darwin
#  includes hack for libpng bug #2009836
sed -e 's/-dynamiclib/-dynamiclib \$\(GCCLDFLAGS\)/g' \
    scripts/makefile.darwin > makefile;
#if [ -n $CC ]
#then 
# sed -e 's/-dynamiclib/-dynamiclib \$\(GCCLDFLAGS\)/g' \
#     -e "s/CC=.*/CC=$CC/" \
#     -e 's/compatibility_version \$(SONUM)/compatibility_version 1.2.0/g' \
#     -e 's/current_version \$(SONUM)/current_version \$(PNGMIN)/g' \
#     -e 's/compatibility_version %OLDSONUM%/compatibility_version 3.0.0/g' \
#     -e 's/current_version %OLDSONUM%/current_version 3.0.0/g' \
#     scripts/makefile.darwin > makefile;
#else
# sed -e 's/-dynamiclib/-dynamiclib \$\(GCCLDFLAGS\)/g' \
#     -e 's/CC=cc/CC=gcc/' \
#     -e 's/compatibility_version \$(SONUM)/compatibility_version 1.2.0/g' \
#     -e 's/current_version \$(SONUM)/current_version \$(PNGMIN)/g' \
#     -e 's/compatibility_version %OLDSONUM%/compatibility_version 3.0.0/g' \
#     -e 's/current_version %OLDSONUM%/current_version 3.0.0/g' \
#     scripts/makefile.darwin > makefile;
#fi

 make clean;
 make $OTHERMAKEARGs install-static install-shared \
  prefix="$REPOSITORYDIR" \
  ZLIBLIB="$MACSDKDIR/usr/lib" \
  ZLIBINC="$MACSDKDIR/usr/include" \
  CC="$CC" CXX="$CXX" \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  OBJCFLAGS="-arch $ARCH" \
  OBJCXXFLAGS="-arch $ARCH" \
  LDFLAGS="-L$REPOSITORYDIR/lib -L. -L$ZLIBLIB -lpng$PNGVER_M -lz -mmacosx-version-min=$OSVERSION" \
  NEXT_ROOT="$MACSDKDIR" \
  LIBPATH="$REPOSITORYDIR/arch/$ARCH/lib" \
  BINPATH="$REPOSITORYDIR/arch/$ARCH/bin" \
  GCCLDFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs" \
   || fail "failed at make step of $ARCH";

done

# merge libpng

for liba in lib/libpng$PNGVER_M.a lib/libpng$PNGVER_M.$PNGVER_FULL.dylib lib/libpng.3.$PNGVER.dylib
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
if [ -f "$REPOSITORYDIR/lib/libpng$PNGVER_M.$PNGVER_FULL.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libpng$PNGVER_M.$PNGVER_FULL.dylib" "$REPOSITORYDIR/lib/libpng$PNGVER_M.$PNGVER_FULL.dylib"
 ln -sfn libpng$PNGVER_M.$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng$PNGVER_M.$PNGVER_M.dylib;
 ln -sfn libpng$PNGVER_M.$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng$PNGVER_M.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libpng.$PNGVER_FULL.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libpng.$PNGVER_FULL.dylib" "$REPOSITORYDIR/lib/libpng.$PNGVER_FULL.dylib"
 ln -sfn libpng.$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng.$PNGVER_M.dylib;
 ln -sfn libpng.$PNGVER_FULL.dylib $REPOSITORYDIR/lib/libpng.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libpng.3.$PNGVER.dylib" ] ; then
 install_name_tool -id "$REPOSITORYDIR/lib/libpng.3.dylib" "$REPOSITORYDIR/lib/libpng.3.$PNGVER.dylib"
 ln -sfn libpng.3.$PNGVER.dylib $REPOSITORYDIR/lib/libpng.3.dylib;
 ln -sfn libpng.3.dylib $REPOSITORYDIR/lib/libpng.dylib;
fi

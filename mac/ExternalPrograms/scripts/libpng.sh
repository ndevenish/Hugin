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



PNGVER="1.2.35"


# init

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# patch

# makefile.darwin
#  includes hack for libpng bug #2009836
if [ $CC != "" ]
then 
 sed -e 's/-dynamiclib/-dynamiclib \$\(GCCLDFLAGS\)/g' \
     -e "s/CC=cc/CC=$CC/" \
     -e 's/compatibility_version \$(SONUM)/compatibility_version 1.2.0/g' \
     -e 's/current_version \$(SONUM)/current_version \$(PNGMIN)/g' \
     -e 's/compatibility_version %OLDSONUM%/compatibility_version 3.0.0/g' \
     -e 's/current_version %OLDSONUM%/current_version 3.0.0/g' \
     scripts/makefile.darwin > makefile;
else
 sed -e 's/-dynamiclib/-dynamiclib \$\(GCCLDFLAGS\)/g' \
     -e 's/CC=cc/CC=gcc/' \
     -e 's/compatibility_version \$(SONUM)/compatibility_version 1.2.0/g' \
     -e 's/current_version \$(SONUM)/current_version \$(PNGMIN)/g' \
     -e 's/compatibility_version %OLDSONUM%/compatibility_version 3.0.0/g' \
     -e 's/current_version %OLDSONUM%/current_version 3.0.0/g' \
     scripts/makefile.darwin > makefile;
fi

# pngconf.h
if [ -f pngconf-bk.h ]
then
 mv -f pngconf-bk.h pngconf.h
fi
cp pngconf.h pngconf-bk.h
patch < $(dirname $0)/pngconf_h.patch


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

 make clean;
 make $OTHERMAKEARGs install-static install-shared \
  prefix="$REPOSITORYDIR" \
  ZLIBLIB="$MACSDKDIR/usr/lib" \
  ZLIBINC="$MACSDKDIR/usr/include" \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  LDFLAGS="-L$REPOSITORYDIR/lib -L. -L$ZLIBLIB -lpng12 -lz" \
  NEXT_ROOT="$MACSDKDIR" \
  LIBPATH="$REPOSITORYDIR/arch/$ARCH/lib" \
  BINPATH="$REPOSITORYDIR/arch/$ARCH/bin" \
  GCCLDFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs";

done


# merge libpng

for liba in lib/libpng12.a lib/libpng12.12.$PNGVER.dylib lib/libpng.3.$PNGVER.dylib
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


if [ -f "$REPOSITORYDIR/lib/libpng12.a" ]
then
 ln -sfn libpng12.a $REPOSITORYDIR/lib/libpng.a;
fi

if [ -f "$REPOSITORYDIR/lib/libpng12.12.$PNGVER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libpng12.1.dylib" "$REPOSITORYDIR/lib/libpng12.12.$PNGVER.dylib"
 ln -sfn libpng12.12.$PNGVER.dylib $REPOSITORYDIR/lib/libpng12.12.dylib;
 ln -sfn libpng12.12.dylib $REPOSITORYDIR/lib/libpng12.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libpng.3.$PNGVER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libpng.3.dylib" "$REPOSITORYDIR/lib/libpng.3.$PNGVER.dylib"
 ln -sfn libpng.3.$PNGVER.dylib $REPOSITORYDIR/lib/libpng.3.dylib;
 ln -sfn libpng.3.dylib $REPOSITORYDIR/lib/libpng.dylib;
fi

# ------------------
#     libjpeg
# ------------------
# $Id: libjpeg.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007, Ippei Ukai


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin7" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4 -mmacosx-version-min=10.3" \
#  i386TARGET="i386-apple-darwin8" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386ONLYARG="-march=prescott -mtune=pentium-m -ftree-vectorize -mmacosx-version-min=10.4" \
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

# update some of libtool stuff
cp /usr/share/libtool/config* ./;

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

 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include -I/usr/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -L/usr/lib -dead_strip" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --disable-shared --enable-static;

 make clean;
 make install-lib;

 # the old config-make stuff do not create shared library well. Best do it by hand.
 rm "libjpeg.62.0.0.dylib";
 gcc -isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -dead_strip \
  -dynamiclib -flat_namespace -undefined suppress \
  -lmx -shared-libgcc -current_version 62.0.0 -compatibility_version 62.0.0\
  -install_name "$REPOSITORYDIR/lib/libjpeg.62.dylib" -o libjpeg.62.0.0.dylib \
  jcomapi.o jutils.o jerror.o jmemmgr.o jmemnobs.o \
  jcapimin.o jcapistd.o jctrans.o jcparam.o jdatadst.o jcinit.o \
  jcmaster.o jcmarker.o jcmainct.o jcprepct.o jccoefct.o jccolor.o \
  jcsample.o jchuff.o jcphuff.o jcdctmgr.o jfdctfst.o jfdctflt.o \
  jfdctint.o \
  jdapimin.o jdapistd.o jdtrans.o jdatasrc.o jdmaster.o \
  jdinput.o jdmarker.o jdhuff.o jdphuff.o jdmainct.o jdcoefct.o \
  jdpostct.o jddctmgr.o jidctfst.o jidctflt.o jidctint.o jidctred.o \
  jdsample.o jdcolor.o jquant1.o jquant2.o jdmerge.o;
 install "libjpeg.62.0.0.dylib" "$REPOSITORYDIR/arch/$ARCH/lib";

done


# merge libjpeg

for liba in lib/libjpeg.a lib/libjpeg.62.0.0.dylib
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


if [ -f "$REPOSITORYDIR/lib/libjpeg.62.0.0.dylib" ]
then
 ln -sfn "libjpeg.62.0.0.dylib" "$REPOSITORYDIR/lib/libjpeg.62.dylib";
 ln -sfn "libjpeg.62.0.0.dylib" "$REPOSITORYDIR/lib/libjpeg.dylib";
fi

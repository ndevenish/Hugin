# ------------------
#     openexr
# ------------------
# $Id: openexr.sh 2004 2007-05-11 00:17:50Z ippei $
# Copyright (c) 2007, Ippei Ukai


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
# ppcTARGET="powerpc-apple-darwin7" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
# i386TARGET="i386-apple-darwin8" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
# OTHERARGs="";


# init

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


g++ "./Half/eLut.cpp" -o "./Half/eLut-native"
g++ "./Half/toFloat.cpp" -o "./Half/toFloat-native"
if [ -f "./Half/Makefile.in-original" ]
then
 echo "original already exists!";
else
 mv "./Half/Makefile.in" "./Half/Makefile.in-original"
fi
sed -e 's/\.\/eLut/\.\/eLut-native/' \
    -e 's/\.\/toFloat/\.\/toFloat-native/' \
    "./Half/Makefile.in-original" > "./Half/Makefile.in"


# compile

ILMVER_M="6"
ILMVER_FULL="$ILMVER_M.0.0"

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
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -dead_strip" \
  NEXT_ROOT="$MACSDKDIR" \
  PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --enable-shared --enable-static;

 mv "libtool" "libtool-bk";
 sed -e "s/-dynamiclib/-dynamiclib -arch $ARCH -isysroot $(echo $MACSDKDIR | sed 's/\//\\\//g')/g" "libtool-bk" > "libtool";

 #hack for apple-gcc 4.2
 if [ $CC != "" ]
 then
  for dir in Half Iex IlmThread Imath
  do
   mv $dir/Makefile $dir/Makefile.bk
   sed 's/-Wno-long-double//g' $dir/Makefile.bk > $dir/Makefile
  done
 fi

 make clean;
 make $OTHERMAKEARGs all;
 make install;

done


# merge

LIBNAMES="IlmThread Imath Iex Half"

for liba in $(for libname in $LIBNAMES; do echo "lib/lib$libname.a lib/lib$libname.$ILMVER_FULL.dylib "; done)
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


for libname in $LIBNAMES
do
 if [ -f "$REPOSITORYDIR/lib/lib$libname.$ILMVER_FULL.dylib" ]
 then
  install_name_tool -id "$REPOSITORYDIR/lib/lib$libname.$ILMVER_M.dylib" "$REPOSITORYDIR/lib/lib$libname.$ILMVER_FULL.dylib";
  
  for ARCH in $ARCHS
  do
   for libname_two in $LIBNAMES
   do
    install_name_tool \
     -change "$REPOSITORYDIR/arch/$ARCH/lib/lib$libname_two.$ILMVER_M.dylib" "$REPOSITORYDIR/lib/lib$libname_two.$ILMVER_M.dylib" \
     "$REPOSITORYDIR/lib/lib$libname.$ILMVER_FULL.dylib";
   done
  done

  ln -sfn "lib$libname.$ILMVER_FULL.dylib" "$REPOSITORYDIR/lib/lib$libname.$ILMVER_M.dylib";
  ln -sfn "lib$libname.$ILMVER_FULL.dylib" "$REPOSITORYDIR/lib/lib$libname.dylib";
 fi
done


#pkgconfig

for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/lib/pkgconfig";
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' "$REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/IlmBase.pc" > "$REPOSITORYDIR/lib/pkgconfig/IlmBase.pc";
 break;
done

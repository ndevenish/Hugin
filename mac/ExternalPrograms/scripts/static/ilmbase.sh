# ------------------
#     openexr
# ------------------
# $Id: openexr.sh 2004 2007-05-11 00:17:50Z ippei $
# Copyright (c) 2007, Ippei Ukai


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
# ppcTARGET="powerpc-apple-darwin7" \
# i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
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
 
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --disable-shared;

 
 	 [ -f "libtool-bk" ] || mv "libtool" "libtool-bk"; # just move it once, fix it many times
	 sed -e "s#-dynamiclib#-dynamiclib -arch $ARCH -isysroot $MACSDKDIR#g" "libtool-bk" > "libtool";
	 chmod +x libtool

	 #hack for apple-gcc 4.2
	 # Can $CC ever be null?
	# if [ $CC != "" ]
	# then
	  for dir in Half Iex IlmThread Imath
	  do
	   mv $dir/Makefile $dir/Makefile.bk
	   sed 's/-Wno-long-double//g' $dir/Makefile.bk > $dir/Makefile
	  done
	# fi

 make clean;
 make $OTHERMAKEARGs all;
 make install;

done


# merge

for liba in lib/libIlmThread.a lib/libImath.a lib/libIex.a lib/libHalf.a
do

 if [ $NUMARCH -eq 1 ] ; then
   mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
   ranlib "$REPOSITORYDIR/$liba";
   continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
   LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 ranlib "$REPOSITORYDIR/$liba";

done


#pkgconfig
for ARCH in $ARCHS
do
  mkdir -p $REPOSITORYDIR/lib/pkgconfig
  sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' \
    $REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/IlmBase.pc > $REPOSITORYDIR/lib/pkgconfig/IlmBase.pc
  break;
done


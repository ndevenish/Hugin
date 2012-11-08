# ------------------
#     libexiv2
# ------------------
# $Id: $
# Copyright (c) 2008, Ippei Ukai


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  OTHERARGs="";

# -------------------------------
# 20091206.0 sg Script NOT tested but uses std boilerplate
# 20100111.0 sg Script tested for building dylib
# 20100121.0 sg Script updated for 0.19
# 20100624.0 hvdw More robust error checking on compilation
# 20120414.0 hvdw update to 0.22
# -------------------------------

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


EXIV2VER="11"

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
#   ARCHARGs="$i386ONLYARG"
   # exiv2 not yet fully compliant with openmp on 32bits
   ARCHARGs="-march=prescott -mtune=pentium-m -ftree-vectorize -mmacosx-version-min=10.5"
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

 echo "Now building for architecture $ARCH"

 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -arch $ARCH -mmacosx-version-min=$OSVERSION -dead_strip  -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --build="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --enable-shared --with-libiconv-prefix=$REPOSITORYDIR --with-libintl-prefix=$REPOSITORYDIR \
  --with-expat=$REPOSITORYDIR --with-zlib=$MACSDKDIR/usr --disable-lensdata --disable-commercial \
  --enable-static  || fail "configure step for $ARCH";

 [ -f "libtool-bk" ] && rm libtool-bk; 
 mv "libtool" "libtool-bk"; 
 sed -e "s#-dynamiclib#-shared-libgcc -dynamiclib -arch $ARCH -isysroot $MACSDKDIR#g" \
     -e 's/-all_load//g' "libtool-bk" > "libtool";
 chmod +x libtool

 make clean;

 #cd xmpsdk/src;
 #make xmpsdk
 #cd ../../;

 #cd src;
 make $OTHERMAKEARGs || fail "failed at make step of $ARCH";
 make install || fail "make install step of $ARCH";
 #cd ../;
done


# merge libexiv2

for liba in lib/libexiv2.a lib/libexiv2.$EXIV2VER.dylib
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


if [ -f "$REPOSITORYDIR/lib/libexiv2.$EXIV2VER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libexiv2.$EXIV2VER.dylib" "$REPOSITORYDIR/lib/libexiv2.$EXIV2VER.dylib"
 ln -sfn libexiv2.$EXIV2VER.dylib $REPOSITORYDIR/lib/libexiv2.dylib;
fi

# merge execs

for program in bin/exiv2
do

 if [ $NUMARCH -eq 1 ] ; then
   if [ -f $REPOSITORYDIR/arch/$ARCHS/$program ] ; then
		 echo "Moving arch/$ARCHS/$program to $program"
  	 mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
  	 strip -x "$REPOSITORYDIR/$program";
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
 strip -x "$REPOSITORYDIR/$program";

done

# Last step for exiv2. exiv2 is linked during build against it's own libexiv2.dylib and therefore has an install_name
# based on the arch/$ARCH directory. We need to change that. Unfortunately we need to do it for every arch even 
# though it is only mentioned once for one of the arc/$ARCHs.

for ARCH in $ARCHS
do
  install_name_tool -change $REPOSITORYDIR/arch/$ARCH/lib/libexiv2.$EXIV2VER.dylib $REPOSITORYDIR/lib/libexiv2.$EXIV2VER.dylib $REPOSITORYDIR/bin/exiv2
done



#pkgconfig
for ARCH in $ARCHS
do
 mkdir -p $REPOSITORYDIR/lib/pkgconfig
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' $REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/exiv2.pc > $REPOSITORYDIR/lib/pkgconfig/exiv2.pc
 break;
done


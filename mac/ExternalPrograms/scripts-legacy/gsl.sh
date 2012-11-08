# ------------------
#     gsl 
# ------------------
# $Id: $
# Copyright (c) 2008, Ippei Ukai
# 2012, Harry van der Wolf


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
# 20120111.0 initial version of gnu science library
# Dependency for new enblend after GSOC 2011
# -------------------------------

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


GSLVER="0"

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
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -arch $ARCH -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --enable-shared --enable-static  || fail "configure step for $ARCH";

 make clean;

 make $OTHERMAKEARGs || fail "failed at make step of $ARCH";
 make install || fail "make install step of $ARCH";
done


# merge gsl libs

for liba in lib/libgsl.a lib/libgsl.$GSLVER.dylib lib/libgslcblas.a lib/libgslcblas.$GSLVER.dylib
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


if [ -f "$REPOSITORYDIR/lib/libgsl.$GSLVER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libgsl.$GSLVER.dylib" "$REPOSITORYDIR/lib/libgsl.$GSLVER.dylib"
 ln -sfn libgsl.$GSLVER.dylib $REPOSITORYDIR/lib/libgsl.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libgslcblas.$GSLVER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libgslcblas.$GSLVER.dylib" "$REPOSITORYDIR/lib/libgslcblas.$GSLVER.dylib"
 ln -sfn libgslcblas.$GSLVER.dylib $REPOSITORYDIR/lib/libgslcblas.dylib;
fi

# merge execs

for program in bin/gsl-randist bin/gsl-histogram
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

# Last step for gsl. gsl-histogram and gsl-randist are linked during build against their own dylibs and therefore have
# an install_name based on the arch/$ARCH directory. We need to change that. Unfortunately we need to do it for every 
# arch even though it is only mentioned once for one of the arc/$ARCHs.

for ARCH in $ARCHS
do
  install_name_tool -change $REPOSITORYDIR/arch/$ARCH/lib/libgsl.$GSLVER.dylib $REPOSITORYDIR/lib/libgslcblas.$GSLVER.dylib $REPOSITORYDIR/bin/gsl-histogram
  install_name_tool -change $REPOSITORYDIR/arch/$ARCH/lib/libgsl.$GSLVER.dylib $REPOSITORYDIR/lib/libgslcblas.$GSLVER.dylib $REPOSITORYDIR/bin/gsl-randist
done



##pkgconfig
#for ARCH in $ARCHS
#do
# mkdir -p $REPOSITORYDIR/lib/pkgconfig
# sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' $REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/gsl.pc > $REPOSITORYDIR/lib/pkgconfig/gsl.pc
# break;
#done


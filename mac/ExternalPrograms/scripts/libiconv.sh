# ------------------
#     libiconv
# ------------------
# $Id: libiconv.sh 1902 2008-01-02 22:27:47Z Harry $
# Copyright (c) 2007, Ippei Ukai
# script skeleton Copyright (c) 2007, Ippei Ukai
# iconv specifics 2010, Harry van der Wolf


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  OTHERARGs="";

# -------------------------------
# 20100117.0 HvdW Script tested
# 20100624.0 hvdw More robust error checking on compilation
# 20121010.0 hvdw update to 1.14 and cleanup
# -------------------------------

# init

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";

ICONVVER="2"
CHARSETVER="1"


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
 else [ $ARCH = "x86_64" ] ; 
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
  LDFLAGS="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --disable-nls --enable-extra-encodings \
  --without-libiconv-prefix --without-libintl-prefix \
  --enable-static --enable-shared  || fail "configure step of $ARCH";

 make clean
 make || fail "failed at make step of $ARCH"
 make $OTHERMAKEARGs install || fail "make install step of $ARCH"
done


# merge libiconv

for liba in lib/libiconv.a lib/libiconv.$ICONVVER.dylib lib/libcharset.a lib/libcharset.$CHARSETVER.dylib
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


if [ -f "$REPOSITORYDIR/lib/libiconv.$ICONVVER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libiconv.$ICONVVER.dylib" "$REPOSITORYDIR/lib/libiconv.$ICONVVER.dylib"
 ln -sfn libiconv.$ICONVVER.dylib $REPOSITORYDIR/lib/libiconv.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libcharset.$CHARSETVER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libcharset.$CHARSETVER.dylib" "$REPOSITORYDIR/lib/libcharset.$CHARSETVER.dylib"
 ln -sfn libcharset.$CHARSETVER.dylib $REPOSITORYDIR/lib/libcharset.dylib;
fi

# merge execs

for program in bin/iconv
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


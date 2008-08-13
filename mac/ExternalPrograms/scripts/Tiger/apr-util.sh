# ------------------
#     libaprutil
# ------------------
# Based on the works of (c) 2007, Ippei Ukai
# Created for Tiger by Harry van der Wolf 2008

# download location http://apr.apache.org/

# prepare

# export REPOSITORYDIR="/PATH2AVIDEMUX/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
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

MAIN_LIB_VER="1"
EXT_MAIN_LIB_VER="$MAIN_LIB_VER.0"
FULL_LIB_VER="$EXT_MAIN_LIB_VER.2.12"

OTHERMAKEARGs=""

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

 export PATH=/usr/bin:$REPOSITORYDIR/bin:$PATH
 # We copied the apr build-1 directory out of the way. For the apr-utils we need to
 # copy them back temporarily
 # cp -R "$REPOSITORYDIR/arch/$ARCH/build-1" "$REPOSITORYDIR/build-1"; 

 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include -arch $ARCH" \
  LDFLAGS="-L$REPOSITORYDIR/lib -arch $ARCH -dead_strip -prebind" \
  PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --enable-shared CC="gcc -arch $ARCH" \
  --with-apr=$REPOSITORYDIR/bin/apr-1-config  --with-expat=$REPOSITORYDIR \
  --with-iconv=/usr --without-berkeley-db \
  --without-mysql --without-pgsql --without-sqlite2 --with-sqlite3=/usr/lib \
;

 make clean;
 #make
 make $OTHERMAKEARGs;
 make install;
 
 # And now we need to remove the temporary directory again
 # rm -rf "$REPOSITORYDIR/build-1";

done


# merge libaprutil

for liba in lib/libaprutil-$MAIN_LIB_VER.a lib/libaprutil-$FULL_LIB_VER.dylib
do

 if [ $NUMARCH -eq 1 ]
 then
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

if [ -f "$REPOSITORYDIR/lib/libaprutil-$FULL_LIB_VER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libaprutil-$FULL_LIB_VER.dylib" "$REPOSITORYDIR/lib/libaprutil-$FULL_LIB_VER.dylib";
 ln -sfn libaprutil-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libaprutil-$EXT_MAIN_LIB_VER.dylib;
 ln -sfn libaprutil-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libaprutil-$MAIN_LIB_VER.dylib;
 ln -sfn libaprutil-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libaprutil.dylib;
fi

#pkgconfig
for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/lib/pkgconfig";
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' "$REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/aprutil-1.pc" > "$REPOSITORYDIR/lib/pkgconfig/aprutil-1.pc";
 break;
done

#Copy shell script
for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/bin";
 cp "$REPOSITORYDIR/arch/$ARCH/lib/aprutil.exp" "$REPOSITORYDIR/lib/aprutil.exp";
 break;
done


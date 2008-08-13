# ------------------
#     libapr
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


 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --with-ssl --enable-shared --with-expat --mandir=$REPOSITORYDIR/share/man \
;

 make clean;
 #make
 make $OTHERMAKEARGs;
 make install;

 # apr build a build structure for all following utilies and so on. We need to move that to
 # the arch directory
 mv "$REPOSITORYDIR/build-1" "$REPOSITORYDIR/arch/$ARCH/build-1"

done


# merge libapr

for liba in lib/libapr-$MAIN_LIB_VER.a lib/libapr-$FULL_LIB_VER.dylib
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

if [ -f "$REPOSITORYDIR/lib/libapr-$FULL_LIB_VER.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libapr-$FULL_LIB_VER.dylib" "$REPOSITORYDIR/lib/libapr-$FULL_LIB_VER.dylib";
 ln -sfn libapr-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libapr-$EXT_MAIN_LIB_VER.dylib;
 ln -sfn libapr-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libapr-$MAIN_LIB_VER.dylib;
 ln -sfn libapr-$FULL_LIB_VER.dylib $REPOSITORYDIR/lib/libapr.dylib;
fi

#pkgconfig
for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/lib/pkgconfig";
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' "$REPOSITORYDIR/arch/$ARCH/lib/pkgconfig/apr-1.pc" > "$REPOSITORYDIR/lib/pkgconfig/apr-1.pc";
 break;
done

#Copy shell script
for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/bin";
 cp "$REPOSITORYDIR/arch/$ARCH/lib/apr.exp" "$REPOSITORYDIR/lib/apr.exp";
 sed 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' "$REPOSITORYDIR/arch/$ARCH/bin/apr-1-config" > "$REPOSITORYDIR/bin/apr-1-config";
 break;
done
chmod a+x "$REPOSITORYDIR/bin/apr-1-config"


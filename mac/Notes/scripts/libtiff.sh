# ------------------
#     libtiff
# ------------------


# prepare

# export REPOSITORYDIR="" \
# ARCHS="ppc i386" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.3.9.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  ppc64ONLYARG="-mcpu=G5 -mtune=G5 -ftree-vectorize" \
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

for ARCH in $ARCHS
do

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  TARGET="i386-apple-darwin8"
  MACSDKDIR=$i386MACSDKDIR
  ARCHARGs="$i386ONLYARG"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET="powerpc-apple-darwin8"
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET="powerpc64-apple-darwin8"
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
 fi

 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -funroll-loops -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -funroll-loops -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --disable-shared --with-apple-opengl-framework;

 make clean;
 cd ./port; make;
 cd ../libtiff; make install;
 cd ../;

 rm $REPOSITORYDIR/include/tiffconf.h;
 cp "./libtiff/tiffconf.h" "$REPOSITORYDIR/arch/$ARCH/include/tiffconf.h";

done


# merge libtiff

for liba in lib/libtiff.a lib/libtiffxx.a
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


# merge config.h

for conf_h in include/tiffconf.h
do
 
 echo "" > "$REPOSITORYDIR/$conf_h";

 if [ $NUMARCH -eq 1 ]
 then
  mv $REPOSITORYDIR/arch/$ARCHS/$conf_h $REPOSITORYDIR/$conf_h;
  continue;
 fi

 for ARCH in $ARCHS
 do
  if [ $ARCH = "i386" -o $ARCH = "i686" ]
  then
   echo "#if defined(__i386__)"              >> "$REPOSITORYDIR/$conf_h";
   echo ""                                   >> "$REPOSITORYDIR/$conf_h";
   cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
   echo ""                                   >> "$REPOSITORYDIR/$conf_h";
   echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
  elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
  then
   echo "#if defined(__ppc__)"               >> "$REPOSITORYDIR/$conf_h";
   echo ""                                   >> "$REPOSITORYDIR/$conf_h";
   cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
   echo ""                                   >> "$REPOSITORYDIR/$conf_h";
   echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
  elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
  then
   echo "#if defined(__ppc64__)"             >> "$REPOSITORYDIR/$conf_h";
   echo ""                                   >> "$REPOSITORYDIR/$conf_h";
   cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
   echo ""                                   >> "$REPOSITORYDIR/$conf_h";
   echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
  fi
 done

done





# ------------------
#     libtiff
# ------------------
# $Id: libtiff.sh 1902 2007-02-04 22:27:47Z ippei $
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
#  OTHERARGs="";


uname_release=$(uname -r)
uname_arch=$(uname -p)
os_dotvsn=${uname_release%%.*}
os_dotvsn=$(($os_dotvsn - 4))
NATIVE_SDKDIR="/Developer/SDKs/MacOSX10.$os_dotvsn.sdk"
NATIVE_OSVERSION="10.$os_dotvsn"
NATIVE_ARCH=$uname_arch
NATIVE_OPTIMIZE=""

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
  TARGET=$i386TARGET
  MACSDKDIR=$i386MACSDKDIR
  ARCHARGs="$i386ONLYARG"
  CC=$i386CC;
  CXX=$i386CXX;
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  TARGET=$ppcTARGET
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
  CC=$ppcCC;
  CXX=$ppcCXX;
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET=$ppc64TARGET
  MACSDKDIR=$ppc64MACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
  CC=$ppc64CC;
  CXX=$ppc64CXX;
 elif [ $ARCH = "x86_64" ]
 then
  TARGET=$x64TARGET
  MACSDKDIR=$x64MACSDKDIR
  ARCHARGs="$x64ONLYARG"
  CC=$x64CC;
  CXX=$x64CXX;
 fi

 # Configure is looking for a specific version of crt1.o based on what the compiler was built for
 # This library isn't in the search path, so copy it to lib
 crt1obj=lib/crt1.$NATIVE_OSVERSION.o 
 [ -f $REPOSITORYDIR/$crt1obj ] || cp $NATIVE_SDK/usr/$crt1obj $REPOSITORYDIR/$crt1obj ;
 # File exists for 10.5 and 10.6. 10.4 is a problem
 [ -f $REPOSITORYDIR/$crt1obj ] || exit 1 ;

 env \
   CC=$CC CXX=$CXX \
   CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
   CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
   CPPFLAGS="-I$REPOSITORYDIR/include" \
   LDFLAGS="-L$REPOSITORYDIR/lib -isysroot $MACSDKDIR -arch $ARCH -dead_strip" \
   NEXT_ROOT="$MACSDKDIR" \
   ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
     --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
     --enable-static --enable-shared --with-apple-opengl-framework --without-x \
     --with-zlib-dir=$MACSDKDIR/usr/lib --with-zlib-include-dir=$MACSDKDIR/usr/include \
     ;

 [ -f $REPOSITORYDIR/$crt1obj ] && rm  $REPOSITORYDIR/$crt1obj;
 make clean;
 cd ./port; make $OTHERMAKEARGs;
 cd ../libtiff; make $OTHERMAKEARGs install;
 cd ../;

 rm $REPOSITORYDIR/include/tiffconf.h;
 cp "./libtiff/tiffconf.h" "$REPOSITORYDIR/arch/$ARCH/include/tiffconf.h";

done


# merge libtiff

for liba in lib/libtiff.a lib/libtiffxx.a lib/libtiff.3.dylib lib/libtiffxx.3.dylib
do

 if [ $NUMARCH -eq 1 ] ; then
   mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
   #Power programming: if filename ends in "a" then ...
   [ ${liba##*.} = a ] && ranlib "$REPOSITORYDIR/$liba";
   continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
   LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
done


if [ -f "$REPOSITORYDIR/lib/libtiff.3.dylib" ] ; then
  install_name_tool -id "$REPOSITORYDIR/lib/libtiff.3.dylib" "$REPOSITORYDIR/lib/libtiff.3.dylib";
  ln -sfn libtiff.3.dylib $REPOSITORYDIR/lib/libtiff.dylib;
fi
if [ -f "$REPOSITORYDIR/lib/libtiffxx.3.dylib" ] ; then
  install_name_tool -id "$REPOSITORYDIR/lib/libtiffxx.3.dylib" "$REPOSITORYDIR/lib/libtiffxx.3.dylib";
  for ARCH in $ARCHS
  do
    install_name_tool -change "$REPOSITORYDIR/arch/$ARCH/lib/libtiff.3.dylib" "$REPOSITORYDIR/lib/libtiff.3.dylib" "$REPOSITORYDIR/lib/libtiffxx.3.dylib";
  done
  ln -sfn libtiffxx.3.dylib $REPOSITORYDIR/lib/libtiffxx.dylib;
fi

# merge config.h

for conf_h in include/tiffconf.h
do
 
  echo "" > "$REPOSITORYDIR/$conf_h";

  if [ $NUMARCH -eq 1 ] ; then
	  mv $REPOSITORYDIR/arch/$ARCHS/$conf_h $REPOSITORYDIR/$conf_h;
    continue;
  fi

  for ARCH in $ARCHS
  do
    if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
      echo "#if defined(__i386__)"              >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
    elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
      echo "#if defined(__ppc__)"               >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
    elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ] ; then
      echo "#if defined(__ppc64__)"             >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
    elif [ $ARCH = "x86_64" ] ; then
      echo "#if defined(__x86_64__)"             >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
      echo ""                                   >> "$REPOSITORYDIR/$conf_h";
      echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
    fi
  done

done

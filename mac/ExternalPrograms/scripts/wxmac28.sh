# ------------------
#     wxMac 2.8
# ------------------
# $Id: wxmac28.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007-2008, Ippei Ukai


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
#  ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  ppcOSVERSION="10.4" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcOPTIMIZE="-mcpu=G3 -mtune=G4" \
#  i386TARGET="i386-apple-darwin8" \
#  i386OSVERSION="10.4" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386OPTIMIZE ="-march=prescott -mtune=pentium-m -ftree-vectorize" \
#  OTHERARGs="";

uname_release=$(uname -r)
uname_arch=$(uname -p)
os_dotvsn=${uname_release%%.*}
os_dotvsn=$(($os_dotvsn - 4))
NATIVE_SDKDIR="/Developer/SDKs/MacOSX10.$os_dotvsn.sdk"
NATIVE_OSVERSION="10.$os_dotvsn"
NATIVE_ARCH=$uname_arch
NATIVE_OPTIMIZE=""

# patch for Snow Leopard
thisarch=$(uname -m)
if [ "$thisarch" = x86_64 ] ; then
	patch -Np1 < ../scripts/wxMac-2.8.10.patch
fi

WXVERSION="2.8"
WXVER_COMP="$WXVERSION.0"
#WXVER_FULL="$WXVER_COMP.5.0"  # for 2.8.8
WXVER_FULL="$WXVER_COMP.6.0"  # for 2.8.10

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# compile

let NUMARCH="0"

# remove 64-bit archs from ARCHS
ARCHS_TMP=$ARCHS
ARCHS=""
for ARCH in $ARCHS_TMP
do
 if [ $ARCH = "i386" -o $ARCH = "i686" -o $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
   NUMARCH=$(($NUMARCH + 1))
   if [ "$ARCHS" = "" ] ; then
     ARCHS="$ARCH"
   else
     ARCHS="$ARCHS $ARCH"
   fi
 fi
done


for ARCH in $ARCHS
do

 mkdir -p "osx-$ARCH-build";
 cd "osx-$ARCH-build";

 mkdir -p "$REPOSITORYDIR/arch/$ARCH/bin";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/lib";
 mkdir -p "$REPOSITORYDIR/arch/$ARCH/include";

 ARCHARGs=""
 MACSDKDIR=""

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
  TARGET=$i386TARGET
  MACSDKDIR=$i386MACSDKDIR
  ARCHARGs="$i386ONLYARG"
  CC=$i386CC
  CXX=$i386CXX
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
  TARGET=$ppcTARGET
  MACSDKDIR=$ppcMACSDKDIR
  ARCHARGs="$ppcONLYARG"
  CC=$ppcCC
  CXX=$ppcCXX
 fi
 
 ARCHARGs=$(echo $ARCHARGs | sed 's/-ftree-vectorize//')
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip -I$REPOSITORYDIR/include" \
  OBJCFLAGS="-arch $ARCH" \
  OBJCXXFLAGS="-arch $ARCH" \
  LDFLAGS="-L$REPOSITORYDIR/lib -isysroot $MACSDKDIR -arch $ARCH -dead_strip -prebind" \
  ../configure --prefix="$REPOSITORYDIR" --exec-prefix=$REPOSITORYDIR/arch/$ARCH --disable-dependency-tracking \
    --host="$TARGET" --with-macosx-sdk=$MACSDKDIR --with-macosx-version-min=$OSVERSION \
    --enable-monolithic --enable-unicode --with-opengl --enable-compat26 --disable-graphics_ctx \
    --enable-shared --disable-debug;

 wxmacdir="lib/wx/include/mac-unicode-release-$WXVERSION/wx"
 wxmacconf="$wxmacdir/setup.h"

 mkdir -p $REPOSITORYDIR/$wxmacdir
 echo "" >$REPOSITORYDIR/$wxmacconf

# For all SDK; CP panel problem still exists.
## disable core graphics implementation for 10.3
#if [[ $TARGET == *darwin7 ]]
#then
 echo '#ifndef wxMAC_USE_CORE_GRAPHICS'    >> $wxmacconf
 echo ' #define wxMAC_USE_CORE_GRAPHICS 0' >> $wxmacconf
 echo '#endif'                             >> $wxmacconf
 echo ''                                   >> $wxmacconf 
#fi

 make clean;

#hack
 cp utils/wxrc/Makefile utils/wxrc/Makefile-copy;
 echo "all: " > utils/wxrc/Makefile;
 echo "" >> utils/wxrc/Makefile;
 echo "install: " >> utils/wxrc/Makefile;
#~hack

 make $OTHERMAKEARGs LIBS="-lexpat" \
    LDFLAGS="-L$REPOSITORYDIR/lib -isysroot $MACSDKDIR -arch $ARCH -L$NATIVE_SDKDIR/usr/lib -dead_strip -prebind" \
  ;
 make install LIBS="-lexpat" \
    LDFLAGS="-L$REPOSITORYDIR/lib -isysroot $MACSDKDIR -arch $ARCH -L$NATIVE_SDKDIR/usr/lib -dead_strip -prebind" \
  ;
 cd ../;

done


# merge libwx

for liba in "lib/libwx_macu-$WXVER_FULL.dylib" "lib/libwx_macu_gl-$WXVER_FULL.dylib"
do
  if [ $NUMARCH -eq 1 ] ; then
    mv "$REPOSITORYDIR/arch/$ARCH/$liba" "$REPOSITORYDIR/$liba";
    continue
  fi

  LIPOARGs=""
  for ARCH in $ARCHS
  do
    LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
  done

  lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
done


if [ -f "$REPOSITORYDIR/lib/libwx_macu-$WXVER_FULL.dylib" ]
then
 install_name_tool \
  -id "$REPOSITORYDIR/lib/libwx_macu-$WXVER_COMP.dylib" \
  "$REPOSITORYDIR/lib/libwx_macu-$WXVER_FULL.dylib";
 ln -sfn "libwx_macu-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_macu-$WXVER_COMP.dylib";
 ln -sfn "libwx_macu-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_macu-$WXVERSION.dylib";
fi
if [ -f "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVER_FULL.dylib" ]
then
 install_name_tool \
  -id "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVER_COMP.dylib" \
  "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVER_FULL.dylib";
 for ARCH in $ARCHS
 do
  install_name_tool \
   -change "$REPOSITORYDIR/arch/$ARCH/lib/libwx_macu-$WXVER_COMP.dylib" \
   "$REPOSITORYDIR/lib/libwx_macu-$WXVER_COMP.dylib" \
   "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVER_FULL.dylib";
 done
 ln -sfn "libwx_macu_gl-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVER_COMP.dylib";
 ln -sfn "libwx_macu_gl-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVERSION.dylib";
fi



# merge setup.h

for confname in "wx/setup.h"
do

 if [ $NUMARCH -eq 1 ] ; then
   ARCH=$ARCHS
   cat "$REPOSITORYDIR/arch/$ARCH/$wxmacconf" >>"$REPOSITORYDIR/$wxmacconf";
   continue
 fi

 for ARCH in $ARCHS
 do
   conf_h="$wxmacconf"

   if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
     echo "#if defined(__i386__)"                       >> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
     cat "$REPOSITORYDIR/arch/$ARCH/$conf_h"  			>> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
     echo "#endif"                                      >> "$REPOSITORYDIR/$wxmacconf";
   elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
     echo "#if defined(__ppc__) || defined(__ppc64__)"  >> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
     cat "$REPOSITORYDIR/arch/$ARCH/$conf_h"			>> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
     echo "#endif"                                      >> "$REPOSITORYDIR/$wxmacconf";
   fi
 done

done

#wx-config

for ARCH in $ARCHS
do
 sed -e 's/^exec_prefix.*$/exec_prefix=\$\{prefix\}/' \
     -e 's/^is_cross \&\& target.*$//' \
     -e 's/-arch '$ARCH'//' \
     $REPOSITORYDIR/arch/$ARCH/bin/wx-config > $REPOSITORYDIR/bin/wx-config
 break;
done

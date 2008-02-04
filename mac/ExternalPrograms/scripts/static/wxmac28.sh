# ------------------
#     wxMac 2.8
# ------------------
# $Id: wxmac28.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007, Ippei Ukai


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


# init

WXVERSION="2.8"

let NUMARCH="0"
for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/bin";
mkdir -p "$REPOSITORYDIR/lib";
mkdir -p "$REPOSITORYDIR/include";


# compile

# remove 64-bit archs from ARCHS
ARCHS_TMP=$ARCHS
ARCHS=""
for ARCH in $ARCHS_TMP
do
 if [ $ARCH = "i386" -o $ARCH = "i686" -o $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  ARCHS="$ARCHS $ARCH"
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
 fi


 env CFLAGS="-arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -dead_strip -prebind" \
  ../configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH --with-macosx-sdk=$MACSDKDIR \
  --disable-debug --disable-shared --enable-monolithic --enable-unicode --with-opengl --enable-compat26 --disable-graphics_ctx;


 # disabled for all targets for now.
# # disable core graphics implementation for 10.3
# if [[ $TARGET == *darwin7 ]]
# then
  echo '#ifndef wxMAC_USE_CORE_GRAPHICS'    >> lib/wx/include/mac-unicode-release-$WXVERSION/wx/setup.h
  echo ' #define wxMAC_USE_CORE_GRAPHICS 0' >> lib/wx/include/mac-unicode-release-$WXVERSION/wx/setup.h
  echo '#endif'                             >> lib/wx/include/mac-unicode-release-$WXVERSION/wx/setup.h
  echo ''                                   >> lib/wx/include/mac-unicode-release-$WXVERSION/wx/setup.h
# fi

 make clean;

#hack
 cp utils/wxrc/Makefile utils/wxrc/Makefile-copy;
 echo "all: " > utils/wxrc/Makefile;
 echo "" >> utils/wxrc/Makefile;
 echo "install: " >> utils/wxrc/Makefile;
#~hack

 make $OTHERMAKEARGs;
 make install;

 cd ../;

done


# merge libwx

for libname in lib/libwx_macu-$WXVERSION lib/libwx_macu_gl-$WXVERSION lib/libwxregexu-$WXVERSION
do

 LIPOARGs=""

 for ARCH in $ARCHS
 do
  
#  if [ $ARCH = "i386" -o $ARCH = "i686" ]
#  then
#   liba="$libname-i386-apple-darwin8.a"
#  elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
#  then
   liba="$libname.a"
#  fi

  if [ $NUMARCH -eq 1 ]
  then
   mv "$REPOSITORYDIR/arch/$ARCH/$liba" "$REPOSITORYDIR/$libname.a";
   ranlib "$REPOSITORYDIR/$libname.a";
   continue
  fi

  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

 if [ $NUMARCH -gt 1 ]
 then
  lipo $LIPOARGs -create -output "$REPOSITORYDIR/$libname.a";
  ranlib "$REPOSITORYDIR/$libname.a";
 fi

done


# merge setup.h

for confname in "wx/setup.h"
do

 wxmacconf="lib/wx/include/mac-unicode-release-static-$WXVERSION/$confname"

 mkdir -p `dirname $REPOSITORYDIR/$wxmacconf`;
 echo "" > "$REPOSITORYDIR/$wxmacconf";

 if [ $NUMARCH -eq 1 ]
 then
  ARCH=$ARCHS

#  if [ $ARCH = "i386" -o $ARCH = "i686" ]
#  then
#   conf_h="lib/wx/include/i386-apple-darwin8-mac-unicode-release-static-$WXVERSION/$confname"
#  elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
#  then
   conf_h="$wxmacconf"
#  fi

  mv "$REPOSITORYDIR/arch/$ARCH/$conf_h" "$REPOSITORYDIR/$wxmacconf";
  continue
 fi

 for ARCH in $ARCHS
 do
  conf_h="$wxmacconf"

  mkdir -p `dirname $REPOSITORYDIR/$wxmacconf`;

  if [ $ARCH = "i386" -o $ARCH = "i686" ]
  then
   echo "#if defined(__i386__)"              >> "$REPOSITORYDIR/$wxmacconf";
   echo ""                                   >> "$REPOSITORYDIR/$wxmacconf";
   cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$wxmacconf";
   echo ""                                   >> "$REPOSITORYDIR/$wxmacconf";
   echo "#endif"                             >> "$REPOSITORYDIR/$wxmacconf";
  elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
  then
   echo "#if defined(__ppc__) || defined(__ppc64__)" >> "$REPOSITORYDIR/$wxmacconf";
   echo ""                                           >> "$REPOSITORYDIR/$wxmacconf";
   cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"          >> "$REPOSITORYDIR/$wxmacconf";
   echo ""                                           >> "$REPOSITORYDIR/$wxmacconf";
   echo "#endif"                                     >> "$REPOSITORYDIR/$wxmacconf";
  fi
 done

done





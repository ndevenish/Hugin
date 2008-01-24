# ---------------------
#   wxWidgets 2.7.0-1
# ---------------------
# $Id: wxmac27.sh 1902 2007-02-04 22:27:47Z ippei $


# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  i386TARGET="i386-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
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

 #debug
 rm -rf "$REPOSITORYDIR/lib/wx/include/mac-unicode-release-2.7/$confname";
 #~debug

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


 env CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-arch $ARCH -L$REPOSITORYDIR/lib -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ../configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --disable-debug --enable-shared --enable-monolithic --enable-unicode --with-opengl;


 make clean;

#hack
 cp utils/wxrc/Makefile utils/wxrc/Makefile-copy;
 echo "all: " > utils/wxrc/Makefile;
 echo "" >> utils/wxrc/Makefile;
 echo "install: " >> utils/wxrc/Makefile;
#~hack

 make install;

 cd ../;

 #debug
 if [ -f "$REPOSITORYDIR/lib/wx/include/mac-unicode-release-2.7/wx/setup.h" ]
 then
  mv "$REPOSITORYDIR/lib/wx/include/mac-unicode-release-2.7/wx/setup.h" "$REPOSITORYDIR/arch/$ARCH/lib/wx/include/mac-unicode-release-2.7/wx/setup.h";
 fi
 #~debug

done


# merge libwx

WXVERSION="2.7"
WXVER_COMP="$WXVERSION.0"
WXVER_FULL="$WXVER_COMP.0.0"

for liba in "lib/libwx_macu-$WXVER_FULL.dylib" "lib/libwx_macu_gl-$WXVER_FULL.dylib"
do
 LIPOARGs=""
 for ARCH in $ARCHS
 do
  if [ $NUMARCH -eq 1 ]
  then
   mv "$REPOSITORYDIR/arch/$ARCH/$liba" "$REPOSITORYDIR/$liba";
   continue
  fi
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done
 if [ $NUMARCH -gt 1 ]
 then
  lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 fi
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
   -change "$REPOSITORYDIR/arch/$ARCH/lib/libwx_macu-$WXVER_COMP.dylib" "$REPOSITORYDIR/lib/libwx_macu-$WXVER_COMP.dylib" \
   "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVER_FULL.dylib";
 done
 ln -sfn "libwx_macu_gl-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVER_COMP.dylib";
 ln -sfn "libwx_macu_gl-$WXVER_FULL.dylib" "$REPOSITORYDIR/lib/libwx_macu_gl-$WXVERSION.dylib";
fi


# merge setup.h

for confname in "wx/setup.h"
do

 wxmacconf="lib/wx/include/mac-unicode-release-2.7/$confname"

 mkdir -p $(dirname "$REPOSITORYDIR/$wxmacconf");
 echo "" > "$REPOSITORYDIR/$wxmacconf";

 if [ $NUMARCH -eq 1 ]
 then
  ARCH=$ARCHS
  conf_h="$wxmacconf"

  mkdir -p `dirname $REPOSITORYDIR/$wxmacconf`;
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
   echo "#if defined(__ppc__)" >> "$REPOSITORYDIR/$wxmacconf";
   echo ""                                           >> "$REPOSITORYDIR/$wxmacconf";
   cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"          >> "$REPOSITORYDIR/$wxmacconf";
   echo ""                                           >> "$REPOSITORYDIR/$wxmacconf";
   echo "#endif"                                     >> "$REPOSITORYDIR/$wxmacconf";
  fi
 done

done





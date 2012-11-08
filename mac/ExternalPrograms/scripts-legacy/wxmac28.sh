# ------------------
#     wxMac 2.8
# ------------------
# $Id: wxmac28.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007-2008, Ippei Ukai

# 2009-12-04.0 Remove unneeded arguments to make and make install; made make single threaded


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

# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
#               Works Intel: 10.5, 10.6 & Powerpc 10.4, 10.5
# 20100624.0 hvdw More robust error checking on compilation
# 20120415.0 hvdw upgrade to 2.8.12
# -------------------------------

fail()
{
        echo "** Failed at $1 **"
        exit 1
}


uname_release=$(uname -r)
uname_arch=$(uname -p)
[ $uname_arch = powerpc ] && uname_arch="ppc"
os_dotvsn=${uname_release%%.*}
os_dotvsn=$(($os_dotvsn - 4))
case $os_dotvsn in
 4 ) os_sdkvsn="10.4u" ;;
 5|6 ) os_sdkvsn=10.$os_dotvsn ;;
 * ) echo "Unhandled OS Version: 10.$os_dotvsn. Build aborted."; exit 1 ;;
esac

NATIVE_SDKDIR="/Developer/SDKs/MacOSX$os_sdkvsn.sdk"
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
#WXVER_FULL="$WXVER_COMP.6.0"  # for 2.8.10
#WXVER_FULL="$WXVER_COMP.7.0"  # for 2.8.11
WXVER_FULL="$WXVER_COMP.8.0"  # for 2.8.12

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
 fi
 
 ARCHARGs=$(echo $ARCHARGs | sed 's/-ftree-vectorize//')
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip -I$REPOSITORYDIR/include" \
  OBJCFLAGS="-arch $ARCH" \
  OBJCXXFLAGS="-arch $ARCH" \
  LDFLAGS="-L$REPOSITORYDIR/lib -arch $ARCH -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  ../configure --prefix="$REPOSITORYDIR" --exec-prefix=$REPOSITORYDIR/arch/$ARCH --disable-dependency-tracking \
    --host="$TARGET" --with-macosx-sdk=$MACSDKDIR --with-macosx-version-min=$OSVERSION \
    --enable-monolithic --enable-unicode --with-opengl --enable-compat26 --disable-graphics_ctx \
    --enable-shared --disable-debug --enable-aui || fail "configure step for $ARCH";

### Setup.h is created by configure!
# For all SDK; CP panel problem still exists.
## disable core graphics implementation for 10.3
#if [[ $TARGET == *darwin7 ]]
#then
 
# need to find out where setup.h was created. This seems to vary if building on powerpc and
# is different under 10.4 and 10.5
 whereIsSetup=$(find . -name setup.h -print)
 whereIsSetup=${whereIsSetup#./}

 echo '#ifndef wxMAC_USE_CORE_GRAPHICS'    >> $whereIsSetup
 echo ' #define wxMAC_USE_CORE_GRAPHICS 0' >> $whereIsSetup
 echo '#endif'                             >> $whereIsSetup
 echo ''                                   >> $whereIsSetup
#fi

 make clean;

#hack
 cp utils/wxrc/Makefile utils/wxrc/Makefile-copy;
 echo "all: " > utils/wxrc/Makefile;
 echo "" >> utils/wxrc/Makefile;
 echo "install: " >> utils/wxrc/Makefile;
#~hack

 case $NATIVE_OSVERSION in
	 10.4 )
     dylib_name="dylib1.o"
		 ;;
	 10.5 | 10.6 )
		 dylib_name="dylib1.10.5.o"
		 ;;
	 * )
		 echo "OS Version $NATIVE_OSVERSION not supported"; exit 1
		 ;;
 esac
 cp $NATIVE_SDKDIR/usr/lib/$dylib_name $REPOSITORYDIR/lib/

# Need to build single-threaded. libwx_macu-2.8.dylib needs to be built before libwx_macu_gl-2.8 to avoid a link error.
# This is only problematic for Intel builds, where jobs can be >1
 make --jobs=1 || fail "failed at make step of $ARCH";
 make install || fail "make install step of $ARCH";

 rm $REPOSITORYDIR/lib/$dylib_name;

 cd ../;

done


# merge libwx

for liba in "lib/libwx_macu-$WXVER_FULL.dylib" "lib/libwx_macu_gl-$WXVER_FULL.dylib"
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

for dummy in "wx/setup.h"
do

 wxmacconf="lib/wx/include/mac-unicode-release-$WXVERSION/wx/setup.h"

 mkdir -p $(dirname "$REPOSITORYDIR/$wxmacconf")
 echo ""  >$REPOSITORYDIR/$wxmacconf

 if [ $NUMARCH -eq 1 ] ; then
   ARCH=$ARCHS
   pushd $REPOSITORYDIR
	 whereIsSetup=$(find ./arch/$ARCH/lib/wx -name setup.h -print)
	 whereIsSetup=${whereIsSetup#./arch/*/}
	 popd 
	 cat "$REPOSITORYDIR/arch/$ARCH/$whereIsSetup" >>"$REPOSITORYDIR/$wxmacconf";
   continue
 fi

 for ARCH in $ARCHS
 do

 	 pushd $REPOSITORYDIR
 	 whereIsSetup=$(find ./arch/$ARCH/lib/wx -name setup.h -print)
 	 whereIsSetup=${whereIsSetup#./arch/*/}
 	 popd 

   if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
     echo "#if defined(__i386__)"                       >> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
		 cat "$REPOSITORYDIR/arch/$ARCH/$whereIsSetup"      >> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
     echo "#endif"                                      >> "$REPOSITORYDIR/$wxmacconf";
   elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
     echo "#if defined(__ppc__) || defined(__ppc64__)"  >> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
		 cat "$REPOSITORYDIR/arch/$ARCH/$whereIsSetup"      >> "$REPOSITORYDIR/$wxmacconf";
     echo ""                                            >> "$REPOSITORYDIR/$wxmacconf";
     echo "#endif"                                      >> "$REPOSITORYDIR/$wxmacconf";
  else
		 echo "Unhandled ARCH: $ARCH. Aborting build."; exit 1
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

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


# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100121.0 sg Script updated for 3.9.2
# 20100624.0 hvdw More robust error checking on compilation
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

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
   TARGET=$i386TARGET
   MACSDKDIR=$i386MACSDKDIR
   ARCHARGs="$i386ONLYARG"
   OSVERSION="$i386OSVERSION"
   CC=$i386CC
   CXX=$i386CXX
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
 fi

 # Configure is looking for a specific version of crt1.o based on what the compiler was built for
 # This library isn't in the search path, so copy it to lib
 case $NATIVE_OSVERSION in
	 10.4 )
   		crt1obj="lib/crt1.o"
			;;
   10.5 | 10.6 )
      crt1obj="lib/crt1.$NATIVE_OSVERSION.o"
			;;
	 * )
			echo "Unsupported OS Version: $NATIVE_OSVERSION";
			exit 1;
			;;
 esac

 [ -f $REPOSITORYDIR/$crt1obj ] || cp $NATIVE_SDK/usr/$crt1obj $REPOSITORYDIR/$crt1obj ;
 # File exists for 10.5 and 10.6. 10.4 is now fixed
 [ -f $REPOSITORYDIR/$crt1obj ] || exit 1 ;

 env \
   CC=$CC CXX=$CXX \
   CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -dead_strip" \
   CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O3 -dead_strip" \
   CPPFLAGS="-I$REPOSITORYDIR/include" \
   LDFLAGS="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip" \
   NEXT_ROOT="$MACSDKDIR" \
   ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
     --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
     --enable-static --enable-shared --with-apple-opengl-framework --without-x \
     || fail "configure step for $ARCH" ;

 [ -f $REPOSITORYDIR/$crt1obj ] && rm  $REPOSITORYDIR/$crt1obj;
 make clean;
 cd ./port; make $OTHERMAKEARGs || fail "failed at make step of $ARCH";
 cd ../libtiff; make $OTHERMAKEARGs install || fail "make install step of $ARCH";
 cd ../;

 rm $REPOSITORYDIR/include/tiffconf.h;
 cp "./libtiff/tiffconf.h" "$REPOSITORYDIR/arch/$ARCH/include/tiffconf.h";

done


# merge libtiff

for liba in lib/libtiff.a lib/libtiffxx.a lib/libtiff.3.dylib lib/libtiffxx.3.dylib
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

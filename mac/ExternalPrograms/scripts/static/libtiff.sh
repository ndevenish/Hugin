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
# 20091206.0 sg Script NOT tested but uses std boilerplate
# -------------------------------

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
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
   TARGET=$ppcTARGET
   MACSDKDIR=$ppcMACSDKDIR
   ARCHARGs="$ppcONLYARG"
   OSVERSION="$ppcOSVERSION"
   CC=$ppcCC
   CXX=$ppcCXX
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ] ; then
   TARGET=$ppc64TARGET
   MACSDKDIR=$ppc64MACSDKDIR
   ARCHARGs="$ppc64ONLYARG"
   OSVERSION="$ppc64OSVERSION"
   CC=$ppc64CC
   CXX=$ppc64CXX
 elif [ $ARCH = "x86_64" ] ; then
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
 fi
 
 env \
  CC=$CC CXX=$CXX \
  CFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CXXFLAGS="-isysroot $MACSDKDIR -arch $ARCH $ARCHARGs $OTHERARGs -O2 -dead_strip" \
  CPPFLAGS="-I$REPOSITORYDIR/include" \
  LDFLAGS="-L$REPOSITORYDIR/lib -mmacosx-version-min=$OSVERSION -dead_strip -prebind" \
  NEXT_ROOT="$MACSDKDIR" \
  ./configure --prefix="$REPOSITORYDIR" --disable-dependency-tracking \
  --host="$TARGET" --exec-prefix=$REPOSITORYDIR/arch/$ARCH \
  --disable-shared --with-apple-opengl-framework --without-x;

 make clean;
 cd ./port; make $OTHERMAKEARGs;
 cd ../libtiff; make $OTHERMAKEARGs install;
 cd ../;

 rm $REPOSITORYDIR/include/tiffconf.h;
 cp "./libtiff/tiffconf.h" "$REPOSITORYDIR/arch/$ARCH/include/tiffconf.h";

done


# merge libtiff

for liba in lib/libtiff.a lib/libtiffxx.a
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
 ranlib "$REPOSITORYDIR/$liba";

done

# merge config.h

for conf_h in include/tiffconf.h
do
 
 if [ $NUMARCH -eq 1 ] ; then
	 if [ -f $REPOSITORYDIR/arch/$ARCHS/$conf_h ] ; then
		 echo "Moving arch/$ARCHS/$conf.h to $conf.h"
		 mv $REPOSITORYDIR/arch/$ARCHS/$conf_h $REPOSITORYDIR/$conf_h;
		 continue
	 else
	   echo "File arch/$ARCHS/$conf_h not found. Aborting build."
	   exit 1;
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
     echo "#if defined(__x86_64__)"            >> "$REPOSITORYDIR/$conf_h";
     echo ""                                   >> "$REPOSITORYDIR/$conf_h";
     cat  "$REPOSITORYDIR/arch/$ARCH/$conf_h"  >> "$REPOSITORYDIR/$conf_h";
     echo ""                                   >> "$REPOSITORYDIR/$conf_h";
     echo "#endif"                             >> "$REPOSITORYDIR/$conf_h";
   fi
 done

done





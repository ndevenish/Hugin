# ------------------
#      boost
# ------------------
# $Id: boost.sh 1902 2007-02-04 22:27:47Z ippei $
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

# -------------------------------
# 20091206.0 sg Script NOT tested but uses std boilerplate
# -------------------------------

BOOST_VER="1_40"
BOOST_THREAD_LIB="libboost_thread"

# install headers

mkdir -p "$REPOSITORYDIR/include"
rm -rf "$REPOSITORYDIR/include/boost";
cp -R "./boost" "$REPOSITORYDIR/include/";


# compile bjab

cd "./tools/jam/src";
sh "build.sh";
cd "../../../";
BJAM=$(ls ./tools/jam/src/bin.mac*/bjam)


# init

let NUMARCH="0"

for i in $ARCHS
do
  NUMARCH=$(($NUMARCH + 1))
done

mkdir -p "$REPOSITORYDIR/lib";


# compile boost_thread

for ARCH in $ARCHS
do

 mkdir -p "stage-$ARCH";

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
  MACSDKDIR=$i386MACSDKDIR
  OSVERSION=$i386OSVERSION
  OPTIMIZE=$i386OPTIMIZE
  boostARCHITECTURE="x86"
  boostADDRESSMODEL="32"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ] ; then
  MACSDKDIR=$ppcMACSDKDIR
  OSVERSION=$ppcOSVERSION
  OPTIMIZE=$ppcOPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="32"
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ] ; then
  MACSDKDIR=$ppc64MACSDKDIR
  OSVERSION=$ppc64OSVERSION
  OPTIMIZE=$ppc64OPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="64"
 elif [ $ARCH = "x86_64" ] ; then
  MACSDKDIR=$x64MACSDKDIR
  OSVERSION=$x64OSVERSION
  OPTIMIZE=$x64OPTIMIZE
  boostARCHITECTURE="x86"
  boostADDRESSMODEL="64"
 fi

 SDKVRSION=$(echo $MACSDKDIR | sed 's/^[^1]*\([[:digit:]]*\.[[:digit:]]*\).*/\1/')

 
 # hack that sends extra arguments to g++
 $BJAM -a --stagedir="stage-$ARCH" --prefix=$REPOSITORYDIR --toolset="darwin" -n stage \
  --with-thread \
  variant=release link=static \
  architecture="$boostARCHITECTURE" address-model="$boostADDRESSMODEL" \
  macosx-version="$SDKVRSION" macosx-version-min="$OSVERSION" \
  | grep "^    " | sed 's/"//g' | sed s/g++/g++\ "$OPTIMIZE"/ | sed 's/-O3/-O2/g' \
  | while read COMMAND
    do
     echo "running command: $COMMAND"
     $COMMAND
    done
done


# merge libboost_thread

for libname in "$BOOST_THREAD_LIB-$BOOST_VER"
do

 liba="$libname.a"
 if [ $NUMARCH -eq 1 ] ; then
   if [ -f stage-$ARCHS/$liba ] ; then
		 echo "Moving stage-$ARCHS/$liba to $liba"
  	 mv "stage-$ARCHS/$liba" "$REPOSITORYDIR/$liba";
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
  if [ -f stage-$ARCH/$liba ] ; then
		echo "Adding stage-$ARCH/$liba to bundle"
 	  LIPOARGs="$LIPOARGs stage-$ARCH/$liba"
	else
		echo "File stage-$ARCH/$liba was not found. Aborting build";
		exit 1;
	fi
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/lib/$libname.a";
 ranlib "$REPOSITORYDIR/lib/$libname.a";
done

if [ -f "$REPOSITORYDIR/lib/$BOOST_THREAD_LIB-$BOOST_VER.a" ] ; then
 ln -sfn $BOOST_THREAD_LIB-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_thread.a;
fi

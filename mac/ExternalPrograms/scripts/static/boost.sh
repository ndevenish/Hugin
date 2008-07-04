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


BOOST_VER="1_35"


# install headers

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

 if [ $ARCH = "i386" -o $ARCH = "i686" ]
 then
  MACSDKDIR=$i386MACSDKDIR
  OSVERSION=$i386OSVERSION
  OPTIMIZE=$i386OPTIMIZE
  boostARCHITECTURE="x86"
  boostADDRESSMODEL="32"
 elif [ $ARCH = "ppc" -o $ARCH = "ppc750" -o $ARCH = "ppc7400" ]
 then
  MACSDKDIR=$ppcMACSDKDIR
  OSVERSION=$ppcOSVERSION
  OPTIMIZE=$ppcOPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="32"
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  MACSDKDIR=$ppc64MACSDKDIR
  OSVERSION=$ppcOSVERSION
  OPTIMIZE=$ppc64OPTIMIZE
  boostARCHITECTURE="power"
  boostADDRESSMODEL="64"
 elif [ $ARCH = "x86_64" ]
 then
  MACSDKDIR=$x64MACSDKDIR
  OSVERSION=$x64OSVERSION
  OPTIMIZE=$x64OPTIMIZE
  boostARCHITECTURE="x86"
  boostADDRESSMODEL="64"
 fi


 echo "WARNING: assumes the SDK version matches the macosx-version-min" 
 
 # hack that sends extra arguments to g++
 $BJAM -a --stagedir="stage-$ARCH" --prefix=$REPOSITORYDIR --toolset="darwin" -n stage \
  --with-thread variant=release link=static \
  architecture="$boostARCHITECTURE" address-model="$boostADDRESSMODEL" macosx-version="$OSVERSION" \
  | grep "^    " | sed 's/"//g' | sed s/g++/g++\ "$OPTIMIZE"/ | sed 's/-O3/-O2/g' \
  | while read COMMAND
    do
     echo "running command: $COMMAND"
     $COMMAND
    done
done


# merge libboost_thread

for libname in "libboost_thread-mt-$BOOST_VER"
do

 LIPOARGs=""

 for ARCH in $ARCHS
 do
  
  liba="$libname.a"

  if [ $NUMARCH -eq 1 ]
  then
   mv "stage-$ARCH/lib/$liba" "$REPOSITORYDIR/lib/$libname.a";
   ranlib "$REPOSITORYDIR/lib/$libname.a";
   continue
  fi

  LIPOARGs="$LIPOARGs stage-$ARCH/lib/$liba"
 done

 if [ $NUMARCH -gt 1 ]
 then
  lipo $LIPOARGs -create -output "$REPOSITORYDIR/lib/$libname.a";
  ranlib "$REPOSITORYDIR/lib/$libname.a";
 fi

done

if [ -f "$REPOSITORYDIR/lib/libboost_thread-mt-$BOOST_VER.a" ]
then
 ln -sfn libboost_thread-mt-$BOOST_VER.a $REPOSITORYDIR/lib/libboost_thread-mt.a;
fi


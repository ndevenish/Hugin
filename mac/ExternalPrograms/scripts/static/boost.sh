# ------------------
#      boost
# ------------------
# $Id: boost.sh 1902 2007-02-04 22:27:47Z ippei $
# Copyright (c) 2007, Ippei Ukai

# prepare

# export REPOSITORYDIR="/PATH2HUGIN/mac/ExternalPrograms/repository" \
# ARCHS="ppc i386" \
#  ppcTARGET="powerpc-apple-darwin8" \
#  ppcMACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  ppcONLYARG="-mcpu=G3 -mtune=G4" \
#  i386TARGET="i386-apple-darwin8" \
#  i386MACSDKDIR="/Developer/SDKs/MacOSX10.4u.sdk" \
#  i386ONLYARG="-mfpmath=sse -msse2 -mtune=pentium-m -ftree-vectorize" \
#  OTHERARGs="";


BOOST_VER="1_34_1"


# install headers

rm -rf "$REPOSITORYDIR/include/boost";
cp -R "./boost" "$REPOSITORYDIR/include/";


# patch

gcc_hpp="$REPOSITORYDIR/include/boost/config/compiler/gcc.hpp"

mv "$gcc_hpp" "$gcc_hpp copy";

cat "$gcc_hpp copy" | sed /^.*versions\ check:$/q > "$gcc_hpp";
echo "// -- version check removed in order to use newer gcc. --" >> "$gcc_hpp";




cd "./tools/jam/src";
sh "build.sh";
cd "../../../";

cp $(dirname $0)/../darwin_mod.jam ./tools/build/v2/tools/darwin_mod.jam


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
 elif [ $ARCH = "ppc64" -o $ARCH = "ppc970" ]
 then
  TARGET=$ppc64TARGET
  MACSDKDIR=$ppc64MACSDKDIR
  ARCHARGs="$ppc64ONLYARG"
 elif [ $ARCH = "x86_64" ]
 then
  TARGET=$x64TARGET
  MACSDKDIR=$x64MACSDKDIR
  ARCHARGs="$x64ONLYARG"
 fi


 BJAM=$(ls ./tools/jam/src/bin.mac*/bjam)
 
 GXXARGS=$ARCHARGs $OTHERARGs
 GXXARGS=$(echo $GXXARGS|sed 's/\ /\\\ /g')
 
 # hack that sends extra arguments to g++
 $BJAM -a --stagedir="stage-$ARCH" --prefix=$myREPOSITORYDIR --toolset="darwin_mod" --with-thread \
  sdkroot=$(basename $MACSDKDIR .sdk) arch="$ARCH" -n stage \
  | grep "^    " | sed 's/"//g' | sed s/g++/g++\ "$GXXARGS"/ \
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




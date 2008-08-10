# --------------------
#   Hugin 0.7.0
# --------------------
# $Id: 01-compile_hugin.sh 1905 2007-02-05 00:11:26Z ippei $
# script skeleton by Ippei Ukai
# cmake part Harry van der Wolf

# prepare
source ./mac/ExternalPrograms/scripts/SetEnv-universal.txt;

# export ="/PATH2HUGIN/mac/ExternalPrograms/repository" \
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


HUGINVER_FULL="0.0"

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

 mkdir -p $ARCH;
 cd $ARCH;
 rm CMakeCache.txt;

# export PATH=$REPOSITORYDIR/bin:$PATH;
 export CMAKE_INCLUDE_PATH="$CMAKE_INCLUDE_PATH:$REPOSITORYDIR/include/wx-2.8";
# export INSTALL_OSX_BUNDLE_DIR="$REPOSITORYDIR/arch/$ARCH";
 export PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig";
 export MAC_SELF_CONTAINED_BUNDLE="$REPOSITORYDIR/arch/$ARCH";

cmake  \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR/arch/$ARCH" \
  -DCMAKE_OSX_ARCHITECTURES="$TARGET" \
  -DCMAKE_OSX_SYSROOT="$MACSDKDIR"\
  -DCMAKE_C_FLAGS="-arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \
  -DCMAKE_CXX_FLAGS="-arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \
  -DCMAKE_LDFLAGS="-L/usr/lib -L$REPOSITORYDIR/lib -dead_strip -prebind" \
  ..;


  # patch hugin_config.h
  cp "../platforms/mac/scripted_universal_build/hugin_config.h" "src/hugin_config.h"

  make;
  make install;
  
  cd ..

done

# Correct install_names in Hugin and HuginStitchProject binaries
for ARCH in $ARCHS
do 
  echo "$ARCH"
  bins="$REPOSITORYDIR/arch/$ARCH/Application/Hugin.app/Contents/MacOS/Hugin $REPOSITORYDIR/arch/$ARCH/Application/HuginStitchProject.app/Contents/MacOS/HuginStitchProject"
  for exec_file in $bins
  do 
    lib_path="`otool -L $exec_file  2>/dev/null | fgrep compatibility | cut -d\( -f1 | grep $ARCH`"
    for lib in $lib_path
    do 
      echo $lib
      libbase="`echo $lib | sed -e 's/ (.*$//' -e 's/^.*\///'`"
      echo " Changing install name for: $libbase"
      install_name_tool -change "$lib" "$REPOSITORYDIR/lib/$libbase" $exec_file
    done
  done
done


# merge execs
echo "merge executables from Hugin"

for program in bin/vig_optimize bin/tca_correct bin/pto2mk bin/nona bin/fulla bin/autooptimiser bin/align_image_stack bin/nona_gui bin/matchpoint bin/hugin_hdrmerge
do

 LIPOARGs=""

 for ARCH in $ARCHS
 do
 
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
 
  if [ $NUMARCH -eq 1 ]
  then
   mv "$REPOSITORYDIR/arch/$ARCH/$program" "$REPOSITORYDIR/$program";
   strip "$REPOSITORYDIR/$program";
   break;
  else
   lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";
   strip "$REPOSITORYDIR/$program";
  fi
 
 done

done


# merge libhugin*.dylibs
echo "Merging the libhugin*.dylibs"

for liba in lib/libhuginANN.$HUGINVER_FULL.dylib lib/libhuginbase.$HUGINVER_FULL.dylib lib/libhuginjhead.$HUGINVER_FULL.dylib lib/libhuginvigraimpex.$HUGINVER_FULL.dylib
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$liba" "$REPOSITORYDIR/$liba";
  if [[ $liba == *.a ]]
  then 
   ranlib "$REPOSITORYDIR/$liba";
  fi
  continue
 fi

 LIPOARGs=""
 
 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$liba"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$liba";
 if [[ $liba == *.a ]]
 then 
  ranlib "$REPOSITORYDIR/$liba";
 fi

done


if [ -f "$REPOSITORYDIR/lib/libhuginANN.$HUGINVER_FULL.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libhuginANN.$HUGINVER_FULL.dylib" "$REPOSITORYDIR/lib/libhuginANN.$HUGINVER_FULL.dylib"
 ln -sfn libhuginANN.$HUGINVER_FULL.dylib $REPOSITORYDIR/lib/libhuginANN.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libhuginbase.$HUGINVER_FULL.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libhuginbase.$HUGINVER_FULL.dylib" "$REPOSITORYDIR/lib/libhuginbase.$HUGINVER_FULL.dylib"
 ln -sfn libhuginbase.$HUGINVER_FULL.dylib $REPOSITORYDIR/lib/libhuginbase.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libhuginjhead.$HUGINVER_FULL.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libhuginjhead.$HUGINVER_FULL.dylib" "$REPOSITORYDIR/lib/libhuginjhead.$HUGINVER_FULL.dylib"
 ln -sfn libhuginjhead.$HUGINVER_FULL.dylib $REPOSITORYDIR/lib/libhuginjhead.dylib;
fi

if [ -f "$REPOSITORYDIR/lib/libhuginvigraimpex.$HUGINVER_FULL.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libhuginvigraimpex.$HUGINVER_FULL.dylib" "$REPOSITORYDIR/lib/libhuginvigraimpex.$HUGINVER_FULL.dylib"
 ln -sfn libhuginvigraimpex.$HUGINVER_FULL.dylib $REPOSITORYDIR/lib/libhuginvigraimpex.dylib;
fi



# merge minimal bundles to universal minimal bundle
# echo "merge minimal bundles to universal minimal bundles"



for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/Application"
 rm -rf "$REPOSITORYDIR/Application/Hugin.app"
 rm -rf "$REPOSITORYDIR/Application/HuginStichProject.app"
 
 cp -R "$REPOSITORYDIR/arch/$ARCH/Application/Hugin.app" "$REPOSITORYDIR/Application/"
 cp -R "$REPOSITORYDIR/arch/$ARCH/Application/HuginStitchProject.app" "$REPOSITORYDIR/Application/"
 break;
done


for program in Application/Hugin.app/Contents/MacOS/Hugin Application/HuginStitchProject.app/Contents/MacOS/HuginStitchProject
do

 if [ $NUMARCH -eq 1 ]
 then
  mv "$REPOSITORYDIR/arch/$ARCHS/$program" "$REPOSITORYDIR/$program";
  strip "$REPOSITORYDIR/$program";
  continue
 fi

 LIPOARGs=""

 for ARCH in $ARCHS
 do
  LIPOARGs="$LIPOARGs $REPOSITORYDIR/arch/$ARCH/$program"
 done

 lipo $LIPOARGs -create -output "$REPOSITORYDIR/$program";

 strip "$REPOSITORYDIR/$program";

done

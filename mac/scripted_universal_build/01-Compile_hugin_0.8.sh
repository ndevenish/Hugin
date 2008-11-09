# --------------------
#   Hugin 0.8.0
# --------------------
# $Id: 01-compile_hugin.sh 1905 2007-02-05 00:11:26Z ippei $
# script skeleton by Ippei Ukai
# cmake part Harry van der Wolf

# NOTE: This script needs to be run from the hugin source directory

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

 # We do not want any interruption from pkg-configs, wx-configs and so on
 # from MacPorts or Fink, who's paths were added to the PATH
 #export PATH=/bin:/sbin:/usr/bin:/usr/sbin:$REPOSITORYDIR/bin

 # If you are on Tiger and you have dowloaded the binary subversion from
 # Martin Ott's webpage, you also need to add /usr/local/bin
 # In that case uncomment the line below and outcomment the above PATH statement
 export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:$REPOSITORYDIR/bin

# export CMAKE_INCLUDE_PATH="$CMAKE_INCLUDE_PATH:$REPOSITORYDIR/lib/wx/include/mac-unicode-release-2.8";

  export PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig";
# env PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig" \
#  CFLAGS="-isysroot $MACSDKDIR -mmacosx-version-min=10.4 -arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include " \
#  CXXFLAGS="-isysroot $MACSDKDIR -mmacosx-version-min=10.4 -arch $ARCH $ARCHARGs $OTHERARGs -dead_strip" \
#  CPPFLAGS="-isysroot $MACSDKDIR -mmacosx-version-min=10.4 -I$REPOSITORYDIR/include -I$REPOSITORYDIR/include/OpenEXR" \
#  LDFLAGS="-L$REPOSITORYDIR/lib -dead_strip -Wl,-syslibroot -Wl,$MACSDKDIR/" \
#  NEXT_ROOT="$MACSDKDIR" \
#;

cmake  \
  -DCMAKE_BUILD_TYPE:STRING="Release" \
  -DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR/arch/$ARCH" \
  -DCMAKE_OSX_ARCHITECTURES="$TARGET" \
  -DCMAKE_OSX_SYSROOT="$MACSDKDIR" \
  -DCMAKE_C_FLAGS:STRING="-isysroot $MACSDKDIR -mmacosx-version-min=10.4 -arch $ARCH -O2 -I/usr/include -I$REPOSITORYDIR/include" \
  -DCMAKE_CXX_FLAGS:STRING="-isysroot $MACSDKDIR -mmacosx-version-min=10.4 -arch $ARCH -O2 -I/usr/include -I$REPOSITORYDIR/include" \
  -DCMAKE_LD_FLAGS:STRING="-L/usr/lib -L$REPOSITORYDIR/lib -Wl,-syslibroot -Wl,$MACSDKDIR/ -mmacosx_version_min=10.4 -flat_namespace -undefined suppress -bind_at_load" \
  -DEXIV2_INCLUDE_DIR=$REPOSITORYDIR/include \
  -DEXIV2_LIBRARIES=$REPOSITORYDIR/lib/libexiv2.a \
  -DOPENEXR_INCLUDE_DIR=$REPOSITORYDIR/include/OpenEXR \
  ..;

# ldflags -bind_at_load -flat_namespace -undefined suppress

#cmake  \
#  -DCMAKE_BUILD_TYPE:STRING="Release" \
#  -DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR/arch/$ARCH" \
#  -DCMAKE_OSX_ARCHITECTURES="$TARGET" \
#  -DCMAKE_C_FLAGS:STRING="-mmacosx-version-min=10.4 -arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \
#  -DCMAKE_CXX_FLAGS:STRING="-mmacosx-version-min=10.4 -arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \
#  -DCMAKE_LD_FLAGS:STRING="-L/usr/lib -L$REPOSITORYDIR/lib -dead_strip" \
#  -DEXIV2_INCLUDE_DIR=$REPOSITORYDIR/include \
#  -DEXIV2_LIBRARIES=$REPOSITORYDIR/lib/libexiv2.a \
#  -DOPENEXR_INCLUDE_DIR=$REPOSITORYDIR/include/OpenEXR \
#  ..;

cp src/hugin1/hugin/CMakeFiles/Hugin.dir/build.make src/hugin1/hugin/CMakeFiles/Hugin.dir/build.make.$ARCH
sed  's/-l'$ARCH'/'$ARCH'/g' src/hugin1/hugin/CMakeFiles/Hugin.dir/build.make.$ARCH > src/hugin1/hugin/CMakeFiles/Hugin.dir/build.make

cp src/hugin1/nona_gui/CMakeFiles/nona_gui.dir/build.make src/hugin1/nona_gui/CMakeFiles/nona_gui.dir/build.make.$ARCH
sed  's/-l'$ARCH'/'$ARCH'/g' src/hugin1/nona_gui/CMakeFiles/nona_gui.dir/build.make.$ARCH > src/hugin1/nona_gui/CMakeFiles/nona_gui.dir/build.make

cp src/hugin1/stitch_project/CMakeFiles/HuginStitchProject.dir/build.make src/hugin1/stitch_project/CMakeFiles/HuginStitchProject.dir/build.make.$ARCH
sed  's/-l'$ARCH'/'$ARCH'/g' src/hugin1/stitch_project/CMakeFiles/HuginStitchProject.dir/build.make.$ARCH > src/hugin1/stitch_project/CMakeFiles/HuginStitchProject.dir/build.make

cp src/PTBatcher/CMakeFiles/PTBatcher.dir/build.make src/PTBatcher/CMakeFiles/PTBatcher.dir/build.make.$ARCH
sed  's/-l'$ARCH'/'$ARCH'/g' src/PTBatcher/CMakeFiles/PTBatcher.dir/build.make.$ARCH > src/PTBatcher/CMakeFiles/PTBatcher.dir/build.make

cp src/PTBatcherGUI/CMakeFiles/PTBatcherGUI.dir/build.make src/PTBatcherGUI/CMakeFiles/PTBatcherGUI.dir/build.make.$ARCH
sed -e 's/-l'$ARCH'/'$ARCH'/g' src/PTBatcherGUI/CMakeFiles/PTBatcherGUI.dir/build.make.$ARCH > src/PTBatcherGUI/CMakeFiles/PTBatcherGUI.dir/build.make



 

#   -DEXIV2_LIBRARIES=$REPOSITORYDIR/lib/libexiv2.dylib \
#   -DCMAKE_LDFLAGS="-L/usr/lib -L$REPOSITORYDIR/lib -dead_strip -prebind -arch $ARCH -liconv" \

#  -DCMAKE_C_FLAGS_RELEASE:STRING="-isysroot $MACSDKDIR -mmacosx-version-min=10.4 -arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \
#  -DCMAKE_CXX_FLAGS_RELEASE:STRING="-isysroot $MACSDKDIR -mmacosx-version-min=10.4 -arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \


  #  copy preconfigured hugin_config_mac.h to hugin_config.h but save cmake created one
  cp "src/hugin_config.h" "src/hugin_config.h.cmake"
  cp "../src/hugin_config_mac.h" "src/hugin_config.h"

read key

  make;
  make install;
  
  cd ..
read key
done

# Correct install_names in Hugin and HuginStitchProject binaries
for ARCH in $ARCHS
do 
  echo "$ARCH"
  bins="$REPOSITORYDIR/arch/$ARCH/Application/Hugin.app/Contents/MacOS/Hugin $REPOSITORYDIR/arch/$ARCH/Application/HuginStitchProject.app/Contents/MacOS/HuginStitchProject $REPOSITORYDIR/arch/$ARCH/Application/PTBatcherGUI.app/Contents/MacOS/PTBatcherGUI"
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

# Correct install_names in nona, hugin_hdrmerge, celeste_standalone, PTBatcher and pto2mk binaries
for ARCH in $ARCHS
do
  echo "$ARCH"
  bins="$REPOSITORYDIR/arch/$ARCH/bin/nona $REPOSITORYDIR/arch/$ARCH/bin/hugin_hdrmerge $REPOSITORYDIR/arch/$ARCH/bin/celeste_standalone $REPOSITORYDIR/arch/$ARCH/bin/PTBatcher $REPOSITORYDIR/arch/$ARCH/bin/pto2mk"
  for exec_file in $bins
  do
    lib_path="`otool -L $exec_file  2>/dev/null | fgrep compatibility | cut -d\( -f1 | grep libhugin`"
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

for program in bin/vig_optimize bin/tca_correct bin/pto2mk bin/nona bin/fulla bin/autooptimiser bin/align_image_stack bin/nona_gui bin/matchpoint bin/hugin_hdrmerge bin/celeste_standalone bin/PTBatcher
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

for liba in lib/libhuginANN.$HUGINVER_FULL.dylib lib/libhuginbase.$HUGINVER_FULL.dylib lib/libhuginjhead.$HUGINVER_FULL.dylib lib/libhuginvigraimpex.$HUGINVER_FULL.dylib lib/libceleste.0.0.dylib lib/libGLEW.1.5.0.dylib
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

if [ -f "$REPOSITORYDIR/lib/libceleste.0.0.dylib" ]
then
 install_name_tool -id "$REPOSITORYDIR/lib/libceleste.0.0.dylib" "$REPOSITORYDIR/lib/libceleste.0.0.dylib"
 ln -sfn libceleste.0.0.dylib $REPOSITORYDIR/lib/libceleste.dylib;
fi


# merge minimal bundles to universal minimal bundle
# echo "merge minimal bundles to universal minimal bundles"



for ARCH in $ARCHS
do
 mkdir -p "$REPOSITORYDIR/Application"
 rm -rf "$REPOSITORYDIR/Application/Hugin.app"
 rm -rf "$REPOSITORYDIR/Application/HuginStichProject.app"
 rm -rf "$REPOSITORYDIR/Application/PTBatcherGUI.app"
 
 cp -R "$REPOSITORYDIR/arch/$ARCH/Application/Hugin.app" "$REPOSITORYDIR/Application/"
 cp -R "$REPOSITORYDIR/arch/$ARCH/Application/HuginStitchProject.app" "$REPOSITORYDIR/Application/"
 cp -R "$REPOSITORYDIR/arch/$ARCH/Application/PTBatcherGUI.app" "$REPOSITORYDIR/Application/"
 break;
done


for program in Application/Hugin.app/Contents/MacOS/Hugin Application/HuginStitchProject.app/Contents/MacOS/HuginStitchProject Application/PTBatcherGUI.app/Contents/MacOS/PTBatcherGUI
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

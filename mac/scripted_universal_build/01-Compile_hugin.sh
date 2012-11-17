# --------------------
#   Hugin 0.7.0
# --------------------
# $Id: 01-compile_hugin.sh 1905 2007-02-05 00:11:26Z ippei $
# script skeleton by Ippei Ukai
# cmake part Harry van der Wolf

# 20121010 hvdw Start versioning this script
# 20121113.0 hvdw Update to hugin 2012 and compile for x86_64
# 20121112.1 hvdw As Cmakes FindBoost always prefers mutlithreaded libs (*mt*) before all other
# boost libs we really need to set DBoost_USE_MULTITHREADED=0 otherwise we can never
# use another file path




# NOTE: This script needs to be run from the hugin top level source directory
# not the scrpts folder and neiter the src folder

# prepare
# SCRIPT_DIR shuld be the full path to where the scripts are located
#SCRIPT_DIR="/Users/Shared/development/hugin_related/hugin/mac/scripted_universal_build"
#source $SCRIPT_DIR/SetEnv.txt;



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

 if [ $ARCH = "i386" -o $ARCH = "i686" ] ; then
   TARGET=$i386TARGET
   MACSDKDIR=$i386MACSDKDIR
   ARCHARGs="$i386ONLYARG"
   OSVERSION="$i386OSVERSION"
   CC=$i386CC
   CXX=$i386CXX
   ARCHFLAG="-m32"
 else [ $ARCH = "x86_64" ] ;
   TARGET=$x64TARGET
   MACSDKDIR=$x64MACSDKDIR
   ARCHARGs="$x64ONLYARG"
   OSVERSION="$x64OSVERSION"
   CC=$x64CC
   CXX=$x64CXX
   ARCHFLAG="-m64"
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
 #export PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:$REPOSITORYDIR/bin

# export CMAKE_INCLUDE_PATH="$CMAKE_INCLUDE_PATH:$REPOSITORYDIR/lib/wx/include/mac-unicode-release-2.8";

 export PKG_CONFIG_PATH="$REPOSITORYDIR/lib/pkgconfig";
 export NEXT_ROOT="$MACSDKDIR";

cmake  \
  -DCMAKE_BUILD_TYPE="Release" \
  -DCMAKE_PREFIX_PATH="$REPOSITORYDIR" \
  -DCMAKE_INSTALL_PREFIX="$REPOSITORYDIR/arch/$ARCH" \
  -DCMAKE_OSX_SYSROOT="$MACSDKDIR"\
  -DCMAKE_C_FLAGS="-isysroot $MACSDKDIR -arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \
  -DCMAKE_CXX_FLAGS="-isysroot $MACSDKDIR -arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include" \
  -DCMAKE_LDFLAGS="-L/usr/lib -L$REPOSITORYDIR/lib -dead_strip -prebind -liconv -mmacosx-version-min=$OSVERSION" \
  -DEXIV2_INCLUDE_DIR=$REPOSITORYDIR/include \
  -DEXIV2_LIBRARIES=$REPOSITORYDIR/lib/libexiv2.dylib \
  -DBoost_USE_MULTITHREADED=0 \
  -DBoost_INCLUDE_DIR=$REPOSITORYDIR/include \
  -DBoost_LIBRARY_DIRS:FILEPATH=$REPOSITORYDIR/lib \
  -DwxWidgets_INCLUDE_DIRS=$REPOSITORYDIR/include \
  -DOPENEXR_INCLUDE_DIR=$REPOSITORYDIR/include/OpenEXR \
  -DBUILD_HSI=0 \
  ..;

#   -DCMAKE_OSX_ARCHITECTURES="$TARGET" \
#  -DCMAKE_C_FLAGS="-arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include -I$REPOSITORYDIR/lib/wx/include/mac-unicode-release-2.8" \
#  -DCMAKE_CXX_FLAGS="-arch $ARCH -O2 -dead_strip -I/usr/include -I$REPOSITORYDIR/include -I$REPOSITORYDIR/lib/wx/include/mac-unicode-release-2.8" \



  #  copy preconfigured hugin_config_mac.h to hugin_config.h but save cmake created one
  cp "src/hugin_config.h" "src/hugin_config.h.cmake"
  cp "../src/hugin_config_mac.h" "src/hugin_config.h"

  # cmake's boost find ALWAYS looks for /opt/local instead of listening to the environment settings
  # so we have to update the CMakeCache.txt
#  mv CMakeCache.txt CMakeCache.txt.org
#  sed -e 's+/opt/local/lib+\$REPOSITORYDIR/lib+g' CMakeCache.txt.org > CMakeCache.txt 
#  sed -i 's+/opt/local/lib+\$REPOSITORYDIR/lib+g' CMakeCache.txt


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

# Correct install_names in nona, hugin_hdrmerge and pto2mk binaries
for ARCH in $ARCHS
do
  echo "$ARCH"
  bins="$REPOSITORYDIR/arch/$ARCH/bin/nona $REPOSITORYDIR/arch/$ARCH/bin/hugin_hdrmerge $REPOSITORYDIR/arch/$ARCH/bin/pto2mk"
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

#!/bin/sh
# 02-Make_bundle.sh
# Harry van der Wolf, 2008

# Set correct environment
source ../ExternalPrograms/scripts/SetEnv-universal.txt
source ./bundle_constants.txt

# take care of build directory
mkdir -p build

# Set some variables
H_app="build/Hugin.app"
H_binary="$H_app/Contents/MacOS/Hugin"
RES_dir="$H_app/Contents/Resources"
HSP_app="$RES_dir/HuginStitchProject.app"
HSP_binary="$HSP_app/Contents/MacOS/HuginStitchProject"
dylib_dir="$REPOSITORYDIR/lib"
old_install_name_dirname="$REPOSITORYDIR/lib"
dylib_install_loc="Libraries"
new_install_name_dirname="@executable_path/../$dylib_install_loc"
exiftoolDir="$EXIFTOOL_DIR"



# Copy minimal bundles to build directory
echo "Copy minimal bundles to build directory"
rm -rf build/Hugin.app

cp -R "$REPOSITORYDIR/Application/Hugin.app" "build"
cp -R "$REPOSITORYDIR/Application/HuginStitchProject.app" "$RES_dir"


# Copy binaries in relevant bundle
H_bins="autopano-sift-c autopano matchpoint panomatic"
HSP_bins="PTmasker PTroller PTblender PTmender enblend enfuse gnumake hugin_hdrmerge nona pto2mk"

#for bins in $H_bins
#do
# echo "Copying  $bins  into bundle"
# cp -Rf $REPOSITORYDIR/bin/$bins "$RES_dir"
#done

for bins in $HSP_bins
do
 echo "Copying  $bins  into bundle"
 cp -Rf $REPOSITORYDIR/bin/$bins "$HSP_app/Contents/MacOS"
done


# Copy necessary libraries into bundle
# Find out libs we need and loop until no changes.
echo ""
echo "Find the libraries we need and loop until no new ones are added."

mkdir -p $H_app/Contents/$dylib_install_loc

a=1
nfiles=0
endl=true
while $endl; do
  echo "Looking for dependencies and copying them into the bundle. Round " $a
  libs="`otool -L $dylib_dir/*  $RES_dir/* $H_binary $HSP_app/Contents/MacOS/*  2>/dev/null | fgrep compatibility | cut -d\( -f1 | grep "$REPOSITORYDIR" | sort | uniq`"
  cp -f $libs $H_app/Contents/$dylib_install_loc
  let "a+=1"  
  nnfiles=`ls $H_app/Contents/$dylib_install_loc | wc -l`
  if [ $nnfiles = $nfiles ]; then
    endl=false
  else
    nfiles=$nnfiles
  fi
done
echo "finished copying necessary libs from $REPOSITORYDIR"



# replace old install_name_dirname to new install_name_dirname
echo "replace old install_name_dirname to new install_name_dirname"
binaries="$H_app/Contents/$dylib_install_loc/*.dylib $RES_dir/* $H_binary $HSP_app/Contents/MacOS/*"

for exec_file in $binaries
do
 
 echo "Processing: $exec_file"
 
 if [[ $exec_file = *.dylib ]]
 then
  for lib in $(otool -D $exec_file | grep $old_install_name_dirname | sed -e 's/ (.*$//' -e 's/^.*\///')
  do
   echo " Changing own install name."
   install_name_tool -id "$new_install_name_dirname/$lib" $exec_file
  done
 fi
 
 for lib in $(otool -L $exec_file | grep $old_install_name_dirname | sed -e 's/ (.*$//' -e 's/^.*\///')
 do
  echo " Changing install name for: $lib"
  install_name_tool -change "$old_install_name_dirname/$lib" "$new_install_name_dirname/$lib" $exec_file
 done

done

# Copy "control point" scripts into bundle
H_scripts="autopano-complete-mac.sh matchpoint-complete-mac.sh"
#for scripts in $H_scripts
#do
# echo "Copying  $scripts  into bundle"
# cp -Rf "../../../mac/$scripts" "$RES_dir"
#done

# Copy Exiftool into the bundle
echo ""
echo "# Now copying Exiftool into the bundle. There is no error checking. If exiftool is"
echo "# not available your bundle will function just as well but without that functionality."
echo ""
rm -rf $RES_dir/ExifTool;
mkdir -p $RES_dir/ExifTool;

cp $exiftoolDir/exiftool $RES_dir/ExifTool/;
cp -r $exiftoolDir/lib $RES_dir/ExifTool/;



# Embed HuginStitchProject inside Hugin
rm -Rf "$HSP_app/Contents/$dylib_install_loc"
ln -s "../../../$dylib_install_loc" "$HSP_app/Contents/$dylib_install_loc"

if (file "$HSP_app/Contents/Resources" | grep -q -v "symbolic link")
then
 cp -r "$HSP_app/Contents/Resources"/* "$H_app/Contents/Resources/"
fi
rm -Rf "$HSP_app/Contents/Resources"
ln -s "../../../Resources" "$HSP_app/Contents/Resources"

# Copy Info.plists into bundle
cp ../Hugin-Info.plist $H_app/Contents/Info.plist
cp ../hugin_stitch_project-Info.plist $HSP_app/Contents/Info.plist

# Create PKGInfo files
echo "APPLHgin" > $H_app/Contents/PkgInfo
echo "APPL????" > $HSP_app/contents/PkgInfo


#!/bin/sh

# $Id: copyExtraTools.sh $

# This script will copy all extra tools to the HuginStitchProject.app MacOS folder

bin_dir="$REPOSITORY_DIR/bin"
App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"


#projectbins="celeste_standalone pano_trafo vig_optimize autooptimiser cpclean fulla pto2mk calibrate_lens deghosting_mask matchpoint tca_correct pano_modify pto_merge icpfind"
Extbins="PTBlender PTcrop PTinfo PTmasker PTmender PToptimizer PTroller PTtiff2psd PTtiffdump PTuncrop exiv2 gnumake enblend enfuse"

# Copy project binaries to MacOS folder in HuginStitchProject.app
#for bins in $projectbins
#do
# cp -Rf $TARGET_BUILD_DIR/$bins "$App/Contents/MacOS/"
#done

# Copy external binaries to MacOS folder in HuginStitchProject.app from Repositorydir folder
for bins in $Extbins
do
 cp -Rf $bin_dir/$bins "$App/Contents/MacOS/"
done


#!/bin/sh

# $Id: copyTools.sh $

# This script will copy all necessary tools to either the MacOS folder
# inside Hugin.app or to the HuginStitchProject.app MacOS folder

bin_dir="$REPOSITORY_DIR/bin"
App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"
HSPAppPath="$App/Contents/Resources/HuginStitchProject.app/Contents/MacOS"


Hbins="celeste_standalone pano_trafo vig_optimize autooptimiser cpclean fulla pto2mk calibrate_lens deghosting_mask matchpoint tca_correct pano_modify pto_merge"
HSPbins="PTmender enblend PTblender PTroller enfuse PTmasker gnumake exiv2"
HSPBins2="hugin_hdrmerge nona cplean"

# Copy to MacOS folder in Hugin.app
for bins in $Hbins
do
 cp -Rf $TARGET_BUILD_DIR/$bins "$App/Contents/MacOS/"
done

# Copy to MacOS folder in HuginStitchProject.app from External folder
for bins in $HSPbins
do
 cp -Rf $bin_dir/$bins "$HSPAppPath"
done

# Copy to MacOS folder in HuginStitchProject.app from Build dir
for bins in $HSPbins2
do
 cp -Rf $TARGET_BUILD_DIR/$bins "$HSPAppPath"
done


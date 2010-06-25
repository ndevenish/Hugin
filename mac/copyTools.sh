#!/bin/sh

# $Id: copyTools.sh $

# This script will copy the necessary tools to the MacOS folder
# inside the HuginStitchProject.app
# This copy step has been removed from the project itself to facilitate
# the 32bit and 32/64bit builds from different folders

bin_dir="$REPOSITORY_DIR/bin"
App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"

HSPbins="PTBlender PTmasker PTmender PToptimizer PTroller gnumake enblend enfuse"

# Copy to MacOS folder in HuginStitch.app
for bins in $HSPbins
do
 cp -Rf $bin_dir/$bins "$App/Contents/MacOS/"
done


#!/bin/sh

# $Id: copyPTBatcherGuibin.sh $

# This script will copy the necessary binaries to the MacOS folder
# inside PTBatcherGui.app
# This replaces the original copy step in XCode to facilate 32bits builds
# as well as 32/64bit builds.

bin_dir="$REPOSITORY_DIR/bin"
App="$TARGET_BUILD_DIR/$PRODUCT_NAME.app"


extbins="PTblender PTmasker PTmender PToptimizer PTroller enfuse enblend gnumake"

# Copy external binaries to MacOS folder in PTBatcherGui.app
for bins in $extbins
do
 cp -Rf $bin_dir/$bins "$App/Contents/MacOS/"
done


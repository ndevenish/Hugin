#!/bin/sh

exiftoolDir="./ExternalPrograms/Image-ExifTool-7.34"
resdir="$TARGET_BUILD_DIR/Hugin.app/Contents/Resources"

rm -rf $resdir/ExifTool;
mkdir -p $resdir/ExifTool;

cp $exiftoolDir/exiftool $resdir/ExifTool/;
cp -r $exiftoolDir/lib $resdir/ExifTool/;
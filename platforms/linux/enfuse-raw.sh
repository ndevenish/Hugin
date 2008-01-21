#!/bin/sh
# Tool to interpret RAW files with the goal to compress the dynamic range
# to 8-bits, while retaining the highlights and enhancing the shadow areas.
# It isn't quite HDR and it isn't quite DRO, but it's close.
# (c) 2008 Simon Oosthoek (Licenced under GPL v2 or later)

raw=$1
wd=`pwd`

# assuming the RAW file doesn't have dots in the name:
ext=`ls $raw | cut -d '.' -f 2`

echo "assuming RAW extension: $ext"
echo
echo "Select Whitebalance and then save ID-file only (save as, bottom of the dialogue)"

ufraw $raw

base=`basename $raw .$ext`
if [ -d ~/tmp ]; then 
	cd ~/tmp
	#prefer ~/tmp, since you don't want to touch globally shared space if you can avoid it.
else
	cd /tmp
fi

#{dark,normal,bright}.tif should probably be uniquified
ufraw-batch --conf="$wd/$base.ufraw" --create-id=no --out-type=tiff8 --exposure=-1 --output=dark.tif "$wd/$raw"
ufraw-batch --conf="$wd/$base.ufraw" --create-id=no --out-type=tiff8 --exposure=0 --output=normal.tif "$wd/$raw"
ufraw-batch --conf="$wd/$base.ufraw" --create-id=no --out-type=tiff8 --exposure=1 --output=bright.tif "$wd/$raw"
ufraw-batch --conf="$wd/$base.ufraw" --create-id=no --out-type=tiff8 --exposure=2 --output=brighter.tif "$wd/$raw"

enfuse --compression=LZW -o ${base}_fused.tif dark.tif normal.tif bright.tif brighter.tif
rm dark.tif normal.tif bright.tif brighter.tif
mv ${base}_fused.tif "$wd"

cd "$wd"

echo "done"

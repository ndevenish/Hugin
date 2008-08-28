#!/bin/sh
# 04-create_dmg_tgz.sh
# Harry van der Wolf, 2008

# Make a compressed image and gzip that for higher compression

mkdir -p archives

if [ $# -ne 1 ]; then    
    echo ""
    echo 1>&2 "" 
    echo "You need to specify a name for the archive to be created. Please do not add an extention."
    echo "Like ./04-create_dmg_tgz.sh hugin0.7_svn1234_20080810"
    echo ""
    exit 127
fi

archive=$1

echo "# Creating directory $archive in archives. Hugin.app and some more will be copied into this"
echo "# directory and from this directory the $archive.dmg and $archive.dmg.gz will be created"
rm -rf archives/$archive
mkdir -p archives/$archive

# copy Hugin.app
cp -R build/Hugin.app archives/$archive/Hugin.app

# I always copy some Docs and and the relevant License files into the bundle.
# Comment these lines out if you don't want to do that
cd archives
cp -R Docs $archive
cp -R Licenses $archive
cp *.webloc $archive
cp "Read me first (Mac).rtf" $archive
cp "Pre-release Note.txt" $archive

# Temporarily copy autopano-sift-c.autocp in
cp -R Autopano-SIFT $archive 




# Create .dmg from folder
hdiutil create -srcfolder "$archive" "$archive.dmg"

# Gzip to compress it even a little further
gzip -9 "$archive.dmg"

echo "Done. Your $archive.dmg.gz has been created in the directory archives"



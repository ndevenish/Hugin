#!/bin/sh

wget --input-file=pages.txt \
    --base=http://wiki.panotools.org/ \
    --no-host-directories \
    --html-extension \
    --page-requisites \
    --convert-links \
    --no-clobber \
    --no-directories \
    --exclude-directories=wiki/skins \
    --ignore-tags=link

rm -f robots.txt
rm -f index.php*
rm -f gnu-fdl.png
rm -f load.php*
rm -f poweredby_mediawiki_88x31.png


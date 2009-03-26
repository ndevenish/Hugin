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

rm robots.txt
rm index.php*


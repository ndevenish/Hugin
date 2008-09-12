#!/bin/sh

wget --input-file=pages.txt \
    --base=http://wiki.panotools.org/ \
    --no-host-directories \
    --html-extension \
    --page-requisites \
    --convert-links \
    --no-clobber \
    --exclude-directories=wiki/skins \
    --ignore-tags=link



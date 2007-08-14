#!/bin/sh

wget --input-file=pages.txt \
    --base=http://wiki.panotools.org/ \
    --no-host-directories \
    --html-extension \
    --convert-links \
    --no-clobber \
    --exclude-directories=wiki \
    --ignore-tags=link

#    --page-requisites


This directory contains the translations for all translated programs inside the
hugin package. Previously only hugin and nona_gui where translated and carried
their own .po files. This has been changed, and all translateable messages are
now in hugin.pot, located in this directory.

To update the hugin.pot and the translation with new strings found in the
source code, run the extract-messages.sh script.

To ignore a particular string from the xrc files, add it to the
ignored-strings.txt file.

There's a translation guide on the wiki, a must read for all translators:
http://wiki.panotools.org/Hugin_translation_guide


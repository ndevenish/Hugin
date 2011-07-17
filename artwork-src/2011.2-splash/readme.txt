INTRO

This is the source for the 2011.2 splash screen contributed by David Haberthür and released under the creative commons CC-BY-SA license.


BITMAP EXPORT

The bitmap was manually produced because using the script in ./artwork-src/update_artwork.sh results in soft images when bitmaps are resized, most likely at the Inkscape stage.  To work around it:

* Open 2011.2.splash.svg with Inkscape
* Export each layer separately, making sure that the other two layers are invisible.  Export area is page, export at 300 dpi, and name the layers 1.png, 2.png, 3.png from top to bottom.
* Use the following ImageMagick commands to sharpen/merge:
    convert -size 535x254 xc:white -composite 3.png -filter Lanczos -resize 535x254 -gravity center -extent 535x254 3b.png
    convert 2.png -filter Lanczos -resize 535x254 -background transparent -gravity center -extent 535x254 2b.png
    convert 1.png -filter Lanczos -resize 535x254 -background transparent -gravity center -extent 535x254 1b.png
    convert -flatten 3b.png 2b.png 1b.png splash.png
* then move the resulting splash.png image to its intended location.
    mv splash.png ../../src/hugin1/hugin/xrc/data/
#!/bin/bash
# -----------------------------------------------------------------------------
# Copyright (c) 2010, Yuval Levy http://www.photopla.net/
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Yuval Levy nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY Yuval Levy ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Yuval Levy BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# -----------------------------------------------------------------------------
# update_artwork.sh - uses the SVG files in this folder to update_artwork
#                     the graphics artwork throughought the Hugin codebase
# -----------------------------------------------------------------------------
# dependencies
# sudo apt-get install inkscape imagemagick icnsutils netpbm gimp
#
# png2ico, written in 2002 and still going strong http://www.winterdrache.de/freeware/png2ico/
#   wget http://www.winterdrache.de/freeware/png2ico/data/png2ico-src-2002-12-08.tar.gz
#   tar xvzf png2ico-src-2002-12-08.tar.gz
#   cd png2ico
#   make
#   sudo cp png2ico /usr/local/bin
# -----------------------------------------------------------------------------
# TODO: add sanity checks - currently simply assuming that everything is
#       in place
# convert -list formats | grep SVG
# folder containing all source SVG
SRC="2010"
# DEBUG - set to "src" for the production run
#                      set it to anything else to create the artwork in a debug
#                      folder for visual inspection prior to production run
# debugged icons will have full path in their name with _ instead of /
# to avoid overwriting same filenames that goes into different folders
DST="debug"
# you should not have to do any edit below this line if you're just
# editing the graphics
# -----------------------------------------------------------------------------


if [ "${DST}" == "src" ]; then
  # for production run use slashes to separate paths
  S="/"
  # and point to the real root of the codebase
  P="../"
else
  # for debug run use underscore to separate paths
  S="_"
  # points to a debug output folder and create it
  P="../debug/"
  mkdir -p ${P}/
fi
# create a folder for the launchpad artwork folder
mkdir -p ${P}launchpad/
mkdir -p ${P}website/css/

# -----------------------------------------------------------------------------
# Branding
# -----------------------------------------------------------------------------

# get the artwork in from Inkscape
inkscape --file=./${SRC}/splash/splash.svg --export-png=./${SRC}/splash.i.png -z

# make the splash screen 535x254 (the build process customize it later
# with version number
convert  -size 535x254 xc:white -composite ./${SRC}/splash.i.png -resize 535x254 -gravity center -extent 535x254 \
  ${P}src${S}hugin1${S}hugin${S}xrc${S}data${S}splash.png

# the logo in the about window is usually the same as in the splash screen
# so it is simply copied to location
cp ${P}src${S}hugin1${S}hugin${S}xrc${S}data${S}splash.png ${P}src${S}hugin1${S}hugin${S}xrc${S}data${S}logo.png

# output of BMP straight out of ImageMagick convert did not seem right (Dolphin would not show preview
# while it did show preview of all original BMPs)
# piping ImageMagick's output through pngtopnm and ppmtobmp solved the issue
# just add the following before the output file in the original ImageMagick command
# - | pngtopnm | ppmtobmp >

## windows installer branding

# Windows installer branding
convert  -size 128x128 xc:white -composite ./${SRC}/splash.i.png -resize 128x128 -gravity center -extent 128x128 - \
  | pngtopnm | ppmtobmp > ${P}platforms${S}windows${S}huginsetup${S}Graphics${S}Hugin.bmp
convert ./${SRC}/splash.i.png -resize 164x164 -gravity center -extent 164x164 \
  ./${SRC}/splash.164.png
convert  -size 164x164 xc:white -composite ./${SRC}/splash.164.png -gravity north -background white -extent 164x314 - \
  | pngtopnm | ppmtobmp > ${P}platforms${S}windows${S}huginsetup${S}Graphics${S}Hugin-sidebar.bmp

# old Windows InnoSetup installer branding
convert  -size 58x55 xc:white -composite ./${SRC}/splash.i.png -resize 58x55 -gravity center -extent 58x55 - \
  | pngtopnm | ppmtobmp > ${P}platforms${S}windows${S}installer${S}smallimage.bmp
convert  -size 164x164 xc:white -composite ./${SRC}/splash.164.png -gravity south -background white -extent 164x314 - \
  | pngtopnm | ppmtobmp > ${P}platforms${S}windows${S}installer${S}wizardimage.bmp

# very old Windows MSI installer
# banner 493x312 was semitransparent white, but since it has not been used for years...
convert  -size 493x312 xc:white -composite ./${SRC}/splash.i.png -resize 493x312 -gravity center -extent 493x312 - \
  | pngtopnm | ppmtobmp > ${P}platforms${S}windows${S}msi${S}big_banner.bmp
# top banner 493x58, same semitransparency left out...
convert  -size 58x58 xc:white -composite ./${SRC}/splash.i.png -resize 58x58 -gravity east -extent 493x58 - \
  | pngtopnm | ppmtobmp > ${P}platforms${S}windows${S}msi${S}top_banner.bmp

## website branding - the favicons are copied later down from the icons section

# Website branding 128x128 with transparent background
convert  ./${SRC}/splash.i.png -resize 128x128 -gravity center -background transparent -extent 128x128 \
  ${P}website/css/hugin-icon.png
# 2011-July-24 added shadow to the background
convert ${P}website/css/hugin-icon.png '(' +clone -background white -shadow 90x6+6+6 ')' +swap -background none -layers merge +repage ${P}website/css/hugin-shadow.png
# Website branding 48x48 icon with transparent background (where is it used?)
convert  ./${SRC}/splash.i.png -resize 48x48 -gravity center -background transparent -extent 48x48 \
  ${P}website/css/icon.png

## Launchpad branding

# Launchpad branding Icon: 14x14px max 5k
convert ./${SRC}/splash.i.png -resize 14x14 -gravity center -extent 14x14 \
  ${P}launchpad/icon.png

# Launchpad branding Logo: 64x64px max 50k
convert ./${SRC}/splash.i.png -resize 64x64 -gravity center -extent 64x64 \
  ${P}launchpad/logo.png

# Launchpad branding Brand: 192x192px max 100k
convert ./${SRC}/splash.i.png -resize 192x192 -gravity center -extent 192x192 \
  ${P}launchpad/brand.png


# -----------------------------------------------------------------------------
# Iconify - a function to create an icon
# -----------------------------------------------------------------------------
# uses global variables as documented below in the list of calls

function Iconify {
  # get the original artwork from Inkscape
  inkscape --file=./${SRC}/icons/${F}.svg --export-png=./${SRC}/${F}.i.png -z

  # Windows: 10 files go into a .ico file
  # 4bit (16colors) and 8bit (256colors) GIF + 24bit PNG
  # at resolutions of 16x16 32x32 48x48 pixels
  # plus one large PNG at 256x256

  # Mac: only full color PNG - the conversion program takes care of the rest
  # the Mac .icns file has a limited list of acceptable icon sizes http://en.wikipedia.org/wiki/Apple_Icon_Image_format
  # stopped at 128 to mimic existing files, but could add 256 and 512 to the list

  # Linux: TODO: describe

  # lists the images to assemble into the Windows .ico file
  local CONV=""

  # png
  a="i"
  for i in 128 48 32 16;
  do
    # beware of too much sharpening
    # I thought that scaling from the rescaled image would yeld a better result than scaling from scratch
    # but I don't see any difference.  But it is faster.  To try the alternative, replace .${a}.png
    # with .i.png below.
    convert ./${SRC}/${F}.i.png -resize ${i}x${i} -sharpen 1x0.3 ./${SRC}/${F}.${i}.png
    CONV="${CONV} ./${SRC}/${F}.${i}.png"
    a="${i}"
  done

  ## THE WINDOWS ICON ##
  if [ $WINDOWS -eq 1 ]; then
    # 4-bit icons and PNG compression are disabled since GIMP does not allow for save options to scripted
	# sizes are defined in windows-icons-gimp.sh line 85; set number in line 107 to the number of sizes used above 48x48
	# at prompt check the "Compressed (PNG)" box for the largest icon
    local ICONV="./${SRC}/${F}.i.png"
    . ./windows-icons-gimp.sh
  fi

  ## THE LINUX ICON ##
  if [ $LINUX -eq 1 ]; then
    # Linux curently uses only the 48px png. Check how to use SVGZ instead
    cp ./${SRC}/${F}.48.png ${P}${O_LIN}
  fi

  ## THE MAC ICON ##
  if [ $APPLE -eq 1 ]; then
    # generate the Mac icns file before adding to the list
    png2icns ${P}${O_MAC} ${CONV}
  fi

  ## THE WINDOWS ICON WITH PNG2ICO ##
#  if [ $WINDOWS -eq 1 ]; then
    # intermediate icons with png2ico because I could not make ImageMagick work properly with
    # color reduction and transparency (see ImageMagick commands below if you want to fix it)
#    local ICONV=""
#    for i in 48 32 16;
#    do
#      ICONV="${ICONV} ./${SRC}/${F}.${i}.png"
#      a="${i}"
#    done
    # png2ico, written in 2002 and still going strong http://www.winterdrache.de/freeware/png2ico/
#    png2ico ./${SRC}/${F}.4.ico --colors 16 ${ICONV}
#    png2ico ./${SRC}/${F}.8.ico --colors 256 ${ICONV}
    # all together now, with a 128x128 PNG for Vista/7
#    convert ./${SRC}/${F}.4.ico ./${SRC}/${F}.8.ico ./${SRC}/${F}.128.png ${P}${O_WIN}
#  fi

  ## THE WINDOW ICON WITH IMAGEMAGICK ##
  ## TODO: debug to reduce dependencies
  ## THE 4-bit & 8-bit GIFs don't look good.
  # gif
#  for i in 48 32 16;
#  do
    # 4bit
#    convert ./${SRC}/${F}.${i}.png -colors 16 -type TruecolorMatte ./${SRC}/${F}.${i}.4.gif
#    CONV="${CONV} ./${SRC}/${F}.${i}.4.gif"
    # 8bit
#    convert ./${SRC}/${F}.${i}.png -colors 256 -type TruecolorMatte ./${SRC}/${F}.${i}.8.gif
#    CONV="${CONV} ./${SRC}/${F}.${i}.8.gif"
#  done
  # generate the Windows .ico file (-colors 256 reduces it to 8bit which should be enough for an icon)
#  convert ${CONV} -colors 256 ${P}${O_WIN}

}  

# -----------------------------------------------------------------------------
# Call Iconify to generate App & Data Icons
# -----------------------------------------------------------------------------
# SVG files are assumed to be 512x512
# for each icon to generate F is the source SVG filename without extension
# O_WIN, O_MAC. O_LIN are the outputs for Windows Mac and Linux respectively
# with full path excluding the root of the repository
# using ${S} instead of slashes
# switches: set to 1 to produce also linux and apple icons respectively
LINUX=1
APPLE=1
WINDOWS=1

# Icon Hugin
F="icon_hugin"
O_WIN="src${S}hugin1${S}hugin${S}xrc${S}data${S}hugin.ico"
O_MAC="src${S}hugin1${S}hugin${S}Hugin.icns"
O_LIN="src${S}hugin1${S}hugin${S}xrc${S}data${S}hugin.png"
Iconify
# extra website copy
cp "${P}src${S}hugin1${S}hugin${S}xrc${S}data${S}hugin.ico" "${P}website/favicon.ico"

# icon PTBatcher
F="icon_ptbatcher"
O_WIN="src${S}hugin1${S}hugin${S}xrc${S}data${S}ptbatcher.ico"
O_MAC="src${S}hugin1${S}hugin${S}PTBatcherGui.icns"
O_LIN="src${S}hugin1${S}hugin${S}xrc${S}data${S}ptbatcher.png"
Iconify

# icon pto files
F="icon_pto"
O_WIN="src${S}hugin1${S}hugin${S}xrc${S}data${S}pto_icon.ico"
O_MAC="${P}src${S}hugin1${S}hugin${S}HuginFiles.icns"
O_LIN="src${S}hugin1${S}hugin${S}xrc${S}data${S}pto_icon.png"
Iconify
# extra Linux option for the gnome MIME type
cp "./${SRC}/${F}.48.png" "${P}src${S}hugin1${S}hugin${S}gnome-mime-application-x-ptoptimizer-script.png"


# -----------------------------------------------------------------------------
# Windows Installer Icons
# -----------------------------------------------------------------------------

# switches: we no longe need linux and apple versions of the icons
LINUX=0
APPLE=0
WINDOWS=1
# just to make sure that we dont overwrite anything
O_MAC=""
O_LIN=""

# hugin-installer
F="hugin-installer-icon"
O_WIN="platforms${S}windows${S}huginsetup${S}Graphics${S}hugin-installer-icon.ico"
Iconify
# old InnoSetup installer
cp "${P}${O_WIN}" "${P}platforms${S}windows${S}installer${S}hugin.ico"

# hugin-uninstaller
F="hugin-uninstaller-icon"
O_WIN="platforms${S}windows${S}huginsetup${S}Graphics${S}hugin-uninstaller-icon.ico"
Iconify


# -----------------------------------------------------------------------------
# Buttons
# -----------------------------------------------------------------------------

for i in ./${SRC}/buttons/*.svg;
do
  f=${i%.svg}
  g=`basename ${f}`
  inkscape --file=${i} --export-png=${P}src${S}hugin1${S}hugin${S}xrc${S}data${S}${g}.png -z
done

# smaller icons
for g in gl_preview drag_tool identify_tool preview_layout crop_tool;
do
  convert ${P}src${S}hugin1${S}hugin${S}xrc${S}data${S}${g}.png -resize 16x16 ${P}src${S}hugin1${S}hugin${S}xrc${S}data${S}${g}_small.png
done


# -----------------------------------------------------------------------------
# Clean up intermediate files
# -----------------------------------------------------------------------------
rm ./${SRC}/*.png
rm ./${SRC}/*.ico
#rm ./${SRC}/*.gif

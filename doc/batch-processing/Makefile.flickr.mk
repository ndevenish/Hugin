#!/usr/bin/make -f

# hugin Makefile to create stereographic skys and little planets from
# equirectangular enfused projects.  Output is 1280x1280 JPEG images suitable
# for uploading to a free flickr account.  Requires Panotools::Script,
# ImageMagick and nona

# Simple usage:
#   make -f Makefile.flickr.mk flickr PTO=myproject.pto

# FIXME
# Roll pitch yaw for downerect.
# Picking between fused and normal has to be done by resetting FUSED_SUFFIX.
# Doesn't use _SHELL safe paths

JPEG_QUALITY = 90
FLICKR_GEOMETRY = 1280x1280
FUSED_SUFFIX = _fused

EQUIRECT_PREFIX = $(LDR_REMAPPED_PREFIX)$(FUSED_SUFFIX)

include $(PTO).mk

PTO_SGRAPHIC = $(EQUIRECT_PREFIX)-sgraphic.pto
PTO_MERCATOR = $(EQUIRECT_PREFIX)-mercator.pto

SKY_PREFIX = $(EQUIRECT_PREFIX)-sky
PTO_SKY = $(SKY_PREFIX).pto
TIFF_SKY = $(SKY_PREFIX)_0000.tif
JPEG_SKY = $(SKY_PREFIX).jpg

PLANET_PREFIX = $(EQUIRECT_PREFIX)-planet
PTO_PLANET = $(PLANET_PREFIX).pto
TIFF_PLANET = $(PLANET_PREFIX)_0000.tif
JPEG_PLANET = $(PLANET_PREFIX).jpg

MERCATOR_PREFIX = $(EQUIRECT_PREFIX)-mercator
PTO_MERCATOR = $(MERCATOR_PREFIX).pto
TIFF_MERCATOR = $(MERCATOR_PREFIX)_0000.tif
JPEG_MERCATOR = $(MERCATOR_PREFIX).jpg

.PHONY : flickr sky planet flickr_clean
.SECONDARY : $(LDR_EXPOSURE_LAYERS_REMAPPED) $(LDR_STACKS) $(LDR_LAYERS)

flickr : sky planet
sky : $(JPEG_SKY)
planet : $(JPEG_PLANET)
mercator : $(JPEG_MERCATOR)

$(PTO_SGRAPHIC) : $(EQUIRECT_PREFIX).tif
	erect2planet $(EQUIRECT_PREFIX).tif


$(PTO_SKY) : $(PTO_SGRAPHIC)
	transform-pano 0 -90 0 $(PTO_SGRAPHIC) $(PTO_SKY)

$(TIFF_SKY) : $(PTO_SKY)
	$(NONA) -r ldr -m TIFF_m -i 0 -o $(SKY_PREFIX)_ $(PTO_SKY)

$(JPEG_SKY) : $(TIFF_SKY)
	convert -geometry $(FLICKR_GEOMETRY) -quality $(JPEG_QUALITY) $(TIFF_SKY) $(JPEG_SKY)
	$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1) $(EXIFTOOL_COPY_ARGS) $(JPEG_SKY)


$(PTO_PLANET) : $(PTO_SGRAPHIC)
	transform-pano 0 90 0 $(PTO_SGRAPHIC) $(PTO_PLANET)

$(TIFF_PLANET) : $(PTO_PLANET)
	$(NONA) -r ldr -m TIFF_m -i 0 -o $(PLANET_PREFIX)_ $(PTO_PLANET)

$(JPEG_PLANET) : $(TIFF_PLANET)
	convert -geometry $(FLICKR_GEOMETRY) -quality $(JPEG_QUALITY) $(TIFF_PLANET) $(JPEG_PLANET)
	$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1) $(EXIFTOOL_COPY_ARGS) $(JPEG_PLANET)


$(PTO_MERCATOR) : $(EQUIRECT_PREFIX).tif
	erect2mercator $(EQUIRECT_PREFIX).tif

$(TIFF_MERCATOR) : $(PTO_MERCATOR)
	$(NONA) -r ldr -m TIFF_m -i 0 -o $(MERCATOR_PREFIX)_ $(PTO_MERCATOR)

$(JPEG_MERCATOR) : $(TIFF_MERCATOR)
	convert -geometry $(FLICKR_GEOMETRY) -quality $(JPEG_QUALITY) $(TIFF_MERCATOR) $(JPEG_MERCATOR)
	$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1) $(EXIFTOOL_COPY_ARGS) $(JPEG_MERCATOR)


flickr_clean :
	$(RM) -f $(TIFF_SKY) $(TIFF_PLANET) $(TIFF_MERCATOR)


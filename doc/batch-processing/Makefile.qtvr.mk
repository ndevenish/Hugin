#!/usr/bin/make -f

# hugin Makefile to create cubic QTVR panoramas.
# Requires Panotools::Script
# Simple usage:
#   make -f Makefile.qtvr.mk qtvr PTO=myproject.pto

# Advanced (make -e) settings:
#   CUBE_PITCH=-90 # if nadir is in the centre of the input panorama
#   JPEG_QUALITY=50
#   QTVR_NAME='My cool panorama'
#   QTVR_PAN=45
#   QTVR_TILT=30
#   QTVR_FOV=70

JPEG_QUALITY = 70
CUBE_ROLL = 0.0
CUBE_PITCH = 0.0
CUBE_YAW = 0.0
QTVR_NAME = 'Panorama created by hugin'
QTVR_PAN = 0.1
QTVR_TILT = 0.1
QTVR_FOV = 90.0

include $(PTO).mk

CUBE_PREFIX = $(LDR_REMAPPED_PREFIX)_cube
CUBE_PROJECT = $(CUBE_PREFIX).pto
MOV = $(LDR_REMAPPED_PREFIX).mov

JPEG_FACE_0 = $(CUBE_PREFIX)0.jpg
JPEG_FACE_1 = $(CUBE_PREFIX)1.jpg
JPEG_FACE_2 = $(CUBE_PREFIX)2.jpg
JPEG_FACE_3 = $(CUBE_PREFIX)3.jpg
JPEG_FACE_4 = $(CUBE_PREFIX)4.jpg
JPEG_FACE_5 = $(CUBE_PREFIX)5.jpg

JPEG_FACES = $(JPEG_FACE_0) $(JPEG_FACE_1) $(JPEG_FACE_2) $(JPEG_FACE_3) $(JPEG_FACE_4) $(JPEG_FACE_5)

.PHONY : qtvr qtvr_clean

#TODO qtvr_full target, work with fused equirectangular too

qtvr : $(MOV)

$(CUBE_PROJECT) : $(LDR_BLENDED)
	erect2cubic --erect=$(LDR_BLENDED_SHELL) --ptofile=$(CUBE_PROJECT) \
	--filespec="JPEG q$(JPEG_QUALITY)" \
	--roll=$(CUBE_ROLL) --pitch=$(CUBE_PITCH) --yaw=$(CUBE_YAW)

$(JPEG_FACE_0) : $(LDR_BLENDED) $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 0 -o $(JPEG_FACE_0) $(CUBE_PROJECT)
$(JPEG_FACE_1) : $(LDR_BLENDED) $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 1 -o $(JPEG_FACE_1) $(CUBE_PROJECT)
$(JPEG_FACE_2) : $(LDR_BLENDED) $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 2 -o $(JPEG_FACE_2) $(CUBE_PROJECT)
$(JPEG_FACE_3) : $(LDR_BLENDED) $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 3 -o $(JPEG_FACE_3) $(CUBE_PROJECT)
$(JPEG_FACE_4) : $(LDR_BLENDED) $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 4 -o $(JPEG_FACE_4) $(CUBE_PROJECT)
$(JPEG_FACE_5) : $(LDR_BLENDED) $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 5 -o $(JPEG_FACE_5) $(CUBE_PROJECT)

$(MOV) : $(JPEG_FACES)
	jpeg2qtvr --outfile=$(MOV) --prefix=$(CUBE_PREFIX) --name=$(QTVR_NAME) \
	 --pan=$(QTVR_PAN) --tilt=$(QTVR_TILT) --fov=$(QTVR_FOV)

qtvr_clean :
	$(RM) $(JPEG_FACES) $(CUBE_PROJECT)


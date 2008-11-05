# Requires Panotools::Script
# Simple usage:
#   make -f Makefile.equirect.mk equirect_all PTO=myproject.pto

JPEG_QUALITY = 70
GEOMETRY = 1280x1280
CUBE_ROLL = 0.0
CUBE_PITCH = 0.0
CUBE_YAW = 0.0
QTVR_NAME = 'Panorama created by hugin'
QTVR_PAN = 0.1
QTVR_TILT = 0.1
QTVR_FOV = 90.0
FUSED_SUFFIX = _fused

equirect_all : qtvr panosalado sky planet mercator

include $(PTO).mk

.PHONY: equirect_all layered psd qtvr preview panosalado spiv sky planet mercator equirect_clean faces_clean sky_clean planet_clean mercator_clean
.SECONDARY: $(LDR_EXPOSURE_LAYERS_REMAPPED) $(LDR_STACKS) $(LDR_LAYERS)

EQUIRECT_PREFIX = $(LDR_REMAPPED_PREFIX)$(FUSED_SUFFIX)
EQUIRECT_PREFIX_SHELL = $(LDR_REMAPPED_PREFIX_SHELL)$(FUSED_SUFFIX)

# a multilayer TIFF

layered : $(EQUIRECT_PREFIX)_layered.tif

$(LDR_REMAPPED_PREFIX)_layered.tif : $(LDR_BLENDED) $(LDR_LAYERS)
	tiffcp $(LDR_REMAPPED_PREFIX_SHELL).tif $(LDR_LAYERS_SHELL) \
	$(LDR_REMAPPED_PREFIX_SHELL)_layered.tif

$(LDR_REMAPPED_PREFIX)_fused_layered.tif : $(LDR_STACKED_BLENDED) $(LDR_STACKS)
	tiffcp $(LDR_REMAPPED_PREFIX_SHELL)_fused.tif $(LDR_STACKS_SHELL) \
	$(LDR_REMAPPED_PREFIX_SHELL)_fused_layered.tif

# a multilayer PSD

psd : $(EQUIRECT_PREFIX)_layered.psd

$(LDR_REMAPPED_PREFIX)_layered.psd : $(LDR_LAYERS) $(LDR_BLENDED)
	PTTiff2psd -o $(LDR_REMAPPED_PREFIX_SHELL)_layered.psd \
	$(LDR_LAYERS_SHELL) $(LDR_BLENDED_SHELL)

$(LDR_REMAPPED_PREFIX)_fused_layered.psd : $(LDR_STACKS) $(LDR_STACKED_BLENDED)
	PTtiff2psd -o $(LDR_REMAPPED_PREFIX_SHELL)_fused_layered.psd \
	$(LDR_STACKS_SHELL) $(LDR_STACKED_BLENDED_SHELL)

# a set of cubefaces

CUBE_PREFIX = $(EQUIRECT_PREFIX)_cube
CUBE_PREFIX_SHELL = $(EQUIRECT_PREFIX_SHELL)_cube

CUBE_PROJECT = $(CUBE_PREFIX).pto
CUBE_PROJECT_SHELL = $(CUBE_PREFIX_SHELL).pto

JPEG_FACE_0 = $(CUBE_PREFIX)0.jpg
JPEG_FACE_0_SHELL = $(CUBE_PREFIX_SHELL)0.jpg
JPEG_FACE_1 = $(CUBE_PREFIX)1.jpg
JPEG_FACE_1_SHELL = $(CUBE_PREFIX_SHELL)1.jpg
JPEG_FACE_2 = $(CUBE_PREFIX)2.jpg
JPEG_FACE_2_SHELL = $(CUBE_PREFIX_SHELL)2.jpg
JPEG_FACE_3 = $(CUBE_PREFIX)3.jpg
JPEG_FACE_3_SHELL = $(CUBE_PREFIX_SHELL)3.jpg
JPEG_FACE_4 = $(CUBE_PREFIX)4.jpg
JPEG_FACE_4_SHELL = $(CUBE_PREFIX_SHELL)4.jpg
JPEG_FACE_5 = $(CUBE_PREFIX)5.jpg
JPEG_FACE_5_SHELL = $(CUBE_PREFIX_SHELL)5.jpg

JPEG_FACES = $(JPEG_FACE_0) $(JPEG_FACE_1) $(JPEG_FACE_2) \
    $(JPEG_FACE_3) $(JPEG_FACE_4) $(JPEG_FACE_5)
JPEG_FACES_SHELL = $(JPEG_FACE_0_SHELL) $(JPEG_FACE_1_SHELL) \
    $(JPEG_FACE_2_SHELL) $(JPEG_FACE_3_SHELL) \
    $(JPEG_FACE_4_SHELL) $(JPEG_FACE_5_SHELL)


$(CUBE_PROJECT) : $(EQUIRECT_PREFIX).tif
	erect2cubic --erect=$(EQUIRECT_PREFIX_SHELL).tif --ptofile=$(CUBE_PROJECT_SHELL) \
	--filespec="JPEG q$(JPEG_QUALITY)" \
	--roll=$(CUBE_ROLL) --pitch=$(CUBE_PITCH) --yaw=$(CUBE_YAW)

$(JPEG_FACE_0) : $(EQUIRECT_PREFIX).tif $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 0 -o $(JPEG_FACE_0_SHELL) $(CUBE_PROJECT_SHELL)
$(JPEG_FACE_1) : $(EQUIRECT_PREFIX).tif $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 1 -o $(JPEG_FACE_1_SHELL) $(CUBE_PROJECT_SHELL)
$(JPEG_FACE_2) : $(EQUIRECT_PREFIX).tif $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 2 -o $(JPEG_FACE_2_SHELL) $(CUBE_PROJECT_SHELL)
$(JPEG_FACE_3) : $(EQUIRECT_PREFIX).tif $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 3 -o $(JPEG_FACE_3_SHELL) $(CUBE_PROJECT_SHELL)
$(JPEG_FACE_4) : $(EQUIRECT_PREFIX).tif $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 4 -o $(JPEG_FACE_4_SHELL) $(CUBE_PROJECT_SHELL)
$(JPEG_FACE_5) : $(EQUIRECT_PREFIX).tif $(CUBE_PROJECT)
	$(NONA) -p UINT8 -i 5 -o $(JPEG_FACE_5_SHELL) $(CUBE_PROJECT_SHELL)

# a qvtr file

MOV = $(EQUIRECT_PREFIX).mov
MOV_SHELL = $(EQUIRECT_PREFIX_SHELL).mov

$(MOV) : faces
	jpeg2qtvr --outfile=$(MOV_SHELL) --prefix=$(CUBE_PREFIX_SHELL) --name=$(QTVR_NAME) \
	 --pan=$(QTVR_PAN) --tilt=$(QTVR_TILT) --fov=$(QTVR_FOV)

# a SPi-V cube strip

SPIV_CUBE = $(EQUIRECT_PREFIX)-SPiV_cube.jpg
SPIV_CUBE_SHELL = $(EQUIRECT_PREFIX_SHELL)-SPiV_cube.jpg

$(SPIV_CUBE) : $(JPEG_FACES)
	convert $(JPEG_FACES_SHELL) +append $(SPIV_CUBE_SHELL)

# a small JPEG preview

EQUIRECT_PREVIEW = $(EQUIRECT_PREFIX)-preview.jpg
EQUIRECT_PREVIEW_SHELL = $(EQUIRECT_PREFIX_SHELL)-preview.jpg

$(EQUIRECT_PREVIEW) : $(EQUIRECT_PREFIX).tif
	convert -geometry 320x160 $(EQUIRECT_PREFIX_SHELL).tif $(EQUIRECT_PREVIEW_SHELL)

# a very simple PanoSalado XML file

PANOSALADO = $(EQUIRECT_PREFIX)-PanoSalado.xml
PANOSALADO_SHELL = $(EQUIRECT_PREFIX_SHELL)-PanoSalado.xml

$(PANOSALADO) : $(JPEG_FACES) $(EQUIRECT_PREVIEW)
	echo $(XML_PANOSALADO) > $(PANOSALADO_SHELL)

XML_PANOSALADO = '<?xml version="1.0"?>\
<PanoSalado>\
  <layer id="PanoSalado" url="PanoSalado.swf?xml=$(PANOSALADO)" depth="0" onStart="loadSpace:myPreview">\
    <spaces>\
      <space id="myPreview" onTransitionEnd="loadSpace:myPano">\
        <sphere id="myPreviewImage">\
          <file>$(EQUIRECT_PREVIEW)</file>\
        </sphere>\
      </space>\
      <space id="myPano">\
        <cube id="myPanoCubeFaces">\
          <file id="front">$(JPEG_FACE_0)</file>\
          <file id="right">$(JPEG_FACE_1)</file>\
          <file id="back">$(JPEG_FACE_2)</file>\
          <file id="left">$(JPEG_FACE_3)</file>\
          <file id="top">$(JPEG_FACE_4)</file>\
          <file id="bottom">$(JPEG_FACE_5)</file>\
        </cube>\
      </space>\
    </spaces>\
  </layer>\
</PanoSalado>'

# a pto base for stereographic output

PTO_SGRAPHIC = $(EQUIRECT_PREFIX)-sgraphic.pto
PTO_SGRAPHIC_SHELL = $(EQUIRECT_PREFIX_SHELL)-sgraphic.pto

$(PTO_SGRAPHIC) : $(EQUIRECT_PREFIX).tif
	erect2planet $(EQUIRECT_PREFIX_SHELL).tif

# a stereographic view looking up

SKY_PREFIX = $(EQUIRECT_PREFIX)-sky
SKY_PREFIX_SHELL = $(EQUIRECT_PREFIX_SHELL)-sky
PTO_SKY = $(SKY_PREFIX).pto
PTO_SKY_SHELL = $(SKY_PREFIX_SHELL).pto
TIFF_SKY = $(SKY_PREFIX)_0000.tif
TIFF_SKY_SHELL = $(SKY_PREFIX_SHELL)_0000.tif
JPEG_SKY = $(SKY_PREFIX).jpg
JPEG_SKY_SHELL = $(SKY_PREFIX_SHELL).jpg

$(PTO_SKY) : $(PTO_SGRAPHIC)
	transform-pano 0 -90 0 $(PTO_SGRAPHIC_SHELL) $(PTO_SKY_SHELL)

$(TIFF_SKY) : $(PTO_SKY)
	$(NONA) -r ldr -m TIFF_m -i 0 -o $(SKY_PREFIX_SHELL)_ $(PTO_SKY_SHELL)

$(JPEG_SKY) : $(TIFF_SKY)
	convert -geometry $(GEOMETRY) -quality $(JPEG_QUALITY) $(TIFF_SKY_SHELL) $(JPEG_SKY_SHELL)
	$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) \
	$(EXIFTOOL_COPY_ARGS) $(JPEG_SKY_SHELL)

# a stereographic view looking down

PLANET_PREFIX = $(EQUIRECT_PREFIX)-planet
PLANET_PREFIX_SHELL = $(EQUIRECT_PREFIX_SHELL)-planet
PTO_PLANET = $(PLANET_PREFIX).pto
PTO_PLANET_SHELL = $(PLANET_PREFIX_SHELL).pto
TIFF_PLANET = $(PLANET_PREFIX)_0000.tif
TIFF_PLANET_SHELL = $(PLANET_PREFIX_SHELL)_0000.tif
JPEG_PLANET = $(PLANET_PREFIX).jpg
JPEG_PLANET_SHELL = $(PLANET_PREFIX_SHELL).jpg

$(PTO_PLANET) : $(PTO_SGRAPHIC)
	transform-pano 0 90 0 $(PTO_SGRAPHIC_SHELL) $(PTO_PLANET_SHELL)

$(TIFF_PLANET) : $(PTO_PLANET)
	$(NONA) -r ldr -m TIFF_m -i 0 -o $(PLANET_PREFIX_SHELL)_ $(PTO_PLANET_SHELL)

$(JPEG_PLANET) : $(TIFF_PLANET)
	convert -geometry $(GEOMETRY) -quality $(JPEG_QUALITY) $(TIFF_PLANET_SHELL) $(JPEG_PLANET_SHELL)
	$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) \
	$(EXIFTOOL_COPY_ARGS) $(JPEG_PLANET_SHELL)

# a mercator view

MERCATOR_PREFIX = $(EQUIRECT_PREFIX)-mercator
MERCATOR_PREFIX_SHELL = $(EQUIRECT_PREFIX_SHELL)-mercator
PTO_MERCATOR = $(MERCATOR_PREFIX).pto
PTO_MERCATOR_SHELL = $(MERCATOR_PREFIX_SHELL).pto
TIFF_MERCATOR = $(MERCATOR_PREFIX)_0000.tif
TIFF_MERCATOR_SHELL = $(MERCATOR_PREFIX_SHELL)_0000.tif
JPEG_MERCATOR = $(MERCATOR_PREFIX).jpg
JPEG_MERCATOR_SHELL = $(MERCATOR_PREFIX_SHELL).jpg

$(PTO_MERCATOR) : $(EQUIRECT_PREFIX).tif
	erect2mercator $(EQUIRECT_PREFIX_SHELL).tif

$(TIFF_MERCATOR) : $(PTO_MERCATOR)
	$(NONA) -r ldr -m TIFF_m -i 0 -o $(MERCATOR_PREFIX_SHELL)_ $(PTO_MERCATOR_SHELL)

$(JPEG_MERCATOR) : $(TIFF_MERCATOR)
	convert -geometry $(GEOMETRY) -quality $(JPEG_QUALITY) $(TIFF_MERCATOR_SHELL) $(JPEG_MERCATOR_SHELL)
	$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) \
	$(EXIFTOOL_COPY_ARGS) $(JPEG_MERCATOR_SHELL)

# some PHONY targets

faces : $(JPEG_FACES)
qtvr : $(MOV)
spiv : $(SPIV_CUBE)
panosalado : $(PANOSALADO)
preview : $(EQUIRECT_PREVIEW)
sky : $(JPEG_SKY)
planet : $(JPEG_PLANET)
mercator : $(JPEG_MERCATOR)

# cleanup

equirect_clean : faces_clean sky_clean planet_clean mercator_clean

faces_clean :
	$(RM) -f $(JPEG_FACES_SHELL) $(CUBE_PROJECT_SHELL)
sky_clean :
	$(RM) -f $(TIFF_SKY_SHELL)
planet_clean :
	$(RM) -f $(TIFF_PLANET_SHELL)
mercator_clean :
	$(RM) -f $(TIFF_MERCATOR_SHELL)


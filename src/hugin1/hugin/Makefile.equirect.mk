# Requires Panotools::Script
# Simple usage:
#   make -f Makefile.equirect.mk equirect_all PTO=myproject.pto

# requirements:
#
# libtiff (tiffcp)
# libpano13 (PTtiff2psd)
# Panotools::Script (erect2cubic jpeg2qtvr erect2planet transform-pano erect2mercator)
# ImageMagick (convert)
# hugin (nona)
# Image::ExifTool (exiftool)

include $(PTO).mk

JPEG_QUALITY = 70
GEOMETRY = $(HUGIN_WIDTH)x$(HUGIN_WIDTH)

ifdef DO_LDR_BLENDED
FUSED_SUFFIX =
else
FUSED_SUFFIX = _fused
endif

equirect_all : qtvr sky planet mercator

.PHONY: equirect_all layered psd qtvr preview panosalado vrml x3d spiv sky planet mercator equirect_clean faces_clean sky_clean planet_clean mercator_clean
.SECONDARY: $(LDR_EXPOSURE_LAYERS_REMAPPED) $(LDR_STACKS) $(LDR_LAYERS)

EQUIRECT_PREFIX = $(LDR_REMAPPED_PREFIX)$(FUSED_SUFFIX)
EQUIRECT_PREFIX_SHELL = $(LDR_REMAPPED_PREFIX_SHELL)$(FUSED_SUFFIX)

# a multilayer TIFF

layered : $(EQUIRECT_PREFIX)_multilayer.tif

# a multilayer PSD

psd : $(EQUIRECT_PREFIX)_multilayer.psd

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

CUBE_ROLL = 0.0
CUBE_PITCH = 0.0
CUBE_YAW = 0.0

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

QTVR_NAME = 'Panorama created by hugin'
QTVR_PAN = 0.1
QTVR_TILT = 0.1
QTVR_FOV = 90.0

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
PANOSALADO_SWF = PanoSalado.swf

$(PANOSALADO) : $(JPEG_FACES) $(EQUIRECT_PREVIEW)
	echo -e $(XML_PANOSALADO) > $(PANOSALADO_SHELL)

XML_PANOSALADO = '<?xml version="1.0"?>\n\
<PanoSalado>\n\
  <layer id="PanoSalado" url="$(PANOSALADO_SWF)?xml=$(PANOSALADO)" depth="0" onStart="loadSpace:myPreview">\n\
    <spaces>\n\
      <space id="myPreview" onTransitionEnd="loadSpace:myPano">\n\
        <sphere id="myPreviewImage">\n\
          <file>$(EQUIRECT_PREVIEW)</file>\n\
        </sphere>\n\
      </space>\n\
      <space id="myPano">\n\
        <cube id="myPanoCubeFaces">\n\
          <file id="front">$(JPEG_FACE_0)</file>\n\
          <file id="right">$(JPEG_FACE_1)</file>\n\
          <file id="back">$(JPEG_FACE_2)</file>\n\
          <file id="left">$(JPEG_FACE_3)</file>\n\
          <file id="top">$(JPEG_FACE_4)</file>\n\
          <file id="bottom">$(JPEG_FACE_5)</file>\n\
        </cube>\n\
      </space>\n\
    </spaces>\n\
  </layer>\n\
</PanoSalado>'

# a very simple VRML file

VRML = $(EQUIRECT_PREFIX).wrl
VRML_SHELL = $(EQUIRECT_PREFIX_SHELL).wrl

$(VRML) : $(JPEG_FACES)
	echo -e $(TXT_VRML) > $(VRML_SHELL)

TXT_VRML = '\#VRML V2.0 utf8\n\
Background {\n\
 frontUrl  "$(JPEG_FACE_0)"\n\
 rightUrl  "$(JPEG_FACE_1)"\n\
 backUrl   "$(JPEG_FACE_2)"\n\
 leftUrl   "$(JPEG_FACE_3)"\n\
 topUrl    "$(JPEG_FACE_4)"\n\
 bottomUrl "$(JPEG_FACE_5)"\n\
}'

# a very simple X3D file

X3D = $(EQUIRECT_PREFIX).x3d
X3D_SHELL = $(EQUIRECT_PREFIX_SHELL).x3d

$(X3D) : $(JPEG_FACES)
	echo -e $(XML_X3D) > $(X3D_SHELL)

XML_X3D = '<?xml version="1.0" encoding="UTF-8"?>\n<!DOCTYPE\
 X3D PUBLIC "ISO//Web3D//DTD X3D 3.0//EN"\
 "http://www.web3d.org/specifications/x3d-3.0.dtd">\n\
<X3D version="3.0" profile="Immersive"\
 xmlns:xsd="http://www.w3.org/2001/XMLSchema-instance"\
 xsd:noNamespaceSchemaLocation="http://www.web3d.org/specifications/x3d-3.0.xsd">\n\
  <Scene>\n\
    <Background\
     frontUrl="$(JPEG_FACE_0)"\
     rightUrl="$(JPEG_FACE_1)"\
     backUrl="$(JPEG_FACE_2)"\
     leftUrl="$(JPEG_FACE_3)"\
     topUrl="$(JPEG_FACE_4)"\
     bottomUrl="$(JPEG_FACE_5)" />\n\
  </Scene>\n\
</X3D>'

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
	convert -quality $(JPEG_QUALITY) $(TIFF_SKY_SHELL) $(JPEG_SKY_SHELL)
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
	convert -quality $(JPEG_QUALITY) $(TIFF_PLANET_SHELL) $(JPEG_PLANET_SHELL)
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
	convert -quality $(JPEG_QUALITY) $(TIFF_MERCATOR_SHELL) $(JPEG_MERCATOR_SHELL)
	$(EXIFTOOL) -overwrite_original_in_place -TagsFromFile $(INPUT_IMAGE_1_SHELL) \
	$(EXIFTOOL_COPY_ARGS) $(JPEG_MERCATOR_SHELL)

# This Makefile is only useful for equirectangular
ifeq ($(HUGIN_PROJECTION),2)
faces : $(JPEG_FACES)
qtvr : $(MOV)
spiv : $(SPIV_CUBE)
panosalado : $(PANOSALADO)
vrml : $(VRML)
x3d : $(X3D)
preview : $(EQUIRECT_PREVIEW)
sky : $(JPEG_SKY)
planet : $(JPEG_PLANET)
mercator : $(JPEG_MERCATOR)
else
equirect_all faces qtvr spiv panosalado vrml x3d preview sky planet mercator :
endif

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


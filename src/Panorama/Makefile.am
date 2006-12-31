# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I$(top_builddir)/src/include -I${top_srcdir}/src/include -I${top_srcdir}/src/foreign $(BOOST_CPPFLAGS) $(PANO_FLAGS)
AM_LDFLAGS = $(LIB_TIFF) $(LIB_PANO)

noinst_LTLIBRARIES = libpanorama.la
libpanorama_la_SOURCES = Panorama.cpp PanoramaMemento.cpp PanoImage.cpp PanoToolsInterface.cpp SpaceTransform.cpp PTOptimise.cpp ImageGraph.cpp Stitcher.cpp Stitcher1.cpp Stitcher2.cpp Stitcher3.cpp Stitcher4.cpp utils.cpp

EXTRA_DIST=Panorama.vcproj OptimizeOptions.cpp
noinst_HEADERS =

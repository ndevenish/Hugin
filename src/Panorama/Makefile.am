# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I${top_srcdir}/src/include -I${top_srcdir}/src/foreign

noinst_LTLIBRARIES = libpanorama.la
libpanorama_la_SOURCES = Panorama.cpp PanoramaMemento.cpp PanoImage.cpp PanoToolsInterface.cpp SpaceTransform.cpp PTOptimise.cpp ImageGraph.cpp Stitcher.cpp

EXTRA_DIST=Panorama.vcproj OptimizeOptions.cpp
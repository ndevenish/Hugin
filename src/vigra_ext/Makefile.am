# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I${top_srcdir}/src/include -I${top_srcdir}/src/foreign

noinst_LTLIBRARIES = libvigra_ext.la
libvigra_ext_la_SOURCES = LoweSIFT.cpp PointMatching.cpp

EXTRA_DIST = vigra_ext.vcproj

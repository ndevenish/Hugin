# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I$(top_builddir)/src/include -I${top_srcdir}/src/include -I${top_srcdir}/src/foreign
AM_LDFLAGS = $(LIB_JPEG) $(LIB_PNG) $(LIB_TIFF) $(LIB_ZLIB)

noinst_LTLIBRARIES = libSIFT.la
libSIFT_la_SOURCES = ScaleSpace.cpp ScaleSpaceTools.cpp

bin_PROGRAMS = sift_keypoints
sift_keypoints_SOURCES = sift_keypoints.cpp
sift_keypoints_LDADD = libSIFT.la ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la

EXTRA_DIST = LICENSE

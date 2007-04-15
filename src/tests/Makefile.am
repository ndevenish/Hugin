# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES    = -I$(top_builddir)/src/include -I${top_srcdir}/src/foreign -I${top_srcdir}/src/include
AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(JPEG_FLAGS) $(TIFF_FLAGS) $(PANO_FLAGS)
AM_LDFLAGS  = $(LIB_JPEG) $(LIB_TIFF) $(LIB_PANO) ${top_builddir}/src/Panorama/libpanorama.la \
                           $(BOOST_THREAD_LIB) \
                           ${top_builddir}/src/common/libcommon.la \
 			   ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
 		           ${top_builddir}/src/jhead/libjhead.la \
 			   ${top_builddir}/src/vigra_ext/libvigra_ext.la

bin_PROGRAMS = test_projections vigra_ext_lut

test_projections_SOURCES = test_projections.cpp

vigra_ext_lut_SOURCES = vigra_ext_lut.cpp
vigra_ext_lut_LDADD = -lboost_unit_test_framework 

EXTRA_DIST = test_projections.vcproj

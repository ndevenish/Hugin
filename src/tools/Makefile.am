# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES    = -I$(top_builddir)/src/include -I${top_srcdir}/src/foreign -I${top_srcdir}/src/include
AM_CPPFLAGS = $(BOOST_CPPFLAGS) $(JPEG_FLAGS) $(TIFF_FLAGS) $(PANO_FLAGS) $(OPENEXR_CFLAGS)
AM_LDFLAGS  = $(LIB_JPEG) $(LIB_TIFF) $(LIB_PANO) $(BOOST_THREAD_LIB) $(OPENEXR_LIBS)

bin_PROGRAMS = align_image_stack nona vig_optimize autooptimiser fulla 
#zhang_undistort color_correct_tiff 

img2vips_SOURCES = img2vips.cpp
img2vips_CPPFLAGS = $(VIPS_CFLAGS)
img2vips_LDADD = ${top_builddir}/src/common/libcommon.la \
                 ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
                 ${top_builddir}/src/vigra_ext/libvigra_ext.la \
                 $(VIPS_LIBS)

nona_SOURCES = nona.cpp
nona_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la \
               ${top_builddir}/src/common/libcommon.la \
               ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
               ${top_builddir}/src/jhead/libjhead.la \
               ${top_builddir}/src/vigra_ext/libvigra_ext.la

#automatch_SOURCES = automatch.cpp
#automatch_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
#	                ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la \
#				    ${top_builddir}/src/klt/libklt.la

autooptimiser_SOURCES =  autooptimiser.cpp
autooptimiser_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
	                    ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la

zhang_undistort_SOURCES = zhang_undistort.cpp
zhang_undistort_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
	                      ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la \
                              ${top_builddir}/src/vigra_ext/libvigra_ext.la \
                              $(BOOST_THREAD_LIB)

color_correct_tiff_SOURCES = color_correct_tiff.cpp
color_correct_tiff_LDADD   = $(LIB_TIFF)

fulla_SOURCES        = fulla.cpp
fulla_LDADD          = ${top_builddir}/src/Panorama/libpanorama.la $(BOOST_THREAD_LIB) \
                       ${top_builddir}/src/common/libcommon.la \
                       ${top_builddir}/src/jhead/libjhead.la \
                       ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
                       ${top_builddir}/src/vigra_ext/libvigra_ext.la

vig_optimize_SOURCES = vig_optimize.cpp
vig_optimize_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la $(BOOST_THREAD_LIB) \
                       ${top_builddir}/src/common/libcommon.la \
                       ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
                       ${top_builddir}/src/jhead/libjhead.la \
                       ${top_builddir}/src/vigra_ext/libvigra_ext.la \
                       ${top_builddir}/src/foreign/levmar/liblevmar.la

align_image_stack_SOURCES = align_image_stack.cpp
align_image_stack_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la $(BOOST_THREAD_LIB) \
			 ${top_builddir}/src/common/libcommon.la \
			 ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la \
			 ${top_builddir}/src/jhead/libjhead.la \
			 ${top_builddir}/src/vigra_ext/libvigra_ext.la
												    


# DGSW remove automatch.cpp from the list below if you uncomment automatch_SOURCES and automatch_LDADD
EXTRA_DIST = nona.vcproj automatch.cpp

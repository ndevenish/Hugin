# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES    = -I$(top_builddir)/src/include -I${top_srcdir}/src/foreign -I${top_srcdir}/src/include
AM_CPPFLAGS = $(BOOST_CFLAGS) $(JPEG_FLAGS) $(TIFF_FLAGS) $(PANO_FLAGS)
AM_LDFLAGS  = $(LIB_JPEG) $(LIB_TIFF) $(LIB_PANO)

bin_PROGRAMS = nona autopano_old panosifter autooptimiser zhang_undistort

nona_SOURCES = nona.cpp
nona_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
               ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la

autopano_old_SOURCES = autopano.cpp
autopano_old_CFLAGS  = $(FFTW_INCLUDES)
autopano_old_LDFLAGS = $(LIB_FFTW)
autopano_old_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
                       ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la \
		       ${top_builddir}/src/klt/libklt.la

automatch_SOURCES = automatch.cpp
automatch_CFLAGS  = $(FFTW_INCLUDES)
automatch_LDFLAGS = $(LIB_FFTW)
automatch_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la \
                    ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la \
		    ${top_builddir}/src/klt/libklt.la

panosifter_SOURCES = panosifter.cpp
panosifter_CFLAGS  = $(PNG_FLAGS) $(ZLIB_FLAGS)
panosifter_LDFLAGS = $(LIB_PNG) $(LIB_ZLIB)
panosifter_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la ${top_builddir}/src/vigra_ext/libvigra_ext.la \
                     ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la \
		     ${top_builddir}/src/klt/libklt.la

autooptimiser_SOURCES =  autooptimiser.cpp
autooptimiser_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la ${top_builddir}/src/vigra_ext/libvigra_ext.la \
                        ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la

zhang_undistort_SOURCES = zhang_undistort.cpp
zhang_undistort_LDADD   = ${top_builddir}/src/Panorama/libpanorama.la ${top_builddir}/src/common/libcommon.la ${top_builddir}/src/vigra_ext/libvigra_ext.la \
                          ${top_builddir}/src/foreign/vigra_impex/libvigra_impex.la ${top_builddir}/src/jhead/libjhead.la


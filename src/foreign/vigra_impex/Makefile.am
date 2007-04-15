# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I$(top_builddir)/src/include -I${top_srcdir}/src/foreign
AM_CXXFLAGS = $(JPEG_FLAGS) $(PNG_FLAGS) $(TIFF_FLAGS) -DHasEXR -I/usr/include/OpenEXR
AM_CFLAGS = $(JPEG_FLAGS) $(PNG_FLAGS) $(TIFF_FLAGS) $(OPENEXR_CFLAGS)
AM_LDFLAGS = $(LIB_JPEG) $(LIB_PNG) $(LIB_TIFF) $(OPENEXR_LIBS)

noinst_LTLIBRARIES = libvigra_impex.la
libvigra_impex_la_SOURCES = bmp.cxx byteorder.cxx codecmanager.cxx gif.cxx imageinfo.cxx jpeg.cxx png.cxx pnm.cxx sun.cxx tiff.cxx viff.cxx void_vector.cxx iccjpeg.c rgbe.c hdr.cxx exr.cxx
noinst_HEADERS = auto_file.hxx bmp.hxx byteorder.hxx codecmanager.hxx error.hxx gif.hxx jpeg.hxx png.hxx pnm.hxx sun.hxx tiff.hxx viff.hxx void_vector.hxx iccjpeg.h hdr.hxx rgbe.h exr.hxx

EXTRA_DIST = vigra_impex.vcproj

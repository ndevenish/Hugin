# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I${top_srcdir}/src/foreign -I${top_srcdir}/src/foreign/vigra
AM_CXXFLAGS = $(JPEG_FLAG) $(PNG_FLAG) $(TIFF_FLAG)

noinst_LTLIBRARIES = libvigra_impex.la
libvigra_impex_la_SOURCES = bmp.cxx byteorder.cxx codecmanager.cxx gif.cxx imageinfo.cxx jpeg.cxx png.cxx pnm.cxx sun.cxx tiff.cxx viff.cxx void_vector.cxx
noinst_HEADERS = auto_file.hxx bmp.hxx byteorder.hxx codecmanager.hxx error.hxx gif.hxx jpeg.hxx png.hxx pnm.hxx sun.hxx tiff.hxx viff.hxx void_vector.hxx

EXTRA_DIST = vigra_impex.vcproj

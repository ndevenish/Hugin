# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I$(top_builddir)/src/include -I${top_srcdir}/src/include

noinst_LTLIBRARIES = libjhead.la
libjhead_la_SOURCES = exif.cpp jpgfile.cpp

EXTRA_DIST = jhead.vcproj

# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I$(top_builddir)/src/include -I${top_srcdir}/src/include -I${top_srcdir}/src/foreign

AM_CXXFLAGS = $(GTK_CFLAGS) $(PANO_FLAGS) $(BOOST_CPPFLAGS)

noinst_LTLIBRARIES = libvigra_ext.la
libvigra_ext_la_SOURCES = MultiThreadOperations.cpp

EXTRA_DIST = vigra_ext.vcproj

# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I$(top_builddir)/src/include -I${top_srcdir}/src/include

noinst_LTLIBRARIES = libcommon.la
libcommon_la_SOURCES = graph.cpp utils.cpp PTLensDB.c lu.c eig_jacobi.cpp

EXTRA_DIST = common.vcproj
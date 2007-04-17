# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I$(top_builddir)/src/include -I${top_srcdir}/src/include

noinst_LTLIBRARIES = liblevmar.la
liblevmar_la_SOURCES = lm.c Axb.c misc.c lmlec.c lmbc.c
noinst_HEADERS = misc.h lm.h
EXTRA_DIST = misc_core.c Axb_core.c lmbc_core.c lm_core.c lmdemo.c lmlec_core.c levmar.vcproj README.txt LICENSE CMakeLists.txt Makefile.vc

# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

INCLUDES = -I${top_srcdir}/src/include

noinst_LTLIBRARIES = libklt.la
libklt_la_SOURCES = error.c convolve.c klt.c klt_util.c pyramid.c selectGoodFeatures.c storeFeatures.c trackFeatures.c writeFeatures.c pnmio.c

EXTRA_DIST = track_saved.c
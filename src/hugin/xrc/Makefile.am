# Over-ride default includes so that we have finer control in the makefiles
DEFAULT_INCLUDES="-I."

SUBDIRS=data

XRC_FILES = $(wildcard $(srcdir)/*.xrc)
xrcdir = @datadir@/hugin/xrc

all-local:
	xrclist='$(XRC_FILES)'; \
	for p in $$xrclist; do \
	    if test -e `basename $$p`; then :; else \
		( $(LN_S) $$p ) ; \
	    fi; \
	done

dist_xrc_DATA = $(XRC_FILES)


# just run make in the source dir

all:
	$(MAKE) -C src

apps install clean distclean:
	$(MAKE) -C src $@
#!/bin/sh
#
# simple shell script to compile hugin on mingw using msys
# arguments will be given to the make without any processing

# fake the configure run, by changing template
cp include/hugin/config.h.tmpl include/hugin/config.h

# build hugin
make -f makefile.g95 $@

# move hugin.exe into right directory
mv hugin.exe hugin/


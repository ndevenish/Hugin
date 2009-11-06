#!/bin/bash
# -----------------------------------------------------------------------------
# Copyright (c) 2009, Yuval Levy http://www.photopla.net/
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in the
#       documentation and/or other materials provided with the distribution.
#     * Neither the name of Yuval Levy nor the
#       names of its contributors may be used to endorse or promote products
#       derived from this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY Yuval Levy ''AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL Yuval Levy BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# -----------------------------------------------------------------------------
# build.sh - runs a clean build process
# -----------------------------------------------------------------------------
# TODO:
# - pass branch and revision as argument from the command line
# - handle building from tarball, inclusive untarring xvfz automatically
# - handle depth of development branches automatically
# -----------------------------------------------------------------------------
# revision
rev="HEAD"
# source folder
# src="yuv"
src="trunk"
# src="branches/gsoc2009_layout"
# src="branches/autocrop"
# src="branches/gsoc2008_masking"
# src="branches/gsoc2008_feature_matching"
# src="release"
# package from release
# src="hugin-2009.4.0"
# package from trunk
# src="hugin-2009.5.0"
# other folder
# src="0.8"
# src="beta3"

# folder for the build
b="bdir"
# build local deb
deb="ON"
# build tarball
trb="ON"
# build with debugging symbols
dbg="ON"
# -----------------------------------------------------------------------------
# update the source tree
cd $src
if [ $rev = "HEAD" ]
then
	svn up
else
	svn up -r $rev
fi
cd ..
# TODO: second cd .. only for dev branches
# cd ..
# set up build folder
if [ -d $b ]
then
	rm -fr $b
fi
mkdir $b
cd $b
# run cmake
if [$dbg = "ON" ]
then
    opt = "-DCMAKE_BUILD_TYPE=custom -DCMAKE_CXX_FLAGS_CUSTM=-g"
else
    opt = ""
fi
cmake "../$src" -DENABLE_LAPACK=YES -DCPACK_BINARY_DEB:BOOL=$deb -DCPACK_BINARY_NSIS:BOOL=OFF \
-DCPACK_BINARY_RPM:BOOL=OFF -DCPACK_BINARY_STGZ:BOOL=OFF -DCPACK_BINARY_TBZ2:BOOL=OFF \
-DCPACK_BINARY_TGZ:BOOL=OFF -DCPACK_BINARY_TZ:BOOL=OFF $opt
# make package
if [ $deb = "ON" ]
then
	make package
	mv *.deb ../
fi
# make tarball
if [ $trb = "ON" ]
then
	make package_source
	mv *.tar.gz ../
fi


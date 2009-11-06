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
# translate.sh - automatically updates all translations from 'trunk'
#                to 'release' and commits.
# -----------------------------------------------------------------------------

SOURCE="src/translations"
SUFFIX="po"

cd trunk
svn up
cd ..
cd release
for i in "$SOURCE"/*.$SUFFIX
do
    l=`echo ${i%%.$SUFFIX} |sed 's#^.*/##'`
    echo $l
    mv "$SOURCE/$l.po" "$SOURCE/$l.in.po"
    msgmerge -o "$SOURCE/$l.po" "../trunk/$SOURCE/$l.po" "$SOURCE/$l.in.po"
    rm "$SOURCE/$l.in.po"
done
svn up
if svn diff > /dev/null ; then 
    svn ci -m "updated translations"
else
    echo "no change"
fi

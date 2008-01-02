#!/bin/sh
BASEDIR="../" # root of translatable sources
PROJECT="hugin" # project name
#BUGADDR="pablo.dangelo@web.de" # MSGID-Bugs
BUGADDR="http://sourceforge.net/tracker/?group_id=77506&atid=550441" # MSGID-Bugs
COPYRIGHT="Pablo dAngelo"
WDIR=`pwd` # working dir

echo "Preparing rc files"

cd ${BASEDIR}
find . -name '*.xrc' | sort > ${WDIR}/xrcfiles.list
xargs --arg-file=${WDIR}/xrcfiles.list wxrc -g > ${WDIR}/xrc.cpp
cd ${WDIR}
echo "Done preparing rc files"
       
       
echo "Extracting messages"
cd ${BASEDIR}
# we use simple sorting to make sure the lines do not jump around too much from system to system
find . -name '*.cpp' -o -name '*.h' -o -name '*.c' | sort > ${WDIR}/infiles.list
echo "xrc.cpp" >> ${WDIR}/infiles.list
cat $WDIR/POTFILES.in >> ${WDIR}/infiles.list

cd ${WDIR}
xgettext --from-code=UTF-8 -C -k_ --copyright-holder="$COPYRIGHT" \
         --msgid-bugs-address="${BUGADDR}" \
         --files-from=infiles.list -D ${BASEDIR} -D ${WDIR} -o ${PROJECT}.pot || { echo "error while calling xgettext. aborting."; exit 1; }
echo "Done extracting messages"
       
       
echo "Merging translations"
catalogs=`find . -name '*.po'`
for cat in $catalogs; do
  echo $cat
  msgmerge --verbose -o $cat.new $cat ${PROJECT}.pot
  mv $cat.new $cat
done
echo "Done merging translations"
       
       
echo "Cleaning up"
cd ${WDIR}
rm xrcfiles.list
rm infiles.list
rm xrc.cpp
echo "Done"

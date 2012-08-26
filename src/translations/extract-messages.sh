#!/bin/sh
BASEDIR="../" # root of translatable sources
PROJECT="hugin" # project name
#BUGADDR="pablo.dangelo@web.de" # MSGID-Bugs
BUGADDR="https://bugs.launchpad.net/hugin/" # MSGID-Bugs
COPYRIGHT="Pablo dAngelo"
WDIR=`pwd` # working dir

# safety checks
which wxrc &> /dev/null || { echo "Error: wxrc utility not found"; exit 0; }
which xgettext &> /dev/null || { echo "Error: xgettext utility not found"; exit 0; }
which msgmerge &> /dev/null || { echo "Error: msgmerge utility not found"; exit 0; }

echo "Preparing rc files"

cd ${BASEDIR}
find . -name '*.xrc' | sort | while read line; do wxrc -g $line >> ${WDIR}/xrc.cpp; done
cd ${WDIR}
echo "Done preparing rc files"
       
echo "Filtering out ignored strings"
./filter-ignored-strings.py xrc.cpp
echo "Done filtering"
       
echo "Extracting messages"
cd ${BASEDIR}
# we use simple sorting to make sure the lines do not jump around too much from system to system
find . -name '*.cpp' -o -name '*.h' -o -name '*.c' \
    | sort > ${WDIR}/infiles.list
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
rm infiles.list
rm xrc.cpp
echo "Done"

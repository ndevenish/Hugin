#! /bin/sh
# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100110.0 sg Make libGLEW and libexiv2 dynamic
#               Update to enblend-enfuse-4.0 and panotools 2.9.15
# 20100112.0 sg Made libxmi dynamic. Created lib-static directory
# 20100117.0 sg Update for glew 1.5.2
# 20100119.0 HvdW Add libiconv
# 20100118.1 sg Fixed missing "" and named SVN directory for panotools libpano13-2.9.16
# 20100121.0 sg Updated for newer packages: boost,jpeg,png,tiff,exiv2,lcms
# 20100121.1 sg Backed out new version of boost
# 20120413.0 hvdw update a lot of stuff. Add new scripts
# 20121010.0 hvdw update lots of scripts
# -------------------------------

DIR="$(cd "$(dirname "$0")" && pwd)"
cd $DIR || exit 1
source SetEnv.txt
pre="<<<<<<<<<<<<<<<<<<<< building"
pst=">>>>>>>>>>>>>>>>>>>>"

cd $REPOSITORYDIR/
cd $(ls -d */|head -n 1) # cd into first folder
# To start this script in the middle, uncomment the next 2 lines and move the "fi" line down as needed
if [ -z "check for zero length string fails" ] ; then echo;
fi
# exit 0
echo "$pre boost $pst"           && cd ../boost*        || exit 1   && sh ../../scripts/boost.sh    || exit 1
echo "$pre gettext $pst"         && cd ../gettext*      || exit 1   && sh ../../scripts/gettext.sh  || exit 1
echo "$pre libffi $pst"          && cd ../libffi*       || exit 1   && sh ../../scripts/libffi.sh   || exit 1
echo "$pre glib2 $pst"           && cd ../glib*         || exit 1   && sh ../../scripts/libglib2.sh || exit 1
echo "$pre fftw $pst"            && cd ../fftw*         || exit 1   && sh ../../scripts/fftw.sh     || exit 1
echo "$pre libglew $pst"         && cd ../glew*         || exit 1   && sh ../../scripts/libglew.sh  || exit 1
echo "$pre gsl $pst"             && cd ../gsl*          || exit 1   && sh ../../scripts/gsl.sh      || exit 1
echo "$pre libjpeg-8d $pst"      && cd ../jpeg*         || exit 1   && sh ../../scripts/libjpeg.sh  || exit 1
echo "$pre libpng $pst"          && cd ../libpng*       || exit 1   && sh ../../scripts/libpng.sh   || exit 1
echo "$pre libtiff $pst"         && cd ../tiff*         || exit 1   && sh ../../scripts/libtiff.sh  || exit 1
echo "$pre ilmbase $pst"         && cd ../ilmbase*      || exit 1   && sh ../../scripts/ilmbase.sh  || exit 1
echo "$pre openexr $pst"         && cd ../openexr*      || exit 1   && sh ../../scripts/openexr.sh	|| exit 1
echo "$pre libpano13 $pst"       && cd ../libpano13*    || exit 1   && sh ../../scripts/pano13.sh   || exit 1
echo "$pre libexiv2 $pst"        && cd ../exiv2*        || exit 1   && sh ../../scripts/libexiv2.sh || exit 1
echo "$pre liblcms-2 $pst"       && cd ../lcms2*        || exit 1   && sh ../../scripts/lcms2.sh    || exit 1
echo "$pre vigra $pst"           && cd ../vigra*        || exit 1   && sh ../../scripts/vigra.sh    || exit 1
echo "$pre wxmac $pst"           && cd ../wxWidgets*    || exit 1   && patch -p1 -N < ../../patches/wxmac-webkit.patch; sh ../../scripts/wxmac.sh || exit 1
echo "$pre enblend-enfuse $pst"  && cd ../enblend-en*   || exit 1   && sh ../../scripts/enblend.sh  || exit 1

cd $REPOSITORYDIR

echo "That's all, folks!!"

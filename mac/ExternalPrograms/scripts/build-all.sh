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

cd /PATHTOHUGIN/hugin/mac/ExternalPrograms/scripts || exit 1
cat SetEnv.txt
source SetEnv.txt
pre="<<<<<<<<<<<<<<<<<<<< building"
pst=">>>>>>>>>>>>>>>>>>>>"

# To start this script in the middle, uncomment the next 2 lines and move the "fi" line down as needed
#if [ -z "this will test will fail" ] ; then
#fi
echo "$pre boost $pst"           && cd ../boost_1_46_0 || exit 1       && sh ../scripts/boost.sh
echo "$pre expat $pst"           && cd ../expat-2.1.0 || exit 1        && sh ../scripts/libexpat.sh;
echo "$pre libiconv $pst"        && cd ../libiconv-1.14  || exit 1   && sh ../scripts/libiconv.sh;
echo "$pre gettext $pst"         && cd ../gettext-0.18.1.1  || exit 1      && sh ../scripts/gettext.sh
echo "$pre libjpeg-8d $pst"      && cd ../jpeg-8d || exit 1            && sh ../scripts/libjpeg.sh;
echo "$pre libpng $pst"          && cd ../libpng-1.4.11 || exit 1      && sh ../scripts/libpng.sh;
echo "$pre libtiff $pst"         && cd ../tiff-4.0.3 || exit 1         && sh ../scripts/libtiff.sh;
echo "$pre wxmac $pst"           && cd ../wxMac-2.9.4 || exit 1       && sh ../scripts/wxmac29.sh;
echo "$pre ilmbase $pst"         && cd ../ilmbase-1.0.2 || exit 1      && sh ../scripts/ilmbase.sh;
echo "$pre openexr $pst"         && cd ../openexr-1.7.0 || exit 1      && sh ../scripts/openexr17.sh;
echo "$pre libpano13 $pst"       && cd ../libpano13-2.9.18 || exit 1   && sh ../scripts/pano13.sh;
echo "$pre libexiv2 $pst"        && cd ../exiv2-0.23 || exit 1         && sh ../scripts/libexiv2.sh;
echo "$pre liblcms $pst"         && cd ../lcms-1.19 || exit 1          && sh ../scripts/lcms.sh;
echo "$pre liblcms-2 $pst"       && cd ../lcms-2.4 || exit 1           && sh ../scripts/lcms2.sh;
echo "$pre tclap $pst"           && cd ../tclap-1.21 || exit 1         && sh ../scripts/tclap.sh;
echo "$pre gnu science lib"      && cd ../gsl-1.15 || exit 1           && sh ../scripts/gsl.sh; 
echo "$pre libxmi $pst"          && cd ../libxmi-1.2 || exit 1         && sh ../scripts/libxmi.sh;
echo "$pre libglew $pst"         && cd ../glew-1.9.0 || exit 1         && sh ../scripts/libglew.sh;
echo "$pre libffi $pst"          && cd ../libffi-3.0.11 || exit 1      && sh ../scripts/libffi.sh;
echo "$pre glib2 $pst"           && cd ../glib-2.32.0 || exit 1        && sh ../scripts/libglib.sh;
echo "$pre lensfun $pst"         && cd ../lensfun-0.2.6 || exit 1        && sh ../scripts/lensfun.sh;
echo "$pre make-382 $pst"        && cd ../make-3.82 || exit 1          && sh ../scripts/gnumake.sh;
echo "$pre multiblend $pst"      && cd ../multiblend || exit 1         && sh ../scripts/multiblend.sh;
# Correct funky name for the enblend-enfuse-4.0 directory
if [ ! -d ../enblend-enfuse-4.0 ] && [ -d ../enblend-enfuse-4.0-753b534c819d ] ; then
	ln -s enblend-enfuse-4.0-753b534c819d ../enblend-enfuse-4.0
fi
echo "$pre enblend-enfuse $pst"  && cd ../enblend-enfuse-4.0 || exit 1 && sh ../scripts/enblend.sh;

# Following packages are optional. Uncomment if you are building them
#echo "$pre autopano-sift-C $pst" && cd ../autopano-sift-C    && sh ../scripts/autopano-sift-C.sh;
#echo "$pre panomatic $pst"       && cd ../panomatic-0.9.4    && sh ../scripts/panomatic.sh;

cd $REPOSITORYDIR

echo "That's all, folks!!"

#! /bin/sh
# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# -------------------------------

cd /PATHTOHUGIN/hugin/mac/ExternalPrograms/scripts
cat SetEnv.txt
source SetEnv.txt
pre="<<<<<<<<<<<<<<<<<<<< building"
pst=">>>>>>>>>>>>>>>>>>>>"

echo "$pre boost $pst"           && cd ../boost_1_40_0       && sh ../scripts/boost.sh
echo "$pre expat $pst"           && cd ../expat-2.0.1        && sh ../scripts/libexpat.sh;
echo "$pre gettext $pst"         && cd ../gettext-0.17       && sh ../scripts/gettext.sh
echo "$pre libjpeg-7 $pst"       && cd ../jpeg-7             && sh ../scripts/libjpeg-7.sh;
echo "$pre libpng $pst"          && cd ../libpng-1.2.38      && sh ../scripts/libpng.sh;
echo "$pre libtiff $pst"         && cd ../tiff-3.8.2         && sh ../scripts/libtiff.sh;
echo "$pre wxmac $pst"           && cd ../wxMac-2.8.10       && sh ../scripts/wxmac28.sh;
echo "$pre ilmbase $pst"         && cd ../ilmbase-1.0.1      && sh ../scripts/ilmbase.sh;
echo "$pre openexr $pst"         && cd ../openexr-1.6.1      && sh ../scripts/openexr16.sh;
echo "$pre libpano13 $pst"       && cd ../libpano13-2.9.14   && sh ../scripts/pano13.sh;
echo "$pre libexiv2 $pst"        && cd ../exiv2-0.18.2       && sh ../scripts/static/libexiv2.sh;
echo "$pre liblcms $pst"         && cd ../lcms-1.17          && sh ../scripts/lcms.sh;
echo "$pre libxmi $pst"          && cd ../libxmi-1.2         && sh ../scripts/static/libxmi.sh;
echo "$pre libglew $pst"         && cd ../glew               && sh ../scripts/static/libglew.sh;
echo "$pre gnumake-119 $pst"     && cd ../gnumake-119        && sh ../scripts/gnumake.sh;
echo "$pre enblend-enfuse $pst"  && cd ../enblend-enfuse-3.2 && sh ../scripts/enblend.sh;
echo "$pre autopano-sift-C $pst" && cd ../autopano-sift-C    && sh ../scripts/autopano-sift-C.sh;
echo "$pre panomatic $pst"       && cd ../panomatic-0.9.4    && sh ../scripts/panomatic.sh;

echo "That's all, folks!!"

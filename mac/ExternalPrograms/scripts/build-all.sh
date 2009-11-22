#! /bin/sh

cd /Users/sgaede/development/hugin/mac/ExternalPrograms/scripts
cat SetEnv.txt
source SetEnv.txt

echo building boost           && cd ../boost_1_40_0       && sh ../scripts/boost.sh
echo building expat           && cd ../expat-2.0.1        && sh ../scripts/libexpat.sh;
echo building gettext         && cd ../gettext-0.17       && sh ../scripts/gettext.sh
echo building libjpeg-7       && cd ../jpeg-7             && sh ../scripts/libjpeg-7.sh;
echo building libpng          && cd ../libpng-1.2.38      && sh ../scripts/libpng.sh;
echo building libtiff         && cd ../tiff-3.8.2         && sh ../scripts/libtiff.sh;
echo building wxmac           && cd ../wxMac-2.8.10       && sh ../scripts/wxmac28.sh;
echo building ilmbase         && cd ../ilmbase-1.0.1      && sh ../scripts/ilmbase.sh;
echo building openexr         && cd ../openexr-1.6.1      && sh ../scripts/openexr16.sh;
echo building libpano13       && cd ../libpano13-2.9.14   && sh ../scripts/pano13.sh;
echo building libexiv2        && cd ../exiv2-0.18.2       && sh ../scripts/static/libexiv2.sh;
echo building liblcms         && cd ../lcms-1.17          && sh ../scripts/lcms.sh;
echo building libxmi          && cd ../libxmi-1.2         && sh ../scripts/static/libxmi.sh;
echo building libglew         && cd ../glew               && sh ../scripts/static/libglew.sh;
echo building gnumake-119     && cd ../gnumake-119        && sh ../scripts/gnumake.sh;
echo building enblend-enfuse  && cd ../enblend-enfuse-3.2 && sh ../scripts/enblend.sh;
echo building autopano-sift-C && cd ../autopano-sift-C    && sh ../scripts/autopano-sift-C.sh;
echo building panomatic       && cd ../panomatic-0.9.4    && sh ../scripts/panomatic.sh;

echo "That's all, folks!!"

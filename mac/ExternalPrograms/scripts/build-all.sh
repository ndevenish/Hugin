#! /bin/sh
# -------------------------------
# 20091206.0 sg Script tested and used to build 2009.4.0-RC3
# 20100110.0 sg Make libGLEW and libexiv2 dynamic
#               Update to enblend-enfuse-4.0 and panotools 2.9.15
# 20100112.0 sg Made libxmi dynamic. Created lib-static directory
# 20100117.0 sg Update for glew 1.5.2
# 20100119.0 HvdW Add libiconv
# 20100118.1 sg Fixed missing "" and named SVN directory for panotools libpano13-2.9.16
# -------------------------------

cd /PATHTOHUGIN/mac/ExternalPrograms/scripts
cat SetEnv.txt
source SetEnv.txt
pre="<<<<<<<<<<<<<<<<<<<< building"
pst=">>>>>>>>>>>>>>>>>>>>"

echo "$pre boost $pst"           && cd ../boost_1_40_0       && sh ../scripts/boost.sh
echo "$pre expat $pst"           && cd ../expat-2.0.1        && sh ../scripts/libexpat.sh;
echo "$pre libiconv $pst"        && cd ../libiconv-1.13.1    && sh ../scripts/libiconv.sh;
echo "$pre gettext $pst"         && cd ../gettext-0.17       && sh ../scripts/gettext.sh
echo "$pre libjpeg-7 $pst"       && cd ../jpeg-7             && sh ../scripts/libjpeg-7.sh;
echo "$pre libpng $pst"          && cd ../libpng-1.2.38      && sh ../scripts/libpng.sh;
echo "$pre libtiff $pst"         && cd ../tiff-3.8.2         && sh ../scripts/libtiff.sh;
echo "$pre wxmac $pst"           && cd ../wxMac-2.8.10       && sh ../scripts/wxmac28.sh;
echo "$pre ilmbase $pst"         && cd ../ilmbase-1.0.1      && sh ../scripts/ilmbase.sh;
echo "$pre openexr $pst"         && cd ../openexr-1.6.1      && sh ../scripts/openexr16.sh;
echo "$pre libpano13 $pst"       && cd ../libpano13-2.9.16   && sh ../scripts/pano13.sh;
echo "$pre libexiv2 $pst"        && cd ../exiv2-0.18.2       && sh ../scripts/libexiv2.sh;
echo "$pre liblcms $pst"         && cd ../lcms-1.17          && sh ../scripts/lcms.sh;
echo "$pre libxmi $pst"          && cd ../libxmi-1.2         && sh ../scripts/libxmi.sh;
echo "$pre libglew $pst"         && cd ../glew-1.5.2         && sh ../scripts/libglew.sh;
echo "$pre gnumake-119 $pst"     && cd ../gnumake-119        && sh ../scripts/gnumake.sh;
echo "$pre enblend-enfuse $pst"  && cd ../enblend-enfuse-4.0 && sh ../scripts/enblend.sh;
echo "$pre autopano-sift-C $pst" && cd ../autopano-sift-C    && sh ../scripts/autopano-sift-C.sh;
echo "$pre panomatic $pst"       && cd ../panomatic-0.9.4    && sh ../scripts/panomatic.sh;

# Separate static libraries into their own directory. Needed to build static tools

cd $REPOSITORYDIR
if [ ! -d lib-static ] ; then
  mkdir -p lib-static
fi
if [ -n "lib/*.a" ] ; then
  cp -f lib/*.a lib-static/
  rm lib/*.a
fi
if [ ! -d lib-static/wx ] ; then
  mv lib/{pkgconfig,wx} lib-static/
  ln -s ../lib-static/pkgconfig lib/pkgconfig
  ln -s ../lib-static/wx        lib/wx
fi

echo "That's all, folks!!"

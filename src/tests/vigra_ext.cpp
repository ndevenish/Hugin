// -*- c-basic-offset: 4 -*-

/** @file impex2.cpp
 *
 *  @brief implementation of impex2 Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include "common/utils.h"

#include "vigra/stdimage.hxx"
#include "vigra/imageiterator.hxx"
#include "vigra/tinyvector.hxx"
#include "vigra/convolution.hxx"
#include "vigra/resizeimage.hxx"

#include "vigra/impexalpha.hxx"

#include "vigra_ext/FunctorAccessor.h"

using namespace boost::unit_test_framework;

using namespace std;
using namespace vigra;
using namespace vigra_ext;

void test_readfunctor_accessor()
{
    // test readfunctor accessor

    typedef FImage IMGType;
    IMGType tmpImage(10,10,9.5);

    typedef IMGType::value_type image_type;
    image_type scale=10;

    typedef vigra_ext::ReadFunctorAccessor<vigra::ScalarIntensityTransform<image_type>,
        IMGType::Accessor> ScalingAccessor;

    vigra::ScalarIntensityTransform<image_type> scaler(scale);

    ScalingAccessor scaleA(scaler,
                           tmpImage.accessor());

    // test access without offset
    image_type value = scaleA(tmpImage.upperLeft());
    BOOST_CHECK_EQUAL(value, 95);

    // test access with offset
    value = scaleA(tmpImage.upperLeft(), Diff2D(1,1));
    BOOST_CHECK_EQUAL(value, 95);
}

void test_writefunctor_accessor()
{
    // test writefunctor accessor

    typedef FImage IMGType;
    IMGType tmpImage(10,10,95);

    typedef IMGType::value_type image_type;
    image_type scale=0.1;

        // construct scaling accessor.
    typedef vigra_ext::WriteFunctorAccessor<vigra::ScalarIntensityTransform<image_type>,
        IMGType::Accessor> ScalingAccessor;

    vigra::ScalarIntensityTransform<image_type> scaler(scale);
    ScalingAccessor scaleA(scaler,
			   tmpImage.accessor());

    // test access without offset
    scaleA.set(10, tmpImage.upperLeft());
    BOOST_CHECK_EQUAL(tmpImage(0,0), 1.0f);

    // test access with offset
    scaleA.set(10, tmpImage.upperLeft(), Diff2D(1,1));
    BOOST_CHECK_EQUAL(tmpImage(1,1), 1.0f);

    BOOST_CHECK_EQUAL(tmpImage(1,0), 95);
}

void test_SplitVector2Accessor()
{
    typedef BImage IMGType;
    typedef IMGType::value_type value_type;

    IMGType img(10,10, 1);
    IMGType alpha(10,10, 2);

    // virtually merge image and mask
    typedef vigra_ext::SplitVector2Accessor<IMGType::Iterator,
	IMGType::Accessor, IMGType::Iterator, IMGType::Accessor> SplitAccessor;
    SplitAccessor splitA(img.upperLeft(), img.accessor(),
                         alpha.upperLeft(), alpha.accessor());

    // do the import
    // need to use a Coordinate iterator, because the
    // MergeAccessor requires coordinates, and not pointers to some memory
    // of the first image.

    // set some values
    BOOST_CHECK_EQUAL(img(0,0), 1);
    BOOST_CHECK_EQUAL(alpha(0,0), 2);

    // set image(0,0) to 10
    splitA.setComponent(10, vigra::CoordinateIterator(),0);
    BOOST_CHECK_EQUAL(img(0,0), 10);
    BOOST_CHECK_EQUAL(alpha(0,0), 2);

    // set alpha(0,0) to 10
    splitA.setComponent(10, vigra::CoordinateIterator(),1);
    BOOST_CHECK_EQUAL(img(0,0), 10);
    BOOST_CHECK_EQUAL(alpha(0,0), 10);

}

void test_SplitVectorNAccessor()
{
    typedef BRGBImage IMGType;
    typedef BImage    AlphaType;

    typedef IMGType::value_type value_type;

    IMGType img(10,10, value_type(1,1,1));
    AlphaType alpha(10,10, 2);

    // virtually merge image and mask
    typedef vigra_ext::SplitVectorNAccessor<IMGType::Iterator,
	IMGType::Accessor, AlphaType::Iterator, AlphaType::Accessor, 4> SplitAccessor;
    SplitAccessor splitA(img.upperLeft(), img.accessor(),
                         alpha.upperLeft(), alpha.accessor());

    // do the import
    // need to use a Coordinate iterator, because the
    // MergeAccessor requires coordinates, and not pointers to some memory
    // of the first image.

    // set some values
    BOOST_CHECK(img(0,0) == value_type(1,1,1));
    BOOST_CHECK(alpha(0,0) == 2);

    // set image(0,0) to 10
    splitA.setComponent(10, vigra::CoordinateIterator(),0);
    BOOST_CHECK(img(0,0) == value_type(10,1,1));
    BOOST_CHECK_EQUAL(alpha(0,0), 2);

    // set alpha(0,0) to 10
    splitA.setComponent(10, vigra::CoordinateIterator(),3);
    BOOST_CHECK(img(0,0) == value_type(10,1,1));
    BOOST_CHECK_EQUAL(alpha(0,0), 10);

}

void test_MergeVectorScalar2VectorAccessor()
{
    typedef IRGBImage IMGType;
    typedef IImage    AlphaType;

    typedef IMGType::value_type value_type;
    typedef value_type::value_type component_type;
    typedef TinyVector<value_type::value_type, 4> merged_type;

    IMGType img(10,10, value_type(1,1,1));
    AlphaType alpha(10,10, 2);

    // merge for read access
    typedef vigra_ext::MergeVectorScalar2VectorAccessor<IMGType::Iterator,
	IMGType::Accessor, AlphaType::Iterator, AlphaType::Accessor, 4> MergeAccessor;

    MergeAccessor mergeA(img.upperLeft(), img.accessor(),
                         alpha.upperLeft(), alpha.accessor());

    // need to use a Coordinate iterator, because the
    // MergeAccessor requires coordinates, and not pointers to some memory
    // of the first image.

    // set some values
    BOOST_CHECK(img(0,0) == value_type(1,1,1));
    BOOST_CHECK(alpha(0,0) == 2);

    // set image(0,0) to 10
    img(0,0) = value_type(10,0,0);
    merged_type res = mergeA(vigra::CoordinateIterator());
    BOOST_CHECK(res == merged_type(10,0,0,2));
    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 0),
                      10);
    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 1),
                      0);
    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 2),
                      0);
    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 3),
                      2);

    BOOST_CHECK_EQUAL(mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 0),
                      10);
    BOOST_CHECK_EQUAL(mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 1),
                      0);
    BOOST_CHECK_EQUAL(mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 2),
                      0);
    component_type t = mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 3);
    BOOST_CHECK_EQUAL(t, 2);
    t = alpha.accessor()(alpha.upperLeft(), Diff2D(0,0));
    BOOST_CHECK_EQUAL(t, 2);


    alpha(0,0) = 11;

    t = alpha.accessor()(alpha.upperLeft(), Diff2D(0,0));
    BOOST_CHECK_EQUAL(t, 11);

    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 0),
                      10);
    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 1),
                      0);
    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 2),
                      0);
    BOOST_CHECK_EQUAL(mergeA.getComponent(vigra::CoordinateIterator(), 3),
                      11);

    BOOST_CHECK_EQUAL(mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 0),
                      10);
    BOOST_CHECK_EQUAL(mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 1),
                      0);
    BOOST_CHECK_EQUAL(mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 2),
                      0);
    t = mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 3);
    BOOST_CHECK_EQUAL(t, 11);
    BOOST_CHECK_EQUAL(mergeA.getComponent(CoordinateIterator(1,1), Diff2D(-1,-1), 3),
                      11);

}


// test import of 16 bit images with alpha channel
void test_import_image()
{
    typedef unsigned short component_type;
    // load image into a 4 channel tiny vector
    typedef BasicImage<TinyVector<component_type, 4> > ImageType;
    typedef BasicImage<component_type> AlphaType;
    typedef BasicImage<TinyVector<component_type, 3> > ColorImageType;
    typedef BasicImage<unsigned char> AlphaImageType;

    // load 16 bit image directly.
    vigra::ImageImportInfo info("test_image_16bit.tif");
    BOOST_REQUIRE_MESSAGE(info.getPixelType() == string("UINT16"), " got Pixeltype: " << info.getPixelType() << ", expected UINT16");
    ImageType imgWithAlpha(info.width(), info.height());
    importImage(info, destImage(imgWithAlpha));

    // ============================================================

    // try to load with splitter, into similar channels..
    ColorImageType imgWithoutAlpha(info.width(), info.height());
    AlphaType      fullAlpha(info.width(), info.height(), 2);
    // virtually merge image and mask
    typedef vigra_ext::SplitVectorNAccessor<ColorImageType::Iterator,
	ColorImageType::Accessor, AlphaType::Iterator, AlphaType::Accessor, 4> SplitAccessor;
    SplitAccessor splitA(imgWithoutAlpha.upperLeft(), imgWithoutAlpha.accessor(),
                         fullAlpha.upperLeft(), fullAlpha.accessor());
    // import image
    importImage(info, make_pair(Diff2D(0,0), splitA));

    bool sameColor = true;
    bool sameAlpha = true;
    // compare images.
    for (int y=0; y < info.height(); y++) {
        for (int x=0; x < info.width(); x++) {
            for (int c=0; c < 3; c++) {
                component_type truth = imgWithAlpha(x,y)[c];
                component_type other = imgWithoutAlpha(x,y)[c];
                BOOST_CHECK_EQUAL(truth, other);
                if ( truth != other) {
                    sameColor = false;
                }
            }
            component_type truth = imgWithAlpha(x,y)[3];
            component_type other = fullAlpha(x,y);
            BOOST_CHECK_EQUAL(truth, other );
            if (truth != other) {
                sameAlpha=false;
            }
        }
    }
    BOOST_CHECK(sameColor);
    BOOST_CHECK(sameAlpha);

    // ============================================================

    // import image with impex alpha
    ColorImageType colorImage(info.width(), info.height());
    AlphaImageType      scaledAlpha(info.width(), info.height());
    importImageAlpha(info, destImage(colorImage), destImage(scaledAlpha));
    bool sameColorImpex = true;
    bool sameAlphaImpex = true;
    // compare images.
    for (int y=0; y < info.height(); y++) {
        for (int x=0; x < info.width(); x++) {
            for (int c=0; c < 3; c++) {
                component_type truth = imgWithAlpha(x,y)[c];
                component_type other = colorImage(x,y)[c];
                if (truth != other) {
                    sameColorImpex = false;
                }
            }
            component_type truth = imgWithAlpha(x,y)[3];
            component_type other = scaledAlpha(x,y);
            component_type scaled = truth >> 8;
            if (scaled != other ) {
                BOOST_CHECK_MESSAGE(scaled != other, "alpha difference: truth: " << truth << " scaled " << scaled << " other: " << other << " image pos: " << x << "," << y);
                sameAlpha = false;
            }
        }
    }
    BOOST_CHECK(sameColorImpex);
    BOOST_CHECK(sameAlphaImpex);

    // ============================================================

    // save raw image
    ImageExportInfo exinfo("test_image_16bit_output_direct.tif");
    exportImage(srcImageRange(imgWithAlpha), exinfo);

    // save using modified alpha channel..
    ImageExportInfo exinfo2("test_image_16bit_output_scaled.tif");
    exportImageAlpha(srcImageRange(colorImage), maskImage(scaledAlpha), exinfo2);

    // ============================================================

    // load scaled alpha image, and compare with original
    ImageImportInfo import2("test_image_16bit_output_scaled.tif");
    ImageType img2(import2.width(), import2.height());
    importImage(import2, destImage(img2));

    for (int y=0; y < info.height(); y++) {
        for (int x=0; x < info.width(); x++) {
            for (int c=0; c < 4; c++) {
                component_type truth = imgWithAlpha(x,y)[c];
                component_type other = img2(x,y)[c];
                BOOST_CHECK_MESSAGE(abs (truth > other) < 256, "error at: (" << x << "," << y << ")[" << c << "]: " << truth << " != " << other);
            }
        }
    }
}

void test_png_codec_16bit()
{
    typedef unsigned short component_type;
    // load image into a 4 channel tiny vector
    typedef BasicImage<TinyVector<component_type, 4> > ImageType;
    typedef BasicImage<component_type> AlphaType;
    typedef BasicImage<TinyVector<component_type, 3> > ColorImageType;
    typedef BasicImage<unsigned char> AlphaImageType;

    // load reference image (tiff)
    vigra::ImageImportInfo info("test_image_16bit.tif");
    BOOST_REQUIRE_MESSAGE(info.getPixelType() == string("UINT16"), " got Pixeltype: " << info.getPixelType() << ", expected UINT16");
    ImageType refImg(info.width(), info.height(), TinyVector<component_type, 4>(1,1,1,1));
    importImage(info, destImage(refImg));

    // load test image (png)
    vigra::ImageImportInfo info2("test_image_16bit.png");
    BOOST_REQUIRE_MESSAGE(info2.getPixelType() == string("UINT16"), " got Pixeltype: " << info.getPixelType() << ", expected UINT16");
    ImageType pngImg(info2.width(), info2.height(), TinyVector<component_type, 4>(2,2,2,2));
    importImage(info2, destImage(pngImg));

    for (int y=0; y < info.height(); y++) {
        for (int x=0; x < info.width(); x++) {
            for (int c=0; c < 4; c++) {
                component_type truth = refImg(x,y)[c];
                component_type other = pngImg(x,y)[c];
                BOOST_CHECK_MESSAGE(truth == other, "error at: (" << x << "," << y << ")[" << c << "]: " << truth << " != " << other);
            }
        }
    }
}

test_suite *
init_unit_test_suite( int, char** )
{
  test_suite* test= BOOST_TEST_SUITE( "vigra_ext functor tests" );
  test->add(BOOST_TEST_CASE(&test_readfunctor_accessor));
  test->add(BOOST_TEST_CASE(&test_writefunctor_accessor));
  test->add(BOOST_TEST_CASE(&test_SplitVector2Accessor));
  test->add(BOOST_TEST_CASE(&test_SplitVectorNAccessor));
  test->add(BOOST_TEST_CASE(&test_MergeVectorScalar2VectorAccessor));
  test->add(BOOST_TEST_CASE(&test_import_image));
  test->add(BOOST_TEST_CASE(&test_png_codec_16bit));

//  test->add(BOOST_TEST_CASE(&transforms_test));
  return test;
}


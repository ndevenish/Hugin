// -*- c-basic-offset: 4 -*-

/** @file interpolator.cpp
 *
 *  @brief Test the interpolator classes
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

#include <panoinc.h>
#include <functional>     // for plus

#include <vigra/basicimage.hxx>
#include <vigra/impex.hxx>
#include <vigra/error.hxx>
#include "vigra/functorexpression.hxx"

#include <PT/Interpolators.h>

//#include <vigra/impex.h>

using namespace boost::unit_test_framework;

using namespace vigra;
using namespace vigra::functor;
using namespace PT;
using namespace PT::TRANSFORM;
using namespace PTools;
using namespace std;

template <class SrcIterator, class SrcAccessor,
          class DestIterator, class DestAccessor,
          class TempImg,
          class Interpolator>
void interpolate(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> src,
                 vigra::triple<DestIterator, DestIterator, DestAccessor> dest,
                 TempImg & temp,
                 Interpolator interp)
{
    Diff2D destsz = dest.second - dest.first;
    int startx =  interp.size/2 - 1;
    int endx = destsz.x - interp.size/2 ;
    int starty = interp.size/2 - 1;
    int endy = destsz.y - interp.size/2;

    InterpolatingAccessor<SrcAccessor,
        typename SrcAccessor::value_type,
        Interpolator> interpol(src.third, interp);

    copyImage(src, destImage(temp));

    double alpha = M_PI/36;
    double tx = (destsz.x/2.0) - 0.5;
    double ty = (destsz.y/2.0) - 0.5;
    // rotate image 180 deg in 5 deg steps
    for (int step=0; step <10; step++) {
        double ca = cos(alpha);
        double sa = sin(alpha);

        DestIterator yd(dest.first + Diff2D(startx, starty));
        for(int y = starty; y < endy; ++y, ++yd.y) {
            DestIterator xd(yd);
            for(int x = startx; x < endx; ++x, ++xd.x) {
                double dx = x - tx;
                double dy = y - ty;
                dx = ca * dx - sa * dy;
                dy = sa * dx + ca * dy;
                dx = dx + tx;
                dy = dy + ty;

                if (dx > startx && dx < endx && dy > starty && dy < endy) {
                    *xd = interpol(temp.upperLeft(), dx, dy);
                } else {
                    *xd = vigra::NumericTraits<typename DestAccessor::value_type>::zero();
                }
            }
        }
        copyImage(dest, destImage(temp));
    }

    /*
    // rotate back
    typename TempImg::Accessor tac(temp.accessor());

    DestIterator yd(dest.first + Diff2D(destsz.x-1, destsz.y-1));
    for(int y = 0; y < destsz.y; ++y, --yd.y) {
        DestIterator xd(yd);
        for(int x = 0; x < destsz.x; ++x, --xd.x) {
            *xd = tac(temp.upperLeft(), Diff2D(x,y));
        }
    }
    */
}

void interpolator_test()
{
    typedef BRGBImage IMGTYPE;
    ImageImportInfo info("Monument.JPG");
//    typedef BImage IMGTYPE;
//    ImageImportInfo info("zoneplate.tif");
//    BOOST_CHECK(!info.isGrayscale());

    // create a gray scale image of appropriate size
    IMGTYPE in(info.width(), info.height());
    // import the image just read
    importImage(info, destImage(in));

    IMGTYPE result(info.width(), info.height());
    IMGTYPE diff(info.width(), info.height());
    result = 0;
    diff = 0;
    // ====== bilin
    BOOST_CHECKPOINT("bilin");
    interpolate(srcImageRange(in),
                destImageRange(result),
                diff,
                interp_bilin());

    // export the remapped image
    vigra::combineTwoImages(srcImageRange(in),
                            srcImage(result),
                            destImage(diff),
                            abs(Arg1()-Arg2()));
    exportImage(srcImageRange(result), "zoneplate_bilin.png");
    exportImage(srcImageRange(diff), "zoneplate_bilin_diff.png");

    // ====== cubic
    BOOST_CHECKPOINT("cubic");
    result = 0;
    diff = 0;
    interpolate(srcImageRange(in),
                destImageRange(result),
                diff,
                interp_cubic());
    exportImage(srcImageRange(result), "zoneplate_cubic.png");
    vigra::combineTwoImages(srcImageRange(in),
                            srcImage(result),
                            destImage(diff),
                            abs(Arg1()-Arg2()));
    exportImage(srcImageRange(diff), "zoneplate_cubic_diff.png");



    result = 0;
    diff = 0;
    BOOST_CHECKPOINT("spline36");
    // ====== spline36
    interpolate(srcImageRange(in),
                destImageRange(result),
                diff,
                interp_spline36());
    exportImage(srcImageRange(result), "zoneplate_spline36.png");
    vigra::combineTwoImages(srcImageRange(in),
                            srcImage(result),
                            destImage(diff),
                            abs(Arg1()-Arg2()));
    exportImage(srcImageRange(diff), "zoneplate_spline36_diff.png");

    result = 0;
    diff = 0;
    // ====== sinc256
    BOOST_CHECKPOINT("sinc256");
    interpolate(srcImageRange(in),
                destImageRange(result),
                diff,
                interp_sinc<8>());
    exportImage(srcImageRange(result), "zoneplate_sinc256.png");

    vigra::combineTwoImages(srcImageRange(in),
                            srcImage(result),
                            destImage(diff),
                            abs(Arg1()-Arg2()));
    exportImage(srcImageRange(diff), "zoneplate_sinc256_diff.png");

}

test_suite *
init_unit_test_suite( int, char** )
{
  test_suite* test= BOOST_TEST_SUITE( "vigra interpolator tests" );
  test->add(BOOST_TEST_CASE(&interpolator_test));
//  test->add(BOOST_TEST_CASE(&transforms_test));
  return test;
}


// -*- c-basic-offset: 4 -*-

/** @file imageproc.cpp
 *
 *  @brief implementation of imageproc Class
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

#include "hugin/ImageCache.h"
#include "hugin/ImageProcessing.h"

using namespace boost::unit_test_framework;

using namespace vigra;

void image_proc_test()
{
  wxImage ws("subject.png");
  BImage subject(ws.GetWidth(), ws.GetHeight());
  vigra::copyImage(srcImageRange(wxVigraImage(ws),
                                 RGBToGrayAccessor<RGBValue<unsigned char> >()),
                   destImage(subject));
  BOOST_CHECK(subject.size() == Diff2D(236,177));

  Diff2D point(91,46);
  Diff2D templWidth(11,11);
  Diff2D templSize(templWidth.x * 2 + 1, templWidth.y * 2 + 1);
  BImage templ(templSize);
  copyImage(subject.upperLeft() + point - templWidth, 
            subject.upperLeft() + point + templWidth + Diff2D(1,1),
            subject.accessor(),
            templ.upperLeft(),
            templ.accessor());

  FImage result(ws.GetWidth(), ws.GetHeight(), 1.0);

  CorrelationResult res;
  res = correlateImage(subject.upperLeft(),
                       subject.lowerRight(),
                       subject.accessor(),
                       result.upperLeft(),
                       result.accessor(),
                       templ.upperLeft() + templWidth,
                       templ.accessor(),
                       -templWidth,
                       templWidth);
  
  BOOST_CHECK(res.pos == point);
  BOOST_CHECK_CLOSE(res.max,1.0,1e-3);
  saveScaledImage(result, -1, 1, "result_corr.png");
  BOOST_CHECK_CLOSE(res.max, (double)result[point], 1e-6);

  /*
  // check subpixel correlation
  spres = correlateSubPixImage(subjImg.upperLeft(),
                               subjImg.upperLeft() + searchLR,
                               subjImg.accessor(),
                               tmplImg.upperLeft() + res.pos,
                               tmplImg.accessor(),
                               tmplUL, tmplLR,
                               5);
  */

  // save & load the template image..
  saveImage(templ,"template.png");

  wxImage tt("template.png");
  BOOST_ASSERT(tt.GetWidth() == templSize.x);
  BOOST_ASSERT(tt.GetHeight() == templSize.y);
  vigra::copyImage(srcImageRange(wxVigraImage(tt),
                                 RGBToGrayAccessor<RGBValue<unsigned char> >()),
                   destImage(templ));
  result.init(1);
  res.max = -1;
  res = correlateImage(subject.upperLeft(),
                       subject.lowerRight(),
                       subject.accessor(),
                       result.upperLeft(),
                       result.accessor(),
                       templ.upperLeft(),
                       templ.accessor(),
                       vigra::Diff2D(0,0),
                       templ.size() - Diff2D(1,1)
    );
  BOOST_CHECK(res.pos == point);
  BOOST_CHECK_CLOSE(res.max,1.0,1e-3);
  std::cout << "result difference: " << 1.0 - res.max << std::endl;
  saveScaledImage(result, -2, 1, "result_corr_2.png");
  BOOST_CHECK_CLOSE(res.max, (double)result[point], 1e-6);


}


test_suite*
init_unit_test_suite( int, char** )
{
  wxInitAllImageHandlers();

  test_suite* test= BOOST_TEST_SUITE( "imageprocessing tests" );
  test->add(BOOST_TEST_CASE(&image_proc_test));
  return test;
}

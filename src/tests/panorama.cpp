// -*- c-basic-offset: 4 -*-

/** @file panorama.cpp
 *
 *  @brief implementation of panorama Class
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

#include "PT/Panorama.h"

using namespace boost::unit_test_framework;

using namespace PT;

void PanoImageTest()
{
    // the main panorama
    Panorama pano;

    // test pano image
    pano.addImage("test.jpg");
    const PanoImage & img = pano.getImage(1);
    BOOST_CHECK_EQUAL(img.getFilename(), "test.jpg");

    Lens l;
    l.focalLength = l.exifFocalLength = 5.40625;
    l.focalLengthConversionFactor = l.exifFocalLengthConversionFactor = 6.88021;
    l.HFOV = l.exifHFOV = 51.6467;
    l.projectionFormat = Lens::RECTILINEAR;
    l.a = l.b = l.c = l.d = l.e = 0;
    BOOST_CHECK(l ==  pano.getLens(1));

    // test a simple panorama creation.
}

test_suite*
init_unit_test_suite( int, char** )
{
    test_suite* test= BOOST_TEST_SUITE( "UTIL tests" );
    test->add(BOOST_TEST_CASE(&PanoImageTest));

    return test;
}

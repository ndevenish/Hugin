// -*- c-basic-offset: 4 -*-

/** @file panotools.cpp
 *
 *  @brief test for my panotools mappers
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
#include "common/stl_utils.h"
#include "hugin/ImageCache.h"
#include "hugin/ImageProcessing.h"
#include "PT/Panorama.h"
#include "hugin/PanoToolsInterface.h"
#include "PT/Transforms.h"

using namespace boost::unit_test_framework;

using namespace vigra;
using namespace PT;
using namespace PT::TRANSFORM;

void stitch_test()
{
    Panorama pano;

    Lens lens;
    pano.addLens(lens);

    VariableMap vars;
    fillVariableMap(vars);
    map_get(vars,"y").setValue(10);
    map_get(vars,"p").setValue(30);
    map_get(vars,"r").setValue(10);
    map_get(vars,"v").setValue(51);
    map_get(vars,"b").setValue(-0.015);

    pano.addImage(PanoImage("photo.jpg", 1600, 1200, 0),vars);

//    wxImage output(1600,1200);
    int w = 1600;
    int h = 1200;
    wxImage output(w,h);

    unsigned char * data = output.GetData();
    std::cout << std::hex << (void *) data << std::endl;
    for (int i=0 ; i < w*h; i++) {
        data[i] = 0xAB;
    }

    UIntSet imgs;
    imgs.insert(0);
    DEBUG_DEBUG("begin stitching");
    const PanoramaOptions & opts = pano.getOptions();
    BOOST_CHECK(PTools::stitchImage(output, pano, imgs, opts));
    DEBUG_TRACE("end stitching");

    output.SaveFile("stich_result.jpg");
}

test_suite*
init_unit_test_suite( int, char** )
{
  wxInitAllImageHandlers();

  test_suite* test= BOOST_TEST_SUITE( "panotool interfaces tests" );
  test->add(BOOST_TEST_CASE(&stitch_test));
  test->add(BOOST_TEST_CASE(&transforms_test));
  return test;
}


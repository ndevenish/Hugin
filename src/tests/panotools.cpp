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
#include "PT/PanoToolsInterface.h"
#include "hugin/PanoToolsInterface.h"
#include "PT/Transforms.h"

using namespace boost::unit_test_framework;

using namespace vigra;
using namespace PT;
using namespace PT::TRANSFORM;
using namespace PTools;
using namespace std;

void remap_test()
{
    Panorama pano;

    Lens lens;
    pano.addLens(lens);

    VariableMap vars;
    fillVariableMap(vars);
    map_get(vars,"y").setValue(5);
    map_get(vars,"p").setValue(0);
    map_get(vars,"r").setValue(0);
    map_get(vars,"v").setValue(50);
    map_get(vars,"b").setValue(-0.02);

    pano.addImage(PanoImage("photo.jpg", 1600, 1200, 0),vars);

//    wxImage output(1600,1200);

    int w = 200;
    PanoramaOptions opts = pano.getOptions();
    opts.projectionFormat = PanoramaOptions::RECTILINEAR;
    opts.HFOV=50;
    opts.VFOV=50;
    opts.width=w;
    pano.setOptions(opts);

    int h = opts.getHeight();

    wxImage output(w,h);

    unsigned char * data = output.GetData();
    std::cout << std::hex << (void *) data << std::endl;
    for (int i=0 ; i < w*h*3; i++) {
        data[i] = 0xAB;
    }

    //====================================================================
    //====================================================================
    // the pt remap routines
    UIntSet imgs;
    imgs.insert(0);
    DEBUG_DEBUG("begin stitching");
    opts = pano.getOptions();
    BOOST_CHECK(PTools::stitchImage(output, pano, imgs, opts));
    DEBUG_TRACE("end stitching");
    output.SaveFile("stich_result_PT.jpg");


    //====================================================================
    //====================================================================
    data = output.GetData();
    std::cout << std::hex << (void *) data << std::endl;
    for (int i=0 ; i < w*h*3; i++) {
        //data[i] = (unsigned char) (exp((double)(i/w))/sin(i/1000.0));
        data[i] = 0;
    }
    //====================================================================
    //====================================================================
    // my remap routine
    DEBUG_DEBUG("own begin stitching");
    Transform transf;
    transf.createTransform(pano,0, pano.getOptions());


    wxImage * src= ImageCache::getInstance().getImage("photo.jpg");
    
    // outline of this image in final panorama
    vector<FDiff2D> outline;
    // bounding box
    FDiff2D ul;
    FDiff2D lr;
    Transform invT;
    invT.createInvTransform(pano, 0, opts);
    PTools::calcBorderPoints(Diff2D(1600,1200), invT, back_inserter(outline),
                             ul, lr);

    Diff2D ulInt((int)floor(ul.x), (int)floor(ul.y));
    Diff2D lrInt((int)ceil(lr.x), (int)ceil(lr.y));
    if (ulInt.x < 0) ulInt.x = 0;
    if (ulInt.y < 0) ulInt.y = 0;
    if (ulInt.x >= w) ulInt.x = w -1;
    if (ulInt.y >= h) ulInt.y = h -1;
    if (lrInt.x < 0) lrInt.x = 0;
    if (lrInt.y < 0) lrInt.y = 0;
    if (lrInt.x >= w) lrInt.x = w -1;
    if (lrInt.y >= h) lrInt.y = h -1;

    // remap image with that transform
    PTools::transformImage(srcIterRange(wxImageUpperLeft(*src),
                                        wxImageLowerRight(*src)),
                           destIterRange(wxImageUpperLeft(output)+ulInt,
                                         wxImageUpperLeft(output)+lrInt),
                           ulInt,
                           transf);
    DEBUG_TRACE("own end stitching");

    
    
    for(vector<FDiff2D>::iterator it = outline.begin(); it != outline.end();
        ++it) 
    {
        *(wxImageUpperLeft(output)+it->toDiff2D()) =  RGBValue<unsigned char>(255,0,0);
    }
    
    output.SaveFile("stich_result_own.jpg");


}

test_suite*
init_unit_test_suite( int, char** )
{
  wxInitAllImageHandlers();

  test_suite* test= BOOST_TEST_SUITE( "panotool interfaces tests" );
  test->add(BOOST_TEST_CASE(&remap_test));
//  test->add(BOOST_TEST_CASE(&transforms_test));
  return test;
}


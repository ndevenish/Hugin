// -*- c-basic-offset: 4 -*-

/** @file transforms.cpp
 *
 *  @brief tests for transformations
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

#include <config.h>
#include <boost/test/unit_test.hpp>
#include <boost/test/floating_point_comparison.hpp>

#include "panoinc.h"
#include "PT/PanoToolsInterface.h"

using namespace boost::unit_test_framework;

using namespace std;
using namespace vigra;
using namespace PT;

void transforms_test()
{
    int width = 2000;
    int height = 2000;

    VariableMap vars;
    fillVariableMap(vars);
    map_get(vars,"v").setValue(50.0);
    map_get(vars,"a").setValue(0.0);
    map_get(vars,"b").setValue(0.0);
    map_get(vars,"c").setValue(0.0);
    map_get(vars,"d").setValue(0.0);
    map_get(vars,"e").setValue(0.0);

    SpaceTransform trans;
    PTools::Transform ptTrans;
    trans.createTransform(Diff2D(width, height),
                          vars, Lens::RECTILINEAR,
                          Diff2D(360,180), PanoramaOptions::EQUIRECTANGULAR,
                          360);
    ptTrans.createTransform(Diff2D(width, height),
                            vars, Lens::RECTILINEAR,
                            Diff2D(360,180), PanoramaOptions::EQUIRECTANGULAR,
                            360);

    SpaceTransform invTrans;
    PTools::Transform ptInvTrans;
    invTrans.createInvTransform(Diff2D(width, height),
                                vars, Lens::RECTILINEAR,
                                Diff2D(360,180),
                                PanoramaOptions::EQUIRECTANGULAR,
                                360);
    ptInvTrans.createInvTransform(Diff2D(width, height),
                                vars, Lens::RECTILINEAR,
                                Diff2D(360,180),
                                PanoramaOptions::EQUIRECTANGULAR,
                                360);

    // set yaw and pitch
    // transform from equirect into image
    FDiff2D src(25.0,25.0);
    FDiff2D dest;
    FDiff2D ptdest;
    std::cout << "erect: " << dest.x << "," << dest.y << std::endl;
    trans.transform(dest,src);
    ptTrans.transform(ptdest,src);

    std::cout << "rect (img): " << dest << " pt: " << ptdest << std::endl;
    BOOST_CHECK_CLOSE((double)ptdest.x, 1000.0, 1e-6);
    BOOST_CHECK_CLOSE((double)ptdest.y, 1000.0, 1e-6);
    BOOST_CHECK_CLOSE((double)ptdest.x, (double)dest.x, 1e-6);
    BOOST_CHECK_CLOSE((double)ptdest.y, (double)dest.y, 1e-6);

    src = ptdest;
    // transfrom back to equirect
    invTrans.transform(dest,src);
    ptInvTrans.transform(ptdest,src);
    std::cout << "erect (25,25): " << dest << " pt: " << ptdest << std::endl;
    BOOST_CHECK_CLOSE((double)dest.x, 25.0, 1e-4);
    BOOST_CHECK_CLOSE((double)dest.y, 25.0, 1e-4);
    BOOST_CHECK_CLOSE((double)ptdest.x, (double)dest.x, 1e-6);
    BOOST_CHECK_CLOSE((double)ptdest.y, (double)dest.y, 1e-6);


    // transfrom back to equirect
    trans.transform(dest,FDiff2D(0,0));
    std::cout << "0, 0 -> img: " << dest.x << "," << dest.y << std::endl;

    // transforms img -> dest
    invTrans.transform(dest,FDiff2D(10,10));
    std::cout << "10, 10 inv -> erect: " << dest.x << "," << dest.y << std::endl;
    BOOST_CHECK_CLOSE((double)dest.x, 25.0, 1e-4);
    BOOST_CHECK_CLOSE((double)dest.y, 25.0, 1e-4);

/*
    PT::TRANSFORM::ERectToRect er2r(fl);

    PT::TRANSFORM::RectToERect r2er(fl);
    PT::TRANSFORM::RectToImg r2img(width, height);
    PT::TRANSFORM::ImgToRect img2r(width, height);

    double yaw=25;
    double pitch=25;
    // set yaw and pitch
    FDiff2D dest(DEG_TO_RAD(yaw),DEG_TO_RAD(pitch));
    std::cout << "erect: " << dest.x << "," << dest.y << std::endl;
    er2r(dest,dest);
    BOOST_CHECK_CLOSE((double)dest.x, 1000.0, 1e-6);
    BOOST_CHECK_CLOSE((double)dest.y, 1000.0, 1e-6);
    r2img(dest,dest);
    std::cout << "rect (img): " << dest.x << "," << dest.y << std::endl;
    BOOST_CHECK_CLOSE((double)dest.x, 2000.0, 1e-6);
    BOOST_CHECK_CLOSE((double)dest.y, 0.0, 1e-6);

    img2r(dest,dest);
    std::cout << "rect (0,0=center): " << dest.x << "," << dest.y << std::endl;
    BOOST_CHECK_CLOSE((double)dest.x, 1000.0, 1e-6);
    BOOST_CHECK_CLOSE((double)dest.y, 1000.0, 1e-6);
    r2er(dest,dest);
    std::cout << "erect: " << dest.x << "," << dest.y << std::endl;
    BOOST_CHECK_CLOSE(RAD_TO_DEG(dest.x), 25.0, 1e-4);
    BOOST_CHECK_CLOSE(RAD_TO_DEG(dest.y), 25.0, 1e-4);
*/

}

void transform_img_test()
{
    const int w=10;
    const int h=10;

    VariableMap vars;
    fillVariableMap(vars);
    map_get(vars,"v").setValue(50.0);
    map_get(vars,"a").setValue(0.0);
    map_get(vars,"b").setValue(0.0);
    map_get(vars,"c").setValue(0.0);
    map_get(vars,"d").setValue(0.0);
    map_get(vars,"e").setValue(0.0);


    SpaceTransform trans;
    trans.createTransform(Diff2D(2*w, 2*h),
                          vars, Lens::RECTILINEAR,
                          Diff2D(w,h), PanoramaOptions::EQUIRECTANGULAR,
                          50);
    cout.precision(2);
    for (int x=0; x<w; x++) {
        for (int y=0; y<h; y++) {
            FDiff2D res;
            double sx, sy;
            trans.transform(res, FDiff2D(x,y));
            cout << sx << "," << sy << " ";
        }
        cout << endl;
    }
    cout << endl;
    cout << endl;

    for (int x=0; x<w; x++) {
        for (int y=0; y<h; y++) {
            double sx, sy;
            trans.transformImgCoord(sx, sy, x,y);
            cout << sx << "," << sy << " ";
        }
        cout << endl;
    }
}

test_suite*
init_unit_test_suite( int, char** )
{
  test_suite* test= BOOST_TEST_SUITE( "transformation routine tests" );
  test->add(BOOST_TEST_CASE(&transforms_test));
  test->add(BOOST_TEST_CASE(&transform_img_test));
  return test;
}


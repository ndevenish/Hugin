// -*- c-basic-offset: 4 -*-

/** @file panorama.cpp
 *
 *  @brief test cases for the panorama class
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

#include "PT/PanoImage.h"
#include "PT/PanoCommand.h"
#include "PT/Panorama.h"

#include <algorithm>
#include "common/stl_utils.h"

//#include "common/CommandHistory.h"

using namespace boost::unit_test_framework;

using namespace PT;
using namespace std;

// a simple PanoramaObserver, to check if all change messages
// come through correctly

class TestObserver : public PT::PanoramaObserver
{
public:
    TestObserver()
        : imgUpdates(0), panoUpdates(0) { }

    virtual void panoramaImagesChanged (Panorama &pano,
                                        const UIntSet &changed)
        {
            imgUpdates++;
            UIntSet res;
            std::set_union(changedImages.begin(), changedImages.end(),
                           changed.begin(), changed.end(),
                           inserter(res,res.begin()));
            changedImages = res;
        }
    virtual void panoramaChanged (Panorama &pano)
        {
            panoUpdates++;
        }

    void reset()
        {
            changedImages.clear();
            imgUpdates = 0;
            panoUpdates = 0;
        }

    UIntSet changedImages;
    int imgUpdates;
    int panoUpdates;
};


/// creates a simple panorama with pano commands, and checks the
/// resulting Panorama model
void SimplePanoTest()
{
    // create our pano.
    Panorama pano;
    TestObserver obs;
    pano.addObserver(&obs);

    // add a Lens for the image.
    Lens lens;
    AddLensCmd lcmd(pano, lens);
    lcmd.execute();

    BOOST_CHECK_EQUAL(obs.imgUpdates, 0);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK(obs.changedImages.size() == 0);
    obs.reset();


    // create Panorama Images
    std::vector<PanoImage> imgs;
    imgs.push_back(PanoImage("image_1.tiff", 640, 480, 0));
    imgs.push_back(PanoImage("image .tiff", 641, 481, 0));
    imgs.push_back(PanoImage("image3.tiff", 642, 482, 0));
    imgs.push_back(PanoImage("image4.tiff", 643, 483, 0));
    imgs.push_back(PanoImage("image5.tiff", 644, 484, 0));
    imgs.push_back(PanoImage("image6.tiff", 645, 485, 0));
    imgs.push_back(PanoImage("image7.tiff", 646, 486, 0));

    AddImagesCmd aimgcmd(pano,imgs);
    aimgcmd.execute();

    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK(obs.changedImages.size() == 7);
    obs.reset();

    // test variable change routines.
    VariableMapVector origvar = pano.getVariables();

    // SetVariableCommand, change the pitch variable
    Variable pv("p",111.0);
    UIntSet cimgs;
    cimgs.insert(1);

    SetVariableCmd pcmd2(pano, cimgs, pv);
    pcmd2.execute();
    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 1);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"p").getValue(), 111.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"p").getValue(), 0.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"p").getValue(), 0.0);
    obs.reset();

    /// RemoveImageCmd
    RemoveImageCmd rimgc(pano,1);
    rimgc.execute();
    BOOST_CHECK_EQUAL(pano.getNrOfImages(), (size_t) 6);
    BOOST_CHECK_EQUAL(pano.getNrOfLenses(), (size_t) 1);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"p").getValue(), 0);
    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 5);
    obs.reset();

    // SetVariableCommand, change a unlinked lens variable, in two images
    Variable lav("a",-0.2);
    cimgs.clear();
    cimgs.insert(1);
    cimgs.insert(3);

    SetVariableCmd pcmd3(pano, cimgs, lav);
    pcmd3.execute();
    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 2);
    BOOST_CHECK(! map_get(pano.getLens(0).variables, "a").isLinked());
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"a").getValue(), 0.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"a").getValue(), -0.2);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"a").getValue(), 0.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(3),"a").getValue(), -0.2);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(4),"a").getValue(), 0.0);
    obs.reset();

    /// change a lens variable in the Lens itself
    LensVariable lenscv("c",-0.4);
    lenscv.setLinked();
    LensVarMap lensvarm;

    lensvarm.insert(make_pair("c", lenscv));
    lensvarm.insert(make_pair("a", LensVariable("a",0.1)));

    SetLensVariableCmd pcmd4(pano, 0, lensvarm);
    pcmd4.execute();
    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 6);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"c").getValue(), -0.4);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"c").getValue(), -0.4);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"c").getValue(), -0.4);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(3),"c").getValue(), -0.4);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(4),"c").getValue(), -0.4);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(5),"c").getValue(), -0.4);

    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"a").getValue(), 0.1);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"a").getValue(), 0.1);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"a").getValue(), 0.1);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(3),"a").getValue(), 0.1);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(4),"a").getValue(), 0.1);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(5),"a").getValue(), 0.1);

    BOOST_CHECK_EQUAL(map_get(pano.getLens(0).variables,"a").getValue(), 0.1);
    BOOST_CHECK(!map_get(pano.getLens(0).variables,"a").isLinked());
    BOOST_CHECK_EQUAL(map_get(pano.getLens(0).variables,"c").getValue(), -0.4);
    BOOST_CHECK(map_get(pano.getLens(0).variables,"c").isLinked());
    obs.reset();


    // change all variables of the image variable (not really, but
    VariableMap var2 = origvar[1];
    map_get(var2,"p").setValue(31.0);
    BOOST_CHECK_EQUAL(map_get(var2,"p").getValue(), 31.0);
    UpdateImageVariablesCmd pcmd1(pano, 1, var2);
    pcmd1.execute();

    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    // 1 is also valid!
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 6);
    obs.reset();

    VariableMapVector updVar = pano.getVariables();
//    BOOST_CHECK(origvar != updVar);
//    BOOST_CHECK_EQUAL(var2, updVar[1]);
//    BOOST_CHECK_EQUAL(map_get(var2,"p"), map_get(pano.getImageVariables(1),"p"));
    BOOST_CHECK_EQUAL(map_get(updVar[1],"p").getValue(), 31.0);

    /// Add another lens
    Lens lens2;
    lens2.projectionFormat = Lens::FULL_FRAME_FISHEYE;
    map_get(lens2.variables,"e").setLinked();
    map_get(lens2.variables,"v").setValue(48);
    map_get(lens2.variables,"e").setValue(2.0);

    AddLensCmd laddCmd(pano,lens2);
    laddCmd.execute();
    BOOST_CHECK_EQUAL(obs.imgUpdates, 0);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    obs.reset();

    /// add lens to images 0 2 3
    cimgs.clear();
    cimgs.insert(0);
    cimgs.insert(2);
    cimgs.insert(3);

    SetImageLensCmd slensCmd(pano, cimgs, 1);
    slensCmd.execute();

    BOOST_CHECK_EQUAL(pano.getImage(0).getLensNr(), (unsigned int) 1);
    BOOST_CHECK_EQUAL(pano.getImage(1).getLensNr(), (unsigned int) 0);
    BOOST_CHECK_EQUAL(pano.getImage(2).getLensNr(), (unsigned int) 1);
    BOOST_CHECK_EQUAL(pano.getImage(3).getLensNr(), (unsigned int) 1);
    BOOST_CHECK_EQUAL(pano.getImage(4).getLensNr(), (unsigned int) 0);
    BOOST_CHECK_EQUAL(pano.getImage(5).getLensNr(), (unsigned int) 0);

    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"e").getValue(), 2.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"e").getValue(), 0.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"e").getValue(), 2.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(3),"e").getValue(), 2.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(4),"e").getValue(), 0.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(5),"e").getValue(), 0.0);

    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"v").getValue(), 48);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"v").getValue(), 50);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"v").getValue(), 48);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(3),"v").getValue(), 48);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(4),"v").getValue(), 50);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(5),"v").getValue(), 50);


    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 3);
    obs.reset();

    /// change a image lens variable in first Lens
    Variable lbVar("b",-4.0);
    cimgs.clear();
    cimgs.insert(1);
    cimgs.insert(5);
    SetVariableCmd pcmd5(pano, cimgs, lbVar);
    pcmd5.execute();
    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 2);
    obs.reset();
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"b").getValue(), -0.01);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"b").getValue(), -4.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"b").getValue(), -0.01);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(3),"b").getValue(), -0.01);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(4),"b").getValue(), -0.01);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(5),"b").getValue(), -4.0);

    BOOST_CHECK_EQUAL(map_get(pano.getLens(0).variables,"b").getValue(), -0.01);
    BOOST_CHECK_EQUAL(map_get(pano.getLens(1).variables,"b").getValue(), -0.01);


    /// change the image part of a linked lens variable
    Variable lvVar("e",-0.5);
    cimgs.clear();
    cimgs.insert(0);
    SetVariableCmd pcmd6(pano, cimgs, lvVar);
    pcmd6.execute();
    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 3);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(0),"e").getValue(), -0.5);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(1),"e").getValue(), 0.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(2),"e").getValue(), -0.5);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(3),"e").getValue(), -0.5);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(4),"e").getValue(), 0.0);
    BOOST_CHECK_EQUAL(map_get(pano.getImageVariables(5),"e").getValue(), 0.0);
    obs.reset();


    // add a third lens
    Lens lens3;
    lens3.projectionFormat = Lens::PANORAMIC;
    map_get(lens2.variables,"b").setLinked();
    map_get(lens2.variables,"v").setValue(360);
    map_get(lens2.variables,"b").setValue(2.0);

    AddLensCmd ladd2Cmd(pano,lens3);
    ladd2Cmd.execute();
    BOOST_CHECK_EQUAL(pano.getNrOfLenses(), (size_t) 3);
    BOOST_CHECK_EQUAL(obs.imgUpdates, 0);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    obs.reset();

    /// add lens 2 to images 0 3 4
    cimgs.clear();
    cimgs.insert(0);
    cimgs.insert(3);
    cimgs.insert(4);

    SetImageLensCmd slens2Cmd(pano, cimgs, 2);
    slens2Cmd.execute();

    BOOST_CHECK_EQUAL(pano.getImage(0).getLensNr(), (unsigned int) 2);
    BOOST_CHECK_EQUAL(pano.getImage(1).getLensNr(), (unsigned int) 0);
    BOOST_CHECK_EQUAL(pano.getImage(2).getLensNr(), (unsigned int) 1);
    BOOST_CHECK_EQUAL(pano.getImage(3).getLensNr(), (unsigned int) 2);
    BOOST_CHECK_EQUAL(pano.getImage(4).getLensNr(), (unsigned int) 2);
    BOOST_CHECK_EQUAL(pano.getImage(5).getLensNr(), (unsigned int) 0);

    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 3);
    obs.reset();


    /// delete image 2, to see if Lens 1 is deleted
    RemoveImageCmd rm2Cmd(pano, 2);
    rm2Cmd.execute();

    BOOST_CHECK_EQUAL(pano.getNrOfLenses(), (size_t) 2);
    BOOST_CHECK_EQUAL(pano.getImage(0).getLensNr(), (unsigned int) 1);
    BOOST_CHECK_EQUAL(pano.getImage(1).getLensNr(), (unsigned int) 0);
    BOOST_CHECK_EQUAL(pano.getImage(2).getLensNr(), (unsigned int) 1);
    BOOST_CHECK_EQUAL(pano.getImage(3).getLensNr(), (unsigned int) 1);
    BOOST_CHECK_EQUAL(pano.getImage(4).getLensNr(), (unsigned int) 0);

    // check if the correct lenses have been removed
    BOOST_CHECK_EQUAL(pano.getLens(0).projectionFormat, Lens::RECTILINEAR);
    BOOST_CHECK_EQUAL(pano.getLens(1).projectionFormat, Lens::PANORAMIC);

    BOOST_CHECK_EQUAL(obs.imgUpdates, 1);
    BOOST_CHECK_EQUAL(obs.panoUpdates, 1);
    BOOST_CHECK_EQUAL(obs.changedImages.size(), (size_t) 4);
    obs.reset();


}

test_suite*
init_unit_test_suite( int, char** )
{
    test_suite* test= BOOST_TEST_SUITE( "UTIL tests" );
    test->add(BOOST_TEST_CASE(&SimplePanoTest));

    return test;
}

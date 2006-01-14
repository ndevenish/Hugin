// -*- c-basic-offset: 4 -*-

/** @file PanorCommand.cpp
 *
 *  @brief implementation of some PanoCommands
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

#include "panoinc_WX.h"
#include "panoinc.h"
#include "common/wxPlatform.h"

#include <hugin/ImageCache.h>
#include <hugin/wxPanoCommand.h>

#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>



using namespace std;
using namespace vigra;
using namespace utils;

namespace PT {

void wxAddCtrlPointGridCmd::execute()
{
    PanoCommand::execute();


    const PanoImage & i1 = pano.getImage(img1);
    const PanoImage & i2 = pano.getImage(img1);

    // run both images through the harris corner detector
    const vigra::BImage & leftImg = ImageCache::getInstance().getPyramidImage(
        i1.getFilename(),1);

    BImage leftCorners(leftImg.size());
    FImage leftCornerResponse(leftImg.size());

    // empty corner image
    leftCorners.init(0);

    DEBUG_DEBUG("running corner detector");

    // find corner response at scale scale
    vigra::cornerResponseFunction(srcImageRange(leftImg),
        destImage(leftCornerResponse),
        scale);

    //    saveScaledImage(leftCornerResponse,"corner_response.png");
    DEBUG_DEBUG("finding local maxima");
    // find local maxima of corner response, mark with 1
    vigra::localMaxima(srcImageRange(leftCornerResponse), destImage(leftCorners), 255);

//    exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("c:/corner_response_maxima.png"));

    DEBUG_DEBUG("thresholding corner response");
    // threshold corner response to keep only strong corners (above 400.0)
    transformImage(srcImageRange(leftCornerResponse), destImage(leftCornerResponse),
        vigra::Threshold<double, double>(
        cornerThreshold, DBL_MAX, 0.0, 1.0));

    vigra::combineTwoImages(srcImageRange(leftCorners), srcImage(leftCornerResponse),
        destImage(leftCorners), std::multiplies<float>());

//    exportImage(srcImageRange(leftCorners), vigra::ImageExportInfo("c:/corner_response_threshold.png"));

    // create transform from img1 -> sphere
    PTools::Transform img1ToSphere;
    PTools::Transform sphereToImg2;

    PanoramaOptions opts;
    opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
    opts.setHFOV(360);
    opts.setWidth(360);
    opts.setVFOV(180);

    img1ToSphere.createInvTransform(pano, img1, opts);
    sphereToImg2.createTransform(pano, img2, opts);


    int border = 5;
    double sphx, sphy;
    double img2x, img2y;
    // sample grid on img1 and try to add ctrl points
    for (unsigned int x=0; x < i1.getWidth(); x += 2 ) {
        for (unsigned int y=0; y < i1.getHeight(); y +=2 ) {
            if (leftCorners(x/2,y/2) > 0) {
                img1ToSphere.transformImgCoord(sphx, sphy, x, y);
                sphereToImg2.transformImgCoord(img2x, img2y, sphx, sphy);
                // check if it is inside..
                if (   img2x > border && img2x < i2.getWidth() - border
                    && img2y > border && img2y < i2.getHeight() - border )
                {
                    // add control point
                    ControlPoint p(img1,x, y, img2, img2x, img2y);
                    pano.addCtrlPoint(p);
                }
            }
        }
    }


    pano.changeFinished();
}

} // namespace


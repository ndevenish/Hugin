// -*- c-basic-offset: 4 -*-
/** @file CalculateOptimalScale.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Panorama.h 1947 2007-04-15 20:46:00Z dangelo $
 *
 * !! from Panorama.h 1947 
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "CalculateOptimalScale.h"

#include <panotools/PanoToolsInterface.h>


namespace HuginBase {

///
double CalculateOptimalScale::calcOptimalScale(PanoramaData& panorama)
{
    if (panorama.getNrOfImages() == 0)
        return 1;

    PanoramaOptions opt = panorama.getOptions();
    double scale = 0;

    for (unsigned i = 0; i < panorama.getNrOfImages(); i++) {
        double s = calcOptimalPanoScale(panorama.getSrcImage(i), opt);
        if (scale < s) {
            scale = s;
        }
    }

    return scale;
}


/** function to calculate the scaling factor so that the distances
    * in the input image and panorama image are similar at the panorama center
    */
double CalculateOptimalScale::calcOptimalPanoScale(const SrcPanoImage & src,
                                                    const PanoramaOptions & dest)
{
    // calculate the input pixel per output pixel ratio at the panorama center.

    PTools::Transform transf;
    SrcPanoImage timg = src;
    timg.setRoll(0);
    timg.setPitch(0);
    timg.setYaw(0);
    timg.setX(0);
    timg.setY(0);
    timg.setZ(0);
    transf.createTransform(timg, dest);
    hugin_utils::FDiff2D imgp1;
    hugin_utils::FDiff2D imgp2;

    transf.transform(imgp1, hugin_utils::FDiff2D(0, 0));
    transf.transform(imgp2, hugin_utils::FDiff2D(1, 1));
    double dist = hugin_utils::norm(imgp2-imgp1);

    return dist / sqrt(2.0);

    /*
        // calculate average pixel density of each image
        // and use the highest one to calculate the width
        double density=0;
        double w = imgSize.x;
        switch (imgProj) {
            case Lens::RECTILINEAR:
                density = 1/RAD_TO_DEG(atan(2*tan(DEG_TO_RAD(v)/2)/w));
                break;
            case Lens::CIRCULAR_FISHEYE:
            case Lens::FULL_FRAME_FISHEYE:
                // if we assume the linear fisheye model: r = f * theta
                // then we get the same pixel density as for cylindrical and equirect
            case Lens::EQUIRECTANGULAR:
            case Lens::PANORAMIC:
                density = w / v;
                break;
        }
        // TODO: use density properly based on the output projection.
        double width = roundi(density * opt.getHFOV());
        */
}


} //namespace

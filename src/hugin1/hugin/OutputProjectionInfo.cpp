// -*- c-basic-offset: 4 -*-

/** @file OutputProjectionInfo.cpp
 *
 *  @author James Legg
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */ 

#include "OutputProjectionInfo.h"

OutputProjectionInfo::OutputProjectionInfo(HuginBase::PanoramaOptions *output)
{
    proj = output;
    // create the transformation from lat/long to the output image.
    HuginBase::SrcPanoImage fake_image;
    fake_image.setProjection(HuginBase::SrcPanoImage::EQUIRECTANGULAR);
    fake_image.setSize(vigra::Size2D(360, 180));
    fake_image.setHFOV(360.0);
    transform.createInvTransform(fake_image, *output);
    // sometimes we want to go back again
    reverse_transform.createTransform(fake_image, *output);
    // now grab the frequently used values
    double x, y;
    transform.transformImgCoord(x, y, 0.0, 90.0);
    x_add_360 = x * -2.0 + output->getSize()->x;
    radius = -x + output->getSize()->x / 2.0;
    transform.transformImgCoord(x, y, 180.0, 0.0);
    y_add_360 = -y * 2.0 + output->getSize()->y;
    north_pole_x = x;
    north_pole_y = y;
    transform.transformImgCoord(x, y, 180.0, 180.0);
    south_pole_x = x;
    south_pole_y = y;
    transform.transformImgCoord(x, y, 180.0, 90.0);
    middle_x = x;
    middle_y = y;
    transform.transformImgCoord(x, y, 120.0, 90.0);
    lower_x = x;
    transform.transformImgCoord(x, y, 240.0, 90.0);
    upper_x = x;
    transform.transformImgCoord(x, y, 180.0, 60.0);
    lower_y = y;
    transform.transformImgCoord(x, y, 180.0, 120.0);
    upper_y = y;
    switch (output->getProjection())
    {
        case HuginBase::PanoramaOptions::TRANSVERSE_MERCATOR:
        case HuginBase::PanoramaOptions::STEREOGRAPHIC:
        case HuginBase::PanoramaOptions::LAMBERT_AZIMUTHAL:
        case HuginBase::PanoramaOptions::HAMMER_AITOFF:
        case HuginBase::PanoramaOptions::FULL_FRAME_FISHEYE:
        case HuginBase::PanoramaOptions::ARCHITECTURAL:
        case HuginBase::PanoramaOptions::ORTHOGRAPHIC:
        case HuginBase::PanoramaOptions::EQUISOLID:
        case HuginBase::PanoramaOptions::THOBY_PROJECTION:
            // The poles are on the sides.
            // ...or at least they can be detected from the sides (disk-likes)
            // I've offset the noth pole slightly as the detection rate was
            // not 100%
            transform.transformImgCoord(x, y, 0.5, 90.0);
            north_pole_x = x;
            north_pole_y = y;
            transform.transformImgCoord(x, y, 359.5, 90.0);
            south_pole_x = x;
            south_pole_y = y;
            break;
        default:
            break;
    }
}

// Some of the properties defined above change with height in some projections.
// We have to convert an output point to elevation, and then find the boundaries
// for +/- 180 degree seam detection at that elevation.
const double OutputProjectionInfo::GetUpperX(const double y) const
{
    double temp, pitch, result;
    reverse_transform.transformImgCoord(temp, pitch, 180, y);
    transform.transformImgCoord(result, temp, 240.0, pitch);
    return result;
}

const double OutputProjectionInfo::GetLowerX(const double y) const
{
    double temp, pitch, result;
    reverse_transform.transformImgCoord(temp, pitch, 180, y);
    transform.transformImgCoord(result, temp, 120.0, pitch);
    return result;
}

// correction also changes, this is needed to move the detected vertices.
const double OutputProjectionInfo::GetXAdd360(const double y) const
{
    double temp, pitch, result;
    reverse_transform.transformImgCoord(temp, pitch, 180, y);
    transform.transformImgCoord(result, temp, 0.0, pitch);
    return result * -2.0 + proj->getSize()->x;
}


// Use the transformations to get arbitary points.

bool OutputProjectionInfo::AngularToImage(double &image_x, double &image_y,
                                          double yaw, double pitch)
{
    return transform.transformImgCoord(image_x, image_y,
                                       yaw + 180.0, pitch + 90.0);
}

bool OutputProjectionInfo::ImageToAngular(double &yaw, double &pitch,
                                          double image_x, double image_y)
{
    bool r = reverse_transform.transformImgCoord(yaw, pitch, image_x, image_y);
    yaw -= 180.0;
    pitch -= 90.0;
    return r;
}


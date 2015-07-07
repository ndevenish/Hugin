// -*- c-basic-offset: 4 -*-

/** @file OutputProjectionInfo.h
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

/* An OutputProjectionInfo gives information about where specific points of
 * latitude and longitude map to on the output image.
 * The object is valid for only the projection passed to its constructor.
 * The idea is that ChoosyRemapper and VertexCoordRemapper can get information
 * about the output projection not specific to their images.
 * ViewState recreates the object when the projections change.
 */

#ifndef _OUTPUTPROJECTIONINFO_H
#define _OUTPUTPROJECTIONINFO_H

#include "panodata/PanoramaOptions.h"
#include <panotools/PanoToolsInterface.h>

class OutputProjectionInfo
{
public:
    explicit OutputProjectionInfo(HuginBase::PanoramaOptions *output);
    // most are for correcting faces crossing the +/-180 degree boundary
    const double GetMiddleX() const
        {return middle_x;}
    const double GetMiddleY() const
        {return middle_y;}
    const double GetLowerX() const
        {return lower_x;}
    const double GetUpperX() const
        {return upper_x;}
    const double GetLowerY() const
        {return lower_y;}
    const double GetUpperY() const
        {return upper_y;}
    // for sinusoidal (or similar) projections, the bounds move with height.
    const double GetUpperX(const double y) const;
    const double GetLowerX(const double y) const;
    // the seam is a circle in some projections
    const double GetRadius() const
        {return radius;}
    // we'll need these to flip things across the seam in cylinder-like cases.
    const double GetXAdd360() const
        {return x_add_360;}
    const double GetYAdd360() const
        {return y_add_360;}
    // they can move with height too
    const double GetXAdd360(const double y) const;
    // locations of poles, used by the ChoosyRemapper.
    const double GetNorthPoleX() const
        {return north_pole_x;}
    const double GetNorthPoleY() const
        {return north_pole_y;}
    const double GetSouthPoleX() const
        {return south_pole_x;}
    const double GetSouthPoleY() const
        {return south_pole_y;}
    // use the transformation for anything else
    bool AngularToImage(double &image_x, double &image_y, double yaw, double pitch);
    bool ImageToAngular(double &yaw, double &pitch, double image_x, double image_y);
private:
    double lower_x, middle_x, upper_x,
           lower_y, middle_y, upper_y,
           radius, x_add_360, y_add_360,
           north_pole_x, north_pole_y,
           south_pole_x, south_pole_y;
    HuginBase::PanoramaOptions *proj;
    HuginBase::PTools::Transform transform;
    HuginBase::PTools::Transform reverse_transform;
};

#endif


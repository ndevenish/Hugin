// -*- c-basic-offset: 4 -*-
/** @file GreatCircles.cpp
 *
 *  @author James Legg
 *
 *  @brief Implement GreatCircles class.
 *
 *  Copyright 2009 James Legg.
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
 */

#include "GreatCircles.h"

// for some reason #include <GL/gl.h> isn't portable enough.
#include <wx/platform.h>
#ifdef __WXMAC__
#include <OpenGL/gl.h>
#else
#ifdef __WXMSW__
#include <vigra/windows.h>
#endif
#include <GL/gl.h>
#endif

// we use the trigonometric functions.
#include <cmath>

// The number of segments to use in subdivision of the line.
// Higher numbers increase the accuracy of the line, but it is slower.
// Must be at least two. More is much better.
const unsigned int segments = 64;

void GreatCircles::setViewState(ViewState * viewStateIn)
{
    m_viewState = viewStateIn;
}

void GreatCircles::drawLineFromSpherical(double startLat, double startLong,
                                         double endLat, double endLong)
{
    DEBUG_ASSERT(m_viewState);
    // get the output projection
    const HuginBase::PanoramaOptions & options = *(m_viewState->GetOptions());
    // make an image to transform spherical coordinates into the output projection
    HuginBase::SrcPanoImage equirectangularImage;
    equirectangularImage.setProjection(HuginBase::SrcPanoImage::EQUIRECTANGULAR);
    equirectangularImage.setHFOV(360.0);
    equirectangularImage.setSize(vigra::Size2D(360.0, 180.0));
    
    // make a transformation from spherical coordinates to the output projection
    HuginBase::PTools::Transform transform;
    transform.createInvTransform(equirectangularImage, options);
    
    /**Handle case where the points are opposite sides of the sphere
     * (i.e. The angle startLat is -endLat and startLong is -endLong.)
     * There are infinetly many great circles in this case, we pick one going
     * through (180, 90), by splitting the problem in two.
     */
    if (startLat == 360.0 - endLat && startLong == 180.0 - endLong)
    {
        // we should probably check to see if we already go through (180, 90).
        if (startLat == 180.0 && startLong == 90.0)
        {
            // foiled again: pick one going through (180, 0) instead.
            drawLineFromSpherical(startLat, startLong, 180.0, 0.0);
            drawLineFromSpherical(180.0, 0.0, endLat, endLong);
            return;
        }
        drawLineFromSpherical(startLat, startLong, 180.0, 90.0);
        drawLineFromSpherical(180.0, 90.0, endLat, endLong);
        return;
    }
    
    // convert start and end positions so that they don't go across the +/-180
    // degree seam
    if (startLat < 90.0 && endLat > 270.0)
    {
        endLat -= 360.0;
    }
    // convert to radians
    startLat *= (M_PI / 180.0);
    startLong *= (M_PI / 180.0);
    endLat *= (M_PI / 180.0);
    endLong *= (M_PI / 180.0);
    
    // find sines and cosines, they are used multiple times.
    double sineStartLat = std::sin(startLat);
    double sineStartLong = std::sin(startLong);
    double sineEndLat = std::sin(endLat);
    double sineEndLong = std::sin(endLong);
    double cosineStartLat = std::cos(startLat);
    double cosineStartLong = std::cos(startLong);
    double cosineEndLat = std::cos(endLat);
    double cosineEndLong = std::cos(endLong);
    
    /* to get points on the great circle, we linearly interpolate between the
     * two 3D coordinates for the given spherical coordinates, then normalise
     * the vector to get back on the sphere. This works everywhere except exact
     * opposite points, where we'll get the original points repeated several
     * times (and if we are even more unlucky we will hit the origin where the
     * normal isn't defined.)
     */
    // convert locations to 3d coordinates.
    double p1[3] = {cosineStartLat * sineStartLong, sineStartLat * sineStartLong, cosineStartLong};
    double p2[3] = {cosineEndLat * sineEndLong, sineEndLat * sineEndLong, cosineEndLong};
    
    ///@todo don't check the +/- 180 degree boundary when projection does not break there.
    bool hasSeam = true;
    // draw a line strip and transform the coordinates as we go.
    double b1 = 0.0;
    double b2 = 1.0;
    const double bDifference = 1.0 / double(segments);
    glBegin(GL_LINE_STRIP);
    // for discontinuity detection.
    int lastSegment = 1;
    for (unsigned int segment_index = 0; segment_index < segments;
         segment_index++, b1 += bDifference, b2 -= bDifference)
    {
        // linearly interpolate positions
        double v[3] = {p1[0] * b1 + p2[0] * b2, p1[1] * b1 + p2[1] * b2, p1[2] * b1 + p2[2] * b2};
        // normalise
        double length = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
        /*double longitude = atan2(numerator,
                                 cosineStartLong * (c1 * std::sin(latitude) -
                                                    c2 * std::cos(latitude)));*/
        double longitude = std::acos(v[2]);
        // acos returns values between 0 and M_PI. The other
        // latitudes are on the back of the sphere, so check y coordinate (v1)
        double latitude = std::acos(v[0] / std::sin(longitude));
        if (v[1] < 0.0)
        {
            // on the back.
            latitude = -latitude + 2 * M_PI;
        }
        
        double vx, vy;
        bool infront =  transform.transformImgCoord(vx,
                                                    vy,
                                                    latitude  * 180.0 / M_PI,
                                                    longitude  * 180.0 / M_PI);
        // don't draw across +/- 180 degree seems.
        if (hasSeam)
        {
            // we divide the width of the panorama into 3 segments. If we jump
            // across the middle section, we split the line into two.
            int newSegment = vx / (options.getWidth() / 3);
            if ((newSegment < 1 && lastSegment > 1) ||
                (newSegment > 1 && lastSegment < 1))
            {
                glEnd();
                glBegin(GL_LINE_STRIP);
            }
            lastSegment = newSegment;
        }
        if (infront)
        {
            glVertex2d(vx, vy);
        } else {
            // don't draw this as it is behind us, and the position is invalid.
            glEnd();
            // hopefully we'll be back with a sensible value next iteration.
            glBegin(GL_LINE_STRIP);
        }
    }
    glEnd();
}

// -*- c-basic-offset: 4 -*-

/** @file PanoramaMemento.cpp
 *
 *  @brief implementation of PanoramaMemento Class
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

#include "Lens.h"

#include <vigra/impex.hxx>
#include <hugin_utils/utils.h>
#include <hugin_utils/stl_utils.h>


namespace HuginBase {

using namespace hugin_utils;


Lens::Lens()
    : m_hasExif(false), m_projectionFormat(RECTILINEAR),
      m_imageSize(0,0), m_sensorSize(36.0,24.0)
{
    fillLensVarMap(variables);
}


const char* Lens::variableNames[] = { "v", "a", "b", "c", "d", "e", "g", "t",
                                    "Va", "Vb", "Vc", "Vd", "Vx", "Vy", 
                                    "Eev", "Er", "Eb",
                                    "Ra", "Rb", "Rc", "Rd", "Re",  0};

double Lens::getHFOV() const
{
    return const_map_get(this->variables,"v").getValue();
}

void Lens::setHFOV(double d)
{
    map_get(variables,"v").setValue(d);
}

double Lens::getFocalLength() const
{

    double HFOV = const_map_get(variables,"v").getValue();
#if 0
    if (isLandscape()) {
        ssize = m_sensorSize;
    } else {
        ssize.y = m_sensorSize.x;
        ssize.x = m_sensorSize.y;
    }
#endif

    switch (m_projectionFormat)
    {
        case RECTILINEAR:
            return (m_sensorSize.x/2.0) / tan(HFOV/180.0*M_PI/2);
            break;
        case CIRCULAR_FISHEYE:
        case FULL_FRAME_FISHEYE:
            // same projection equation for both fisheye types,
            // assume equal area projection.
            return m_sensorSize.x / (HFOV/180*M_PI);
            break;
        default:
            // TODO: add formulas for other projections
            DEBUG_WARN("Focal length calculations only supported with rectilinear and fisheye images");
            return 0;
    }
}

void Lens::setEV(double ev)
{
    map_get(variables, "Eev").setValue(ev);
}

void Lens::setFocalLength(double fl)
{
#if 0
    if (isLandscape()) {
        ssize = m_sensorSize;
    } else {
        ssize.y = m_sensorSize.x;
        ssize.x = m_sensorSize.y;
    }
#endif

    double hfov=map_get(variables, "v").getValue();
    switch (m_projectionFormat) {
        case RECTILINEAR:
            hfov = 2*atan((m_sensorSize.x/2.0)/fl)  * 180.0/M_PI;
            break;
        case CIRCULAR_FISHEYE:
        case FULL_FRAME_FISHEYE:
            hfov = m_sensorSize.x / fl * 180/M_PI;
        default:
            // TODO: add formulas for other projections
            DEBUG_WARN("Focal length calculations only supported with rectilinear and fisheye images");
    }
    map_get(variables, "v").setValue(hfov);
}


void Lens::setCropFactor(double factor)
{
    // calculate diagonal on our sensor
    double d = sqrt(36.0*36.0 + 24.0*24.0) / factor;

    double r = (double)m_imageSize.x / m_imageSize.y;

    // calculate the sensor width and height that fit the ratio
    // the ratio is determined by the size of our image.
    m_sensorSize.x = d / sqrt(1 + 1/(r*r));
    m_sensorSize.y = m_sensorSize.x / r;
}

double Lens::getCropFactor() const
{
    double d2 = m_sensorSize.x*m_sensorSize.x + m_sensorSize.y*m_sensorSize.y;
    return sqrt(36.0*36+24*24) / sqrt(d2);
}


double Lens::getAspectRatio() const
{
    return (double)m_imageSize.x / m_imageSize.y;
}


bool Lens::isLandscape() const
{
    return m_imageSize.x >= m_imageSize.y;
}


void Lens::update(const Lens & l)
{
    m_projectionFormat = l.m_projectionFormat;
    m_sensorSize = l.m_sensorSize;
    m_imageSize = l.m_imageSize;
    variables = l.variables;
}



} //namespace

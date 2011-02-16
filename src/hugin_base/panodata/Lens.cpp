// -*- c-basic-offset: 4 -*-

/** @file Lens.cpp
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
    : m_hasExif(false), m_projectionFormat(BaseSrcPanoImage::RECTILINEAR),
      m_imageSize(0,0), m_cropFactor(1.0)
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
    return SrcPanoImage::calcFocalLength(m_projectionFormat,HFOV,getCropFactor(),m_imageSize);
}

void Lens::setEV(double ev)
{
    map_get(variables, "Eev").setValue(ev);
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
    m_cropFactor = l.getCropFactor();    m_imageSize = l.m_imageSize;
    variables = l.variables;
}



} //namespace

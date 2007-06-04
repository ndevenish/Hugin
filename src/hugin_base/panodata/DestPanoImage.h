// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
 *
 *  @brief implementation of HFOVDialog Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoImage.h 1970 2007-04-18 22:26:56Z dangelo $
 *
 *  !! from PanoImage.h 1970
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

#ifndef PANOIMAGE_H
#define PANOIMAGE_H

#include <iostream>
#include <vector>
#include <common/utils.h>
#include <common/math.h>
#include <vigra/diff2d.hxx>

namespace PT {

/** Holds information about the destination image.
 */
class DestPanoImage
{
public:
    enum Projection { RECTILINEAR = 0,
                      CYLINDRICAL = 1,
                      EQUIRECTANGULAR = 2,
                      FULL_FRAME_FISHEYE = 3};
 
    DestPanoImage()
    {
        m_proj = EQUIRECTANGULAR;
        m_hfov = 360;
        m_size = vigra::Size2D(360,180);
        m_roi = vigra::Rect2D(m_size);
    }

    DestPanoImage(Projection proj, double hfov, vigra::Size2D sz)
     : m_proj(proj), m_hfov(hfov), m_size(sz), m_roi(sz)
    {
    }
    bool horizontalWarpNeeded()
    {
        switch (m_proj)
        {
            case CYLINDRICAL:
            case EQUIRECTANGULAR:
                if (m_hfov == 360) return true;
            case FULL_FRAME_FISHEYE:
            case RECTILINEAR:
            default:
                break;
        }
        return false;
    }
    // data accessors
    const Projection & getProjection() const
    { return m_proj; }
    void setProjection(const Projection & val)
    { m_proj = val; }

    const double & getHFOV() const
    { return m_hfov; }
    void setHFOV(const double & val)
    { m_hfov = val; }

    const vigra::Size2D & getSize() const
    { return m_size; }
    void setSize(const vigra::Size2D & val)
    { m_size = val; }

    const vigra::Rect2D & getROI() const
    { return m_roi; }
    void setROI(const vigra::Rect2D & val)
    { m_roi = val; }

private:
    Projection m_proj;
    double m_hfov;
    vigra::Size2D m_size;
    vigra::Rect2D m_roi;
};


} // namespace

#endif // PANOIMAGE_H

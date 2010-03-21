// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
 *
 *  @brief implementation of HFOVDialog Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#ifndef _PANODATA_DESTPANOIMAGE_H
#define _PANODATA_DESTPANOIMAGE_H

#include <hugin_shared.h>
#include <vigra/diff2d.hxx>


namespace HuginBase {

    
/** Holds information about the destination image.
 */
class IMPEX DestPanoImage
{
        
    public:
        ///
        enum Projection {
            RECTILINEAR = 0,
            CYLINDRICAL = 1,
            EQUIRECTANGULAR = 2,
            FULL_FRAME_FISHEYE = 3
        };
     
        
    public:
        ///
        DestPanoImage()
          : m_proj(EQUIRECTANGULAR),
            m_hfov(360),
            m_size(vigra::Size2D(360,180)),
            m_roi(vigra::Size2D(360,180))
        {}
        
        ///
        DestPanoImage(Projection proj, double hfov, vigra::Size2D sz)
          : m_proj(proj),
            m_hfov(hfov),
            m_size(sz),
            m_roi(sz)
        {}
        
        
    public:
        ///
        bool horizontalWarpNeeded() const
        {
            if (m_proj == CYLINDRICAL || m_proj == EQUIRECTANGULAR)
            {
                if (m_hfov == 360)
                    return true;
            }
            
            return false;
        }
        
        
        // data accessors

    public:
        ///
        const Projection & getProjection() const
            { return m_proj; }
        
        ///
        void setProjection(const Projection & val)
            { m_proj = val; }

        ///
        const double & getHFOV() const
            { return m_hfov; }
        
        ///
        void setHFOV(const double & val)
            { m_hfov = val; }

        ///
        const vigra::Size2D & getSize() const
            { return m_size; }
        
        ///
        void setSize(const vigra::Size2D & val)
            { m_size = val; }

        ///
        const vigra::Rect2D & getROI() const
            { return m_roi; }
        
        ///
        void setROI(const vigra::Rect2D & val)
            { m_roi = val; }

        
    private:
        Projection m_proj;
        double m_hfov;
        vigra::Size2D m_size;
        vigra::Rect2D m_roi;
    
};


} // namespace

#endif // _H

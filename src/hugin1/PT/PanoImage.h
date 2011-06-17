// -*- c-basic-offset: 4 -*-

/** @file hugin1/PT/PanoImage.h
 *
 *  @brief implementation of HFOVDialog Class
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

#ifndef _Hgn1_PANOIMAGE_H
#define _Hgn1_PANOIMAGE_H

#include <panodata/SrcPanoImage.h>

namespace PT {

    using HuginBase::SrcPanoImage;
    
    inline bool initImageFromFile(SrcPanoImage & img, double & focalLength, double & cropFactor, bool applyExposureValue)
    {
        return img.readEXIF(focalLength, cropFactor, true, applyExposureValue);
    }

    inline double calcHFOV(SrcPanoImage::Projection proj, double fl, double crop, vigra::Size2D imageSize)
    {
        return HuginBase::SrcPanoImage::calcHFOV(proj,fl,crop,imageSize);
    }

    inline double calcFocalLength(SrcPanoImage::Projection proj, double hfov, double crop, vigra::Size2D imageSize)
    {
        return HuginBase::SrcPanoImage::calcFocalLength(proj,hfov,crop,imageSize);
    }
    
    inline double calcCropFactor(SrcPanoImage::Projection proj, double hfov, double focalLength, vigra::Size2D imageSize)
    {
        return HuginBase::SrcPanoImage::calcCropFactor(proj,hfov,focalLength,imageSize);
    }
    
} // namespace

#endif // PANOIMAGE_H

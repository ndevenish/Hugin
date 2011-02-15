// -*- c-basic-offset: 4 -*-
/** @file PanoImage.h
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

#ifndef _PANODATA_PANOIMAGE_H
#define _PANODATA_PANOIMAGE_H

#include <hugin_config.h>

#include <iostream>
#include <vector>
#include <vigra/diff2d.hxx>
#include <hugin_math/hugin_math.h>


namespace HuginBase {

/** optimization & stitching options. 
 *  @deprecated this structure is depreacated and will be removed in future
 *  do not use in new code */
struct ImageOptions {

    ImageOptions()
        : featherWidth(10),
          ignoreFrameWidth(0),
          morph(false),
          docrop(false),
          autoCenterCrop(true),
          m_vigCorrMode(VIGCORR_RADIAL|VIGCORR_DIV),
          responseType(0),
          active(true)
     { };

    
    /// u10           specify width of feather for stitching. default:10
    unsigned int featherWidth;
    
    /// m20           ignore a frame 20 pixels wide. default: 0
    unsigned int ignoreFrameWidth;

    /// Morph-to-fit using control points.
    bool morph;

    // crop parameters
    bool docrop;
    bool autoCenterCrop;
    vigra::Rect2D cropRect;

    
    /// vignetting correction mode (bitflags, no real enum)
    enum VignettingCorrMode { 
        VIGCORR_NONE = 0,      ///< no vignetting correction
        VIGCORR_RADIAL = 1,    ///< radial vignetting correction
        VIGCORR_FLATFIELD = 2, ///< flatfield correction
        VIGCORR_DIV = 4        ///< correct by division.
    };
    
    int m_vigCorrMode;
    
    // coefficients for vignetting correction (even degrees: 0,2,4,6, ...)
    std::string m_flatfield;
    
    // the response type (Rt)
    int responseType;

    // is image active (displayed in preview and used for optimisation)
    bool active;
};

} // namespace
#endif // _H

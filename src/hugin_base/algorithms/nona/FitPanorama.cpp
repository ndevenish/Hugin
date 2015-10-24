// -*- c-basic-offset: 4 -*-
/** @file FitPanorama.cpp
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

#include "FitPanorama.h"

#include <algorithm>
#include <panodata/PanoramaData.h>
#include <panotools/PanoToolsInterface.h>
#include <algorithms/nona/CalculateFOV.h>

namespace HuginBase {

void CalculateFitPanorama::fitPano(PanoramaData& panorama, double& HFOV, double& height)
{
    // FIXME: doesn't work properly for fisheye and mirror projections,
    // it will not calculate a vfov bigger than 180.
    hugin_utils::FDiff2D fov = CalculateFOV::calcFOV(panorama);
    
    // use estimated fov to calculate a suitable panorama height.
    // calculate VFOV based on current panorama
    PTools::Transform transf;
    SrcPanoImage src;
    src.setProjection(SrcPanoImage::EQUIRECTANGULAR);
    src.setHFOV(360);
    src.setSize(vigra::Size2D(360,180));
    
    // output pano with new hfov
    PanoramaOptions opts = panorama.getOptions();
    opts.setHFOV(fov.x, false);
    transf.createInvTransform(src, opts);
    
    // limit fov to suitable range for this projection
    fov.x = std::min(fov.x, panorama.getOptions().getMaxHFOV());
    fov.y = std::min(fov.y, panorama.getOptions().getMaxVFOV());
    
    hugin_utils::FDiff2D pmiddle;
    // special case for projections with max VFOV > 180 (fisheye, stereographic)
    if (panorama.getOptions().getMaxVFOV() >  180 && fov.x > 180) {
        transf.transform(pmiddle, hugin_utils::FDiff2D(180, 180 - fov.x / 2 + 0.01));
    } else {
        transf.transform(pmiddle, hugin_utils::FDiff2D(0, fov.y / 2));
    }
    
    height = fabs(2*pmiddle.y);
    HFOV = fov.x;
}


bool FitPanorama::runAlgorithm()
{
    if( CalculateFitPanorama::runAlgorithm() )
    {
        
        PanoramaOptions opts = o_panorama.getOptions();
        
        opts.setHFOV(getResultHorizontalFOV());
        opts.setHeight(hugin_utils::roundi(getResultHeight()));
        
        o_panorama.setOptions(opts);
        
        return true; // let's hope so.
        
    }
    
    return false;
}

} //namespace

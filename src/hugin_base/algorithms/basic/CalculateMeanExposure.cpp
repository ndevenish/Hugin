// -*- c-basic-offset: 4 -*-
/** @file CalculateMeanExposure.cpp
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

#include "CalculateMeanExposure.h"

#include <panodata/PanoramaData.h>


namespace HuginBase {


///
double CalculateMeanExposure::calcMeanExposure(const PanoramaData& pano)
{
    double exposure=0;
    size_t i;
    for (i = 0; i < pano.getNrOfImages(); i++) {
        exposure += const_map_get(pano.getImageVariables(i),"Eev").getValue();
    }
    return exposure / i;
}


}

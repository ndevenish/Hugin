// -*- c-basic-offset: 4 -*-
/**  @file CleanCP.h
 *
 *  @brief algorithms for remove control points by statistic method
 *  
 *  the algorithm is based on ptoclean by Bruno Postle
 *
 *  @author Thomas Modes
 *
 *  $Id$
 *
 */
 
 /*  This is free software; you can redistribute it and/or
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

#ifndef _CLEANCP_H
#define _CLEANCP_H

#include <hugin_shared.h>
#include <panodata/Panorama.h>
#include <appbase/ProgressDisplay.h>

namespace HuginBase {

/** optimises images pairwise and removes for every image pair control points with error > mean+n*sigma 
  @param pano panorama which should be used
  @param n determines, how big the deviation from mean should be to determine wrong control points, default 2.0
  @return set which contains control points with error > mean+n*sigma */
IMPEX UIntSet getCPoutsideLimit_pair(Panorama pano, AppBase::ProgressDisplay& progress, double n=2.0);
/** optimises the whole panorama and removes all control points with error > mean+n*sigma 
  @param pano panorama which should be used
  @param n determines, how big the deviation from mean should be to determine wrong control points, default 2.0
  @param skipOptimisation skips the optimisation step, the current position of the images is used
  @param includeLineCp include also line control points when calculating mean and check them also for limit
  @return set which contains control points with error > mean+n*sigma */
IMPEX UIntSet getCPoutsideLimit(Panorama pano, double n = 2.0, bool skipOptimisation = false, bool includeLineCp = false);

/** returns these control points, which are in masks */
IMPEX UIntSet getCPinMasks(Panorama pano);

}  // namespace
#endif // _H

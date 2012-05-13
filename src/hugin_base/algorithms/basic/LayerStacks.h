// -*- c-basic-offset: 4 -*-
/** @file LayerStacks.h
 *
 *  @brief declaration of functions to handle stacks and layers
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _BASICALGORITHMS_LAYERSTACKS_H
#define _BASICALGORITHMS_LAYERSTACKS_H

#include <panodata/PanoramaData.h>

namespace HuginBase
{
/** returns vector of set of output stacks */
IMPEX std::vector<UIntSet> getHDRStacks(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts);
/** returns vector of set of output exposure layers */
IMPEX std::vector<UIntSet> getExposureLayers(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts);
/** returns set of images which are visible in output ROI */
IMPEX UIntSet getImagesinROI (const PanoramaData& pano, const UIntSet activeImages);
}

#endif /* _BASICALGORITHMS_LAYERSTACKS_H */

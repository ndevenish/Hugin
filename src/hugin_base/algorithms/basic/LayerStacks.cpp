// -*- c-basic-offset: 4 -*-
/** @file LayerStacks.cpp
 *
 *  @brief implementation of functions to handle stacks and layers
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

#include "LayerStacks.h"

#include <panodata/PanoramaData.h>
#include <algorithms/basic/CalculateOverlap.h>
#include <algorithms/nona/ComputeImageROI.h>

namespace HuginBase
{
using namespace std;

vector<UIntSet> getHDRStacks(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts)
{
    vector<UIntSet> result;

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

    UIntSet stack;

    CalculateImageOverlap overlap(&pano);
    overlap.calculate(10);  // we are testing 10*10=100 points
    do {
        unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        for (UIntSet::iterator it = allImgs.begin(); it !=  allImgs.end(); ) {
            unsigned srcImg2 = *it;
            it++;
            if(overlap.getOverlap(srcImg,srcImg2)>opts.outputStacksMinOverlap)
            {
                stack.insert(srcImg2);
                allImgs.erase(srcImg2);
            }
        }
        result.push_back(stack);
        stack.clear();
    } while (allImgs.size() > 0);

    return result;
}

vector<UIntSet> getExposureLayers(const PanoramaData & pano, UIntSet allImgs, PanoramaOptions opts)
{
    vector<UIntSet> result;

    // if no images are available, return empty result vector
    if ( allImgs.empty() )
    {
        return result;
    }

    UIntSet stack;

    do {
        unsigned srcImg = *(allImgs.begin());
        stack.insert(srcImg);
        allImgs.erase(srcImg);

        // find all images that have a suitable overlap.
        SrcPanoImage simg = pano.getSrcImage(srcImg);
        double maxEVDiff = opts.outputLayersExposureDiff;
        for (UIntSet::iterator it = allImgs.begin(); it !=  allImgs.end(); ) {
            unsigned srcImg2 = *it;
            it++;
            SrcPanoImage simg2 = pano.getSrcImage(srcImg2);
            if ( fabs(simg.getExposureValue() - simg2.getExposureValue()) < maxEVDiff )
            {
                stack.insert(srcImg2);
                allImgs.erase(srcImg2);
            }
        }
        result.push_back(stack);
        stack.clear();
    } while (allImgs.size() > 0);

    return result;
}

UIntSet getImagesinROI (const PanoramaData& pano, const UIntSet activeImages)
{
    UIntSet images;
    PanoramaOptions opts = pano.getOptions();
    for (UIntSet::const_iterator it = activeImages.begin(); it != activeImages.end(); ++it)
    {
        vigra::Rect2D roi = estimateOutputROI(pano, opts, *it);
        if (! (roi.isEmpty())) {
            images.insert(*it);
        }
    }
    return images;
}

}

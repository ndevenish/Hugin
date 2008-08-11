// -*- c-basic-offset: 4 -*-

/** @file InterestPoints.h
 *
 *  @brief compute interest points
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: align_image_stack.cpp 2493 2007-10-24 20:26:26Z dangelo $
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

#ifndef VIGRA_EXT_INTEREST_POINTS
#define VIGRA_EXT_INTEREST_POINTS

#include <vector>
#include <map>

#include <vigra/error.hxx>
#include <vigra/impex.hxx>
#include <vigra/cornerdetection.hxx>
#include <vigra/localminmax.hxx>
#include <vigra/diff2d.hxx>

namespace vigra_ext
{

template <class ImageIter, class ImageAcc>
void findInterestPointsOnGrid(vigra::triple<ImageIter, ImageIter, ImageAcc> img, double scale,
 unsigned nPoints, unsigned grid, std::vector<std::multimap<double, vigra::Diff2D> > &allPoints
)
{
    //////////////////////////////////////////////////
    // find interesting corners using harris corner detector

    vigra::Diff2D size2 = img.second - img.first;
    vigra::Size2D size(size2.x, size2.y);

    for (unsigned party=0;party<grid;party++) {
        for (unsigned partx=0;partx<grid;partx++) {
            std::multimap<double, vigra::Diff2D> points;
            DEBUG_TRACE("selecting points for grid partition (" << partx << ", " << party << ")");
	    // select the nPoints with the highest response
	    // some distribution criteria might be useful, too
	    // to avoid clustering all points on a single object.

            // run corner detector only in current sub-region (saves a lot of memory for big images)
            vigra::Rect2D rect(partx*size.x/grid, party*size.y/grid,
                               (partx+1)*size.x/grid, (party+1)*size.y/grid);

            rect &= vigra::Rect2D(size);

            vigra::BImage leftCorners(rect.size(), vigra::UInt8(0));
            vigra::FImage leftCornerResponse(rect.size());

            DEBUG_DEBUG("running corner detector. nPoints: " << nPoints << ",  scale: " << scale );
            // find corner response at scale scale
            vigra::cornerResponseFunction(srcIterRange(img.first + rect.upperLeft(), img.first + rect.lowerRight(), img.third),
                                          vigra::destImage(leftCornerResponse),
                                          scale);

            // find local maxima of corner response, mark with 1
            vigra::localMaxima(vigra::srcImageRange(leftCornerResponse), vigra::destImage(leftCorners), 255);

            double minResponse = 0;
            // sample grid on img1 and try to add ctrl points
            for (int y=0; y < rect.height(); y++ ) {
                for (int x=0; x < rect.width(); x++ ) {
                    if (leftCorners(x,y) == 0) {
                        continue;
                    }
                    double resp = leftCornerResponse(x,y);
                    if (resp > minResponse) {
                        // add to point map
                        points.insert(std::make_pair(resp, vigra::Diff2D(x,y) + rect.upperLeft()));
                        // if we have reached the maximum
                        // number of points, increase the threshold, to avoid
                        // growing the points map too much.
                        // extract more than nPoints, because some might be bad
                        // and cannot be matched with the other image.
                        if (points.size() > nPoints) {
                            // remove the point with the lowest corner response.
//                            leftCorners(points.begin()->second.x,points.begin()->second.y)=0;
                            points.erase(points.begin());
                            // use new threshold for next selection.
                            minResponse = points.begin()->first;
                        }
                    }
                }
            }

            allPoints.push_back(points);
        }
    }
}

} // namespace

#endif

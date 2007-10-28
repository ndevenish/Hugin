// -*- c-basic-offset: 4 -*-
/** @file blend.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _VIGRA_EXT_BLEND_H
#define _VIGRA_EXT_BLEND_H

#include <vigra/impex.hxx>
#include <hugin_utils/utils.h>
#include <appbase/ProgressDisplayOld.h>

#include <vigra_ext/utils.h>
#include <vigra_ext/NearestFeatureTransform.h>
#include <vigra_ext/ROIImage.h>


//#define DEBUG_MASKIMG


namespace vigra_ext
{


/** blends two images, they overlap and the iterators point
 *  to excatly the same position.
 */
template <typename ImgIter, typename ImgAccessor,
          typename ImgMaskIter, typename ImgMaskAccessor,
          typename PanoIter, typename PanoAccessor,
          typename MaskIter, typename MaskAccessor>
void blendOverlap(vigra::triple<ImgIter, ImgIter, ImgAccessor> image,
                  std::pair<ImgMaskIter, ImgMaskAccessor> imageMask,
		  std::pair<PanoIter, PanoAccessor> pano,
		  std::pair<MaskIter, MaskAccessor> panoMask,
                  AppBase::MultiProgressDisplay & progress)
{
    vigra::Diff2D size = image.second - image.first;

#ifdef DEBUG_MASKIMG
    // save the masks
    vigra::exportImage(srcIterRange(imageMask.first, imageMask.first + size),
                vigra::ImageExportInfo("blendImageMask_before.tif"));
    vigra::exportImage(srcIterRange(panoMask.first, panoMask.first + size),
                vigra::ImageExportInfo("blendPanoMask_before.tif"));
	
#endif

    // create new blending masks
    vigra::BasicImage<typename MaskIter::value_type> blendPanoMask(size);
    vigra::BasicImage<typename MaskIter::value_type> blendImageMask(size);

    // calculate the stitching masks.
    vigra_ext::nearestFeatureTransform(srcIterRange(panoMask.first, panoMask.first + size),
                                       imageMask,
                                       destImage(blendPanoMask),
                                       destImage(blendImageMask),
                                       progress);

#ifdef DEBUG_MASKIMG
     // save the masks
     vigra::exportImage(srcImageRange(blendImageMask), vigra::ImageExportInfo("blendImageMask.tif"));
     vigra::exportImage(srcImageRange(blendPanoMask), vigra::ImageExportInfo("blendPanoMask.tif"));
	
#endif
     // copy the image into the panorama
     vigra::copyImageIf(image, vigra::maskImage(blendImageMask), pano);
     // copy mask
     vigra::copyImageIf(vigra::srcImageRange(blendImageMask), vigra::maskImage(blendImageMask), panoMask);
}

/** blend \p img into \p pano, using \p alpha mask and \p panoROI
 *
 *  updates \p pano, \p alpha and \p panoROI.
 *
 *  \param img      is the image that should be blended,
 *  \param pano     destinatation image
 *  \param alpha    destination image alpha
 *  \param panoROI  used part of pano. Does not indicate the allocated part!
 *                  It is supposed that \p pano is defined at least where
 *                  \p img is defined.
 *  \param progress Display progress there.
 */
template <typename ImageType, typename AlphaImageType,
          typename PanoIter, typename PanoAccessor,
          typename AlphaIter, typename AlphaAccessor>
void blend(vigra_ext::ROIImage<ImageType, AlphaImageType> & img,
           vigra::triple<PanoIter, PanoIter, PanoAccessor> pano,
           std::pair<AlphaIter, AlphaAccessor> alpha,
           vigra::Rect2D & panoROI,
           AppBase::MultiProgressDisplay & progress)
{
    typedef typename AlphaIter::value_type AlphaValue;
    // calculate the overlap by intersecting the two image
    // rectangles.
    DEBUG_DEBUG("image bounding Box: " << img.boundingBox());
    DEBUG_DEBUG("pano size: " << pano.second - pano.first);
    DEBUG_DEBUG("pano roi: " << panoROI);
    vigra::Rect2D overlap = img.boundingBox() & panoROI;
    DEBUG_DEBUG("overlap: " << overlap);

    if (!overlap.isEmpty()) {
        // image ROI's overlap.. calculate real overlapping area.

	// corner points of overlapping area.
	vigra::Point2D overlapUL(INT_MAX, INT_MAX);
	vigra::Point2D overlapLR(0,0);
	AlphaIter alphaIter = alpha.first + overlap.upperLeft();
//	typename AlphaImageType::traverser imgAlphaIter = img.maskUpperLeft()
//                                                      + overlap.upperLeft();
// DGSW FIXME - Unreferenced
//	typename AlphaImageType::Accessor imgAlphaAcc = img.maskAccessor();
	// find real, overlapping ROI, by iterating over ROI
	for (int y=overlap.top(); y < overlap.bottom(); y++, ++(alphaIter.y))
        {
	    for (int x=overlap.left(); x < overlap.right(); x++,
                  ++(alphaIter.x)) {
		// check if images overlap
		if (img.getMask(x,y) > 0 && alpha.second(alpha.first,vigra::Diff2D(x,y)) > 0) {
                    // overlap, use it to calculate bounding box
                    if (overlapUL.x > x) overlapUL.x = x;
                    if (overlapUL.y > y) overlapUL.y = y;
                    if (overlapLR.x < x) overlapLR.x = x;
                    if (overlapLR.y < y) overlapLR.y = y;
		}
            }
        }
        if (overlapUL.x != INT_MAX) {
            // the real overlap. we could have copied the image here, but
            // we leave that to the real blending routine.
            vigra::Rect2D realOverlap(overlapUL, overlapLR + vigra::Point2D(1,1));

	    // images overlap, call real blending routine
	    blendOverlap(applyRect(realOverlap, vigra_ext::srcImageRange(img)),
		         applyRect(realOverlap, vigra_ext::srcMask(img)),
			 applyRect(realOverlap, std::make_pair(pano.first, pano.third)),
			 applyRect(realOverlap, alpha),
                         progress);

            // now, copy the non-overlapping parts

            // upper stripe
            vigra::Rect2D border(img.boundingBox().left(),
                                 img.boundingBox().top(),
                                 img.boundingBox().right(),
                                 realOverlap.top());
            // copy image
            vigra::copyImageIf(applyRect(border, vigra_ext::srcImageRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, std::make_pair(pano.first,pano.third)));
            // copy mask
            vigra::copyImageIf(applyRect(border, vigra_ext::srcMaskRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, alpha));

            // left stripe
            border.setUpperLeft(vigra::Point2D(img.boundingBox().left(),
                                               realOverlap.top()));
            border.setLowerRight(vigra::Point2D(realOverlap.left(),
                                                realOverlap.bottom()));
            // copy image
            vigra::copyImageIf(applyRect(border, vigra_ext::srcImageRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, std::make_pair(pano.first,pano.third)));
            // copy mask
            vigra::copyImageIf(applyRect(border, vigra_ext::srcMaskRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, alpha));

            // right stripe
            border.setUpperLeft(vigra::Point2D(realOverlap.right(),
                                realOverlap.top()));
            border.setLowerRight(vigra::Point2D(img.boundingBox().right(),
                                 realOverlap.bottom()));
            // copy image
            vigra::copyImageIf(applyRect(border, vigra_ext::srcImageRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, std::make_pair(pano.first,pano.third)));
            // copy mask
            vigra::copyImageIf(applyRect(border, vigra_ext::srcMaskRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, alpha));

            // lower stripe
            border.setUpperLeft(vigra::Point2D(img.boundingBox().left(),
                                               realOverlap.bottom()));
            border.setLowerRight(vigra::Point2D(img.boundingBox().right(),
                                                img.boundingBox().bottom()));
            // copy image
            vigra::copyImageIf(applyRect(border, vigra_ext::srcImageRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, std::make_pair(pano.first,pano.third)));
            // copy mask
            vigra::copyImageIf(applyRect(border, vigra_ext::srcMaskRange(img)),
                               applyRect(border, vigra_ext::srcMask(img)),
                               applyRect(border, alpha));
        } else {
            DEBUG_DEBUG("ROI's overlap, but no overlapping pixels found");
            // copy image
            vigra::copyImageIf(applyRect(img.boundingBox(), vigra_ext::srcImageRange(img)),
                               applyRect(img.boundingBox(), vigra_ext::srcMask(img)),
                               applyRect(img.boundingBox(), std::make_pair(pano.first,pano.third)));
            // copy mask
            vigra::copyImageIf(applyRect(img.boundingBox(), vigra_ext::srcMaskRange(img)),
                               applyRect(img.boundingBox(), vigra_ext::srcMask(img)),
                               applyRect(img.boundingBox(), alpha));
        }
    } else {
	// image ROI's do not overlap, no blending, just copy
	// alpha channel is not considered, because the whole target
	// is free.
        // copy image
        DEBUG_DEBUG("image rect: " <<img.boundingBox());
        DEBUG_DEBUG("panorama size: " << pano.second - pano.first); 
        vigra::copyImage(applyRect(img.boundingBox(), vigra_ext::srcImageRange(img)),
                         applyRect(img.boundingBox(), std::make_pair(pano.first,pano.third)));
        // copy mask
        vigra::copyImage(applyRect(img.boundingBox(), vigra_ext::srcMaskRange(img)),
                         applyRect(img.boundingBox(), alpha));

    }
}


} // namespace


#endif // _H

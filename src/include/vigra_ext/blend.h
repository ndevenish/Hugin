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

#ifndef _BLEND_H
#define _BLEND_H

#include <common/utils.h>

#include <vigra_ext/utils.h>
#include <vigra_ext/NearestFeatureTransform.h>
#include <vigra_ext/tiffUtils.h>

#include <vigra_ext/LayerImage.h>

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
                  utils::MultiProgressDisplay & progress)
{
    vigra::Diff2D size = image.second - image.first;

#ifdef DEBUG
    // save the masks
    exportImage(srcIterRange(imageMask.first, imageMask.first + size),
                vigra::ImageExportInfo("blendImageMask_before.tif"));
    exportImage(srcIterRange(panoMask.first, panoMask.first + size),
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

#ifdef DEBUG
     // save the masks
     exportImage(srcImageRange(blendImageMask), vigra::ImageExportInfo("blendImageMask.tif"));
     exportImage(srcImageRange(blendPanoMask), vigra::ImageExportInfo("blendPanoMask.tif"));
	
#endif
     // copy the image into the panorama
     vigra::copyImageIf(image, vigra::maskImage(blendImageMask), pano);
     // copy mask
     vigra::copyImageIf(vigra::srcImageRange(blendImageMask), vigra::maskImage(blendImageMask), panoMask);
}
    
/** blend \p img into \p pano, using \p alpha mask and \p panoROI
 *
 *  updates \p pano, \p alpha and \p panoROI
 */
template <typename ImageType, typename AlphaImageType,
          typename PanoIter, typename PanoAccessor,
          typename AlphaIter, typename AlphaAccessor>
void blend(vigra_ext::LayerImage<ImageType, AlphaImageType> & img,
           vigra::triple<PanoIter, PanoIter, PanoAccessor> pano,
           std::pair<AlphaIter, AlphaAccessor> alpha,
           vigra_ext::ROI<vigra::Diff2D> & panoROI,
           utils::MultiProgressDisplay & progress)
{
    typedef typename AlphaIter::value_type AlphaValue;
    // intersect the ROI's.
    vigra_ext::ROI<vigra::Diff2D> overlap;
    const vigra_ext::ROI<vigra::Diff2D> & imgROI = img.roi();
    if (panoROI.intersect(img.roi(),overlap)) {
        // image ROI's overlap.. calculate overlap mask
	vigra::BasicImage<AlphaValue> overlapMask(overlap.size());
	vigra_ext::OverlapSizeCounter counter;
	// calculate union of panorama and image mask
	vigra::inspectTwoImages(overlap.apply(img.alpha(),imgROI),
				    overlap.apply(alpha, panoROI),
				    counter);

	DEBUG_DEBUG("overlap found: " << overlap << " pixels: " << counter.getSize());
 	if (counter.getSize() > 0) {
	    // images overlap, call real blending routine
	    blendOverlap(overlap.apply(img.image(), imgROI),
		         overlap.apply(srcIter(img.alpha().first), imgROI),
			 overlap.apply(destIter(pano.first), panoROI),
			 overlap.apply(alpha, panoROI),
                         progress);

            // now, copy the non-overlapping parts

            vigra::Diff2D imgUL = imgROI.getUL();
            vigra::Diff2D imgLR = imgROI.getLR();

            std::vector<vigra_ext::ROI<vigra::Diff2D> > borderAreas;

            vigra_ext::ROI<vigra::Diff2D> roi;

            // upper part
            roi.setCorners(imgUL,
                           vigra::Diff2D(imgLR.x, overlap.getUL().y));
            DEBUG_DEBUG("upper area: " << roi);
            if (roi.size().x > 0 && roi.size().y > 0) {
                borderAreas.push_back(roi);
            }

            // left area
            roi.setCorners(vigra::Diff2D(imgUL.x, overlap.getUL().y),
                           vigra::Diff2D(overlap.getUL().x, overlap.getLR().y));
            DEBUG_DEBUG("left area: " << roi);
            if (roi.size().x > 0 && roi.size().y > 0) {
                borderAreas.push_back(roi);
            }

            // right area
            roi.setCorners(vigra::Diff2D(overlap.getLR().x, overlap.getUL().y),
                           vigra::Diff2D(imgLR.x, overlap.getLR().y));
            DEBUG_DEBUG("right area: " << roi);
            if (roi.size().x > 0 && roi.size().y > 0) {
                borderAreas.push_back(roi);
            }

            // bottom area
            roi.setCorners(vigra::Diff2D(imgUL.x, overlap.getLR().y),
                           imgLR);
            DEBUG_DEBUG("bottom area: " << roi);
            if (roi.size().x > 0 && roi.size().y > 0) {
                borderAreas.push_back(roi);
            }

            for (std::vector<vigra_ext::ROI<vigra::Diff2D> >::iterator it = borderAreas.begin();
                it != borderAreas.end();
                ++it)
            {
                // copy image with mask.
                vigra::copyImageIf((*it).apply(img.image(), imgROI),
                                   (*it).apply(srcIter(img.alpha().first), imgROI),
                                   (*it).apply(destIter(pano.first),panoROI) );
                // copy mask
                vigra::copyImageIf((*it).apply(img.alpha(),imgROI),
                                   (*it).apply(maskIter(img.alpha().first),imgROI),
                                   (*it).apply(alpha,panoROI) );
            }
        } else {
            // copy image with mask.
            vigra::copyImageIf(img.image(),
                               maskIter(img.alpha().first),
                               img.roi().apply(destIter(pano.first),panoROI) );
            // copy mask
            vigra::copyImageIf(img.alpha(),
                               maskIter(img.alpha().first),
                               img.roi().apply(alpha,panoROI) );
        }
    } else {
	// image ROI's do not overlap, no blending, just copy
	// alpha channel is not considered, because the whole target
	// is free.
	vigra::copyImage(img.image(),
	                 img.roi().apply(destIter(pano.first)));

        // copy mask
	vigra::copyImage(img.alpha(),
	                 img.roi().apply(alpha));
    }
    img.roi().unite(panoROI, panoROI);
}


} // namespace


#endif // _BLEND_H

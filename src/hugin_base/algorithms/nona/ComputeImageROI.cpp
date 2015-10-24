// -*- c-basic-offset: 4 -*-
/** @file ComputeImageROI.h
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

#include "ComputeImageROI.h"

#include <algorithm>
#include <nona/RemappedPanoImage.h>
#include <nona/RemappedPanoImage.h>



namespace HuginBase {

/** calculate the outline of the image
 *
 *  @param src       description of source picture
 *  @param dest      description of output picture (panorama)
 *  @param imgRect   output: position of image in panorama.
 */
template <class TRANSFORM>
void estimateImageRect(const SrcPanoImage & src,
                       const PanoramaOptions & dest,
                       TRANSFORM & transf,
                       vigra::Rect2D & imgRect);
    
///
template <class TRANSFORM>
void estimateImageAlpha(const SrcPanoImage & src,
                        const PanoramaOptions & dest,
                        TRANSFORM & transf,
                        vigra::Rect2D & imgRect,
                         vigra::BImage & alpha,
                         double & scale);
                         
        
    
template <class TRANSFORM>
void estimateImageAlpha(const SrcPanoImage & src,
                        const PanoramaOptions & dest,
                       TRANSFORM & transf,
                       vigra::Rect2D & imgRect,
                       vigra::BImage & alpha,
                       double & scale)
{
    hugin_utils::FDiff2D ul, lr;
    ul.x = DBL_MAX;
    ul.y = DBL_MAX;
    lr.x = -DBL_MAX;
    lr.y = -DBL_MAX;

    // remap into a miniature version of the pano and use
    // that to check for boundaries. This should be much more
    // robust than the old code that tried to trace the boundaries
    // of the images using the inverse transform, which could be fooled
    // easily by fisheye images.

    double maxLength = 180;
    scale = std::min(maxLength/dest.getSize().x, maxLength/dest.getSize().y);

    // take dest roi into account...
    vigra::Size2D destSz;
    destSz.x = hugin_utils::ceili(dest.getSize().x * scale);
    destSz.y = hugin_utils::ceili(dest.getSize().y * scale);
    vigra::Rect2D destRect;
    destRect.setUpperLeft(vigra::Point2D (hugin_utils::floori(dest.getROI().left() * scale),
                          hugin_utils::floori(dest.getROI().top() * scale)));
    destRect.setLowerRight(vigra::Point2D (hugin_utils::ceili(dest.getROI().right() * scale),
                   hugin_utils::ceili(dest.getROI().bottom() * scale)));
    destRect = destRect & vigra::Rect2D(destSz);

    DEBUG_DEBUG("scale " << scale);
    DEBUG_DEBUG("dest roi " << dest.getROI());
    DEBUG_DEBUG("dest Sz: " << destSz);
    DEBUG_DEBUG("dest rect: " << destRect);

    hugin_utils::FDiff2D cropCenter;
    double radius2=0;
    if (src.getCropMode() == SrcPanoImage::CROP_CIRCLE) {
        cropCenter.x = src.getCropRect().left() + src.getCropRect().width()/2.0;
        cropCenter.y = src.getCropRect().top() + src.getCropRect().height()/2.0;
        radius2 = std::min(src.getCropRect().width()/2.0, src.getCropRect().height()/2.0);
        radius2 = radius2 * radius2;
    }

    // remap image
    vigra::BImage img(destSz.x, destSz.y, (unsigned char)0);
    for (int y=destRect.top(); y < destRect.bottom(); y++) {
        for (int x=destRect.left(); x < destRect.right(); x++) {
            // sample image
            // coordinates in real image pixels
            double sx,sy;
            transf.transformImgCoord(sx,sy, x/scale, y/scale);
            bool valid=true;
            if (src.getCropMode() == SrcPanoImage::CROP_CIRCLE) {
                double dx = sx - cropCenter.x;
                double dy = sy - cropCenter.y;
                if (dx*dx + dy*dy > radius2) {
                        valid = false;
                }
            } else if (!src.getCropRect().contains(vigra::Point2D(hugin_utils::roundi(sx), hugin_utils::roundi(sy))) ) {
                valid = false;
            }
            if(valid && src.hasActiveMasks())
                valid=!src.isInsideMasks(vigra::Point2D(hugin_utils::roundi(sx), hugin_utils::roundi(sy)));

            if (valid) {
                img(x,y) = 255;
/*                if ( ul.x > (x-1)/scale ) {
                    ul.x = (x-1)/scale;
                }
                if ( ul.y > (y-1)/scale ) {
                    ul.y = (y-1)/scale;
                }

                if ( lr.x < (x+1)/scale ) {
                    lr.x = (x+1)/scale;
                }
                if ( lr.y < (y+1)/scale ) {
                    lr.y = (y+1)/scale;
                }
*/
            } else {
                img(x,y) = 0;
            }
        }
    }

    alpha.resize(img.size());

        // dilate alpha image, to cover neighbouring pixels,
        // that may be valid in the full resolution image
    vigra::discDilation(vigra::srcImageRange(img),
                        vigra::destImage(alpha), 1);

    ul.x = destRect.right();
    ul.y = destRect.bottom();
    lr.x = destRect.left();
    lr.y = destRect.top();

    for (int y=destRect.top(); y < destRect.bottom(); y++) {
        for (int x=destRect.left(); x < destRect.right(); x++) {
            if (alpha(x,y)) {
                if ( ul.x > x ) {
                    ul.x = x;
                }
                if ( ul.y > y ) {
                    ul.y = y;
                }

                if ( lr.x < x ) {
                    lr.x = x;
                }
                if ( lr.y < y ) {
                    lr.y = y;
                }
            }
        }
    }

    // check if we have found some pixels..
    if ( ul.x == destRect.right() || ul.y == destRect.bottom() 
         || lr.x == destRect.left()|| lr.y == destRect.top() ) {

        // no valid pixel found. return empty ROI
        imgRect = vigra::Rect2D();

        initImage(img.upperLeft()+destRect.upperLeft(), 
                  img.upperLeft()+destRect.lowerRight(),
                  img.accessor(),0);
        /*
        // no valid pixel.. strange.. either there is no image here, or we have
        // overlooked some pixel.. to be on the safe side. remap the whole image here...
        imgRect = dest.getROI();
        alpha.resize(img.size().x, img.size().y, 0);
        initImage(img.upperLeft()+destRect.upperLeft(), 
                  img.upperLeft()+destRect.lowerRight(),
                  img.accessor(),255);
        */
    } else {
        // bounding rect after scan
        DEBUG_DEBUG("ul: " << ul << "  lr: " << lr);
        ul.x = (ul.x)/scale;
        ul.y = (ul.y)/scale;
        lr.x = (lr.x+1)/scale;
        lr.y = (lr.y+1)/scale;
        imgRect.setUpperLeft(vigra::Point2D(hugin_utils::roundi(ul.x), hugin_utils::roundi(ul.y)));
        imgRect.setLowerRight(vigra::Point2D(hugin_utils::roundi(lr.x), hugin_utils::roundi(lr.y)));
        // ensure that the roi is inside the destination rect
        imgRect = dest.getROI() & imgRect;
        DEBUG_DEBUG("bounding box: " << imgRect);
    }

}

/** calculate the outline of the image
 *
 *  @param src       description of source picture
 *  @param dest      description of output picture (panorama)
 *  @param imgRect   output: position of image in panorama.
 */
template <class TRANSFORM>
void estimateImageRect(const SrcPanoImage & src, const PanoramaOptions & dest,
                       TRANSFORM & transf, vigra::Rect2D & imgRect)
{
    vigra::BImage img;
    double scale;
    estimateImageAlpha(src, dest, transf, imgRect, img, scale);
}

    vigra::Rect2D estimateOutputROI(const PanoramaData & pano, const PanoramaOptions & opts, unsigned i)
    {
        vigra::Rect2D imageRect;
        SrcPanoImage srcImg = pano.getSrcImage(i);
        PTools::Transform transf;
        transf.createTransform(srcImg, opts);
        estimateImageRect(srcImg, opts, transf, imageRect);
        return imageRect;
    }

    std::vector<vigra::Rect2D> ComputeImageROI::computeROIS(const PanoramaData& panorama,
                                                            const PanoramaOptions & opts,
                                                            const UIntSet & images)
    {
        std::vector<vigra::Rect2D> res;
        for (UIntSet::const_iterator it = images.begin(); 
             it != images.end(); ++it)
        {
            res.push_back(estimateOutputROI(panorama, panorama.getOptions(), *it));
        }
        return res;
    }

}

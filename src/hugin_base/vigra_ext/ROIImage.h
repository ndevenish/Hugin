// -*- c-basic-offset: 4 -*-
/** @file ROIImage.h
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

#ifndef _ROIIMAGE_H
#define _ROIIMAGE_H

#include <hugin_utils/utils.h>
#include <vigra/imageinfo.hxx>

#include "lut.h"

namespace vigra_ext
{


/** apply a roi to an image area
 *
 *  adjusts the iterators, so that they iterate
 *  over the region specifed by \p r
 *
 *  Ignores the lowerRight in \p img. If \p r is bigger than
 *  the new iterators will point outside of the other one.
 */
template <class ImgIter, class ImgAcc>
vigra::triple<ImgIter, ImgIter, ImgAcc>
applyRect(vigra::Rect2D & r, vigra::triple<ImgIter, ImgIter, ImgAcc> img)
{
#ifdef DEBUG
    vigra::Diff2D t = img.second - (img.first + r.lowerRight());
    DEBUG_ASSERT(t.x >= 0);
    DEBUG_ASSERT(t.y >= 0);
#endif
    return vigra::triple<ImgIter, ImgIter, ImgAcc>
           (img.first + r.upperLeft(),
            img.first + r.lowerRight(),
            img.third);
}

/** apply a roi to an image
 *
 *  adjusts the iterators, so that they iterate
 *  over the region specifed by \p r
 */
template <class ImgIter, class ImgAcc>
vigra::pair<ImgIter, ImgAcc>
applyRect(vigra::Rect2D & r, vigra::pair<ImgIter, ImgAcc> img)
{
    return vigra::pair<ImgIter, ImgAcc>
           (img.first + r.upperLeft(),
            img.second);
}



/** brief description.. This image will only hold a partial image,
 *  but look like a much bigger one. Iterators haven't been written
 *  for it yet, but maybe that will follow.
 *
 *  It should support most operations that other images support
 *  The alpha channel will be in a separate image, and can be
 *  obtained with separate functions.
 *
 */
template <class Image, class Mask>
class ROIImage
{

// typedefs for own types
	

public:
// typedefs for the children types
    typedef typename Image::value_type      image_value_type;
    typedef typename Image::traverser       image_traverser;
    typedef typename Image::const_traverser image_const_traverser;
    typedef typename Image::Accessor        ImageAccessor;
    typedef typename Image::ConstAccessor   ImageConstAccessor;

    typedef typename Mask::value_type       mask_value_type;
    typedef typename Mask::traverser        mask_traverser;
    typedef typename Mask::const_traverser  mask_const_traverser;
    typedef typename Mask::Accessor         MaskAccessor;
    typedef typename Mask::ConstAccessor    MaskConstAccessor;

    /** ctor.
     */
    ROIImage()
    {}

    /** dtor.
     */
    virtual ~ROIImage()
    {}

    /** resize the image
     *
     *  erases the current image content.
     */
    void resize(const vigra::Rect2D & rect)
    {
	m_region = rect;
	if (!m_region.isEmpty()) {
	    m_image.resize(m_region.size());
            m_mask.resize(m_region.size());
        } else {
            m_image.resize(vigra::Size2D(1,1));
            m_mask.resize(vigra::Size2D(1,1));
        }
    }

    /** returns an traverser to the upper left of the image,
     *  this is usually outside of the rectangle.
     *
     *  The traverser must not be dereferenced there. It is
     *  only valid inside the Rect.
     */
    image_traverser
    upperLeft()
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_image.size().x > 0);
        DEBUG_ASSERT(m_image.size().y > 0);
	return m_image.upperLeft() - m_region.upperLeft();
    }

    image_const_traverser
    upperLeft() const
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_image.size().x > 0);
        DEBUG_ASSERT(m_image.size().y > 0);
	return m_image.upperLeft() - m_region.upperLeft();
    }

    image_traverser
    lowerRight()
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_image.size().x > 0);
        DEBUG_ASSERT(m_image.size().y > 0);
	return m_image.upperLeft() + m_region.size();
    }

    image_const_traverser
    lowerRight() const
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_image.size().x > 0);
        DEBUG_ASSERT(m_image.size().y > 0);
	return m_image.upperLeft() + m_region.size();
    }

    /** return the accessor of this image */
    ImageAccessor accessor()
    {
	return m_image.accessor();
    }

    ImageConstAccessor accessor() const
    {
        const ROIImage<Image,Mask> & t = *this;
	return t.m_image.accessor();
    }

    /** returns an traverser to the upper left of the image,
     *  this is usually outside of the rectangle.
     *
     *  The traverser must not be dereferenced there. It is
     *  only valid inside the Rect.
     */
    mask_traverser
    maskUpperLeft()
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_mask.size().x > 0);
        DEBUG_ASSERT(m_mask.size().y > 0);
	return m_mask.upperLeft() - m_region.upperLeft();
    }

    mask_const_traverser
    maskUpperLeft() const
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_mask.size().x > 0);
        DEBUG_ASSERT(m_mask.size().y > 0);
	return m_mask.upperLeft() - m_region.upperLeft();
    }

    mask_traverser
    maskLowerRight()
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_mask.size().x > 0);
        DEBUG_ASSERT(m_mask.size().y > 0);
	return m_mask.upperLeft() + m_region.size();
    }

    mask_const_traverser
    maskLowerRight() const
    {
        DEBUG_DEBUG(m_region);
        DEBUG_ASSERT(m_mask.size().x > 0);
        DEBUG_ASSERT(m_mask.size().y > 0);
	return m_mask.upperLeft() + m_region.size();
    }

    /** return the accessor of the alpha channel */
    MaskAccessor maskAccessor()
    {
	return m_mask.accessor();
    }

    MaskConstAccessor maskAccessor() const
    {
	return m_mask.accessor();
    }

    vigra::Rect2D & boundingBox()
    {
	return m_region;
    }

    image_value_type operator()(int x, int y)
    {
	return operator[](vigra::Diff2D(x,y));
    }

    image_value_type operator[](vigra::Diff2D const & pos)
        {
            if (m_region.contains(vigra::Point2D(pos))) {
                return m_image[pos - m_region.upperLeft()];
            } else {
                return vigra::NumericTraits<image_value_type>::zero();
            }
        }


    mask_value_type getMask(int x, int y)
    {
	return getMask(vigra::Diff2D(x,y));
    }

    mask_value_type getMask(const vigra::Diff2D & pos)
        {
            if (m_region.contains(vigra::Point2D(pos))) {
                return m_mask[pos - m_region.upperLeft()];
            } else {
                return vigra::NumericTraits<mask_value_type>::zero();
            }
        }


    Image       m_image;    ///< remapped image
    Mask        m_mask;    ///< corresponding alpha channel

protected:
    vigra::Rect2D      m_region;   ///< bounding rectangle of the image
};


/** helper function for ROIImages */

template<typename Image, typename Mask>
inline vigra::triple<typename ROIImage<Image, Mask>::image_const_traverser,
                     typename ROIImage<Image, Mask>::image_const_traverser,
                     typename ROIImage<Image, Mask>::ImageConstAccessor>
srcImageRange(const ROIImage<Image,Mask> & img)
{
return vigra::triple<typename ROIImage<Image,Mask>::image_const_traverser,
                   typename ROIImage<Image,Mask>::image_const_traverser,
                   typename ROIImage<Image,Mask>::ImageConstAccessor>(img.upperLeft(),
                                                             img.lowerRight(),
                                                             img.accessor());
}


template<typename Image, typename Mask>
inline vigra::pair<typename ROIImage<Image, Mask>::image_const_traverser,
                   typename ROIImage<Image, Mask>::ImageConstAccessor>
srcImage(const ROIImage<Image,Mask> & img)
{
return vigra::pair<typename ROIImage<Image,Mask>::image_const_traverser,
                   typename ROIImage<Image,Mask>::ImageConstAccessor>(img.upperLeft(),
                                                             img.accessor());
}


template <class Image, class Alpha>
inline vigra::triple<typename ROIImage<Image,Alpha>::image_traverser,
                     typename ROIImage<Image,Alpha>::image_traverser,
                     typename ROIImage<Image,Alpha>::ImageAccessor>
destImageRange(ROIImage<Image,Alpha> & img)
{
    return vigra::triple<typename ROIImage<Image,Alpha>::image_traverser,
	        typename ROIImage<Image,Alpha>::image_traverser,
                typename ROIImage<Image,Alpha>::ImageAccessor>(img.upperLeft(),
			       			          img.lowerRight(),	
                                                          img.accessor());
}

template <class Image, class Alpha>
inline vigra::pair<typename ROIImage<Image,Alpha>::image_traverser,
                   typename ROIImage<Image,Alpha>::ImageAccessor>
destImage(ROIImage<Image,Alpha> & img)
{
    return vigra::pair<typename ROIImage<Image,Alpha>::image_traverser,
                typename ROIImage<Image,Alpha>::ImageAccessor>(img.upperLeft(),
                                                          img.accessor());
}

template <class Image, class Alpha>
inline vigra::triple<typename ROIImage<Image,Alpha>::mask_const_traverser,
                     typename ROIImage<Image,Alpha>::mask_const_traverser,
                     typename ROIImage<Image,Alpha>::MaskConstAccessor>
srcMaskRange(const ROIImage<Image,Alpha> & img)
{
    return vigra::triple<typename ROIImage<Image,Alpha>::mask_const_traverser,
	        typename ROIImage<Image,Alpha>::mask_const_traverser,
                typename ROIImage<Image,Alpha>::MaskConstAccessor>(img.maskUpperLeft(),
			       			          img.maskLowerRight(),	
                                                          img.maskAccessor());
}

template <class Image, class Alpha>
inline vigra::pair<typename ROIImage<Image,Alpha>::mask_const_traverser,
                   typename ROIImage<Image,Alpha>::MaskConstAccessor>
srcMask(const ROIImage<Image,Alpha> & img)
{
    return vigra::pair<typename ROIImage<Image,Alpha>::mask_const_traverser,
                typename ROIImage<Image,Alpha>::MaskConstAccessor>(img.maskUpperLeft(),
                                                          img.maskAccessor());
}

template <class Image, class Alpha>
inline vigra::triple<typename ROIImage<Image,Alpha>::mask_traverser,
                     typename ROIImage<Image,Alpha>::mask_traverser,
                     typename ROIImage<Image,Alpha>::MaskAccessor>
destMaskRange(ROIImage<Image,Alpha> & img)
{
    return vigra::triple<typename ROIImage<Image,Alpha>::mask_traverser,
	        typename ROIImage<Image,Alpha>::mask_traverser,
                typename ROIImage<Image,Alpha>::MaskAccessor>(img.maskUpperLeft(),
			       			          img.maskLowerRight(),	
                                                          img.maskAccessor());
}

template <class Image, class Alpha>
inline vigra::pair<typename ROIImage<Image,Alpha>::mask_traverser,
                   typename ROIImage<Image,Alpha>::MaskAccessor>
destMask(ROIImage<Image,Alpha> & img)
{
    return vigra::pair<typename ROIImage<Image,Alpha>::mask_traverser,
                typename ROIImage<Image,Alpha>::MaskAccessor>(img.maskUpperLeft(),
                                                          img.maskAccessor());
}

#if 0
template <class Image, class Mask, class Functor>
void inspectROIImages(std::vector<ROIImage<Image,Mask> *> imgs, Functor & f)
{
    int nImg = imgs.size();
    std::vector<typename Image::traverser> imgUL(nImg);
    std::vector<typename Mask::traverser> maskUL(nImg);
    std::vector<vigra::Rect2D> rois(nImg);
    for (unsigned i=0; i < nImg; i++) {
        imgs[i] = imgs->upperLeft();
        masks[i] = imgs->maskUpperLeft();
        rois[i] = imgs->boundingBox();
    }
}
#endif

/** function to inspect a variable number of images.
 *
 *  They do not need to overlap, and will be only evaluated inside their ROI
 *
 */
template <class ImgIter, class ImgAcc, class MaskIter, class MaskAcc, class Functor>
void inspectImagesIf(std::vector<ImgIter> imgs,
                     std::vector<MaskIter> masks,
                     std::vector<vigra::Rect2D> rois,
                     Functor & f)
{

    typedef typename ImgIter::value_type PixelType;
    typedef typename MaskIter::value_type MaskType;

    typedef typename ImgIter::row_iterator RowImgIter;
    typedef typename MaskIter::row_iterator RowMaskIter;

    int nImg = imgs.size();

    vigra_precondition(nImg > 1, "more than one image needed");

    // get maximum roi
    vigra::Rect2D maxRoi= rois[0];
    for (unsigned i=1; i < nImg; i++) {
        maxRoi |= rois[i];
    }
    // skip empty area on the top and right
    for (int k=0; k < nImg; k++) {
        imgs[k].first.x += maxRoi.left();
        imgs[k].first.y += maxRoi.top();
        masks[k].first.x += maxRoi.left();
        masks[k].first.y += maxRoi.top();
    }

    std::vector<RowImgIter> rowImgIter(nImg);
    std::vector<RowMaskIter> rowMaskIter(nImg);

    vigra::Point2D p;
    std::vector<PixelType> val(nImg);
    std::vector<MaskType> mval(nImg);
    for(p.y=maxRoi.top(); p.y < maxRoi.bottom(); ++p.y)
    {
        for (int k=0; k < nImg; k++) {
            rowImgIter[k] = imgs[k].first.rowIterator();
            rowMaskIter[k] = masks[k].first.rowIterator();
        }

        for (p.x=maxRoi.left(); p.x < maxRoi.right(); ++p.x) {
            for (int k=0; k < nImg; k++) {
                if (rois[k].contains(p)) {
                    val[k] = *(rowImgIter[k]);
                    mval[k] = *(rowMaskIter[k]);
                } else {
                    mval[k] = 0;
                }
                rowImgIter[k]++;
                rowMaskIter[k]++;
            }
            f(val,mval);
        }
        for (int k=0; k < nImg; k++) {
            ++(imgs[k].first.y);
            ++(masks[k].first.y);
        }
    }
}


/** algorithm to reduce a set of ROI images */
template<class ROIIMG, class DestIter, class DestAccessor,
         class MaskIter, class MaskAccessor, class FUNCTOR>
void reduceROIImages(std::vector<ROIIMG *> images,
                     vigra::triple<DestIter, DestIter, DestAccessor> dest,
                     vigra::pair<MaskIter, MaskAccessor> destMask,
                     FUNCTOR & reduce)
{
    typedef typename DestAccessor::value_type ImgType;
    typedef typename MaskAccessor::value_type MaskType;

    typedef typename
        vigra::NumericTraits<ImgType> Traits;
    typedef typename
        Traits::RealPromote RealImgType;

    typedef typename vigra_ext::LUTTraits<MaskType> MaskLUTTraits;

    unsigned int nImg = images.size();

    vigra::Diff2D size = dest.second - dest.first;

    // iterate over the whole image...
    // calculate something on the pixels in the overlapping images. 
    for (int y=0; y < size.y; y++) {
        for (int x=0; x < size.x; x++) {
            reduce.reset();
            MaskType maskRes=0;
            for (unsigned int i=0; i< nImg; i++) {
                MaskType a;
                a = images[i]->getMask(x,y);
                if (a) {
                    maskRes = vigra_ext::LUTTraits<MaskType>::max();
                    reduce(images[i]->operator()(x,y), a);
                }
            }
            dest.third.set(reduce(), dest.first, vigra::Diff2D(x,y));
            destMask.second.set(maskRes, destMask.first, vigra::Diff2D(x,y));
        }
    }
}


}

#endif // _ROIIMAGE_H

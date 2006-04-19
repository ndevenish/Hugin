// -*- c-basic-offset: 4 -*-
/** @file RemappedPanoImage.h
 *
 *  Contains functions to transform whole images.
 *  Can use PTools::Transform or PT::SpaceTransform for the calculations
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

#ifndef _REMAPPEDPANOIMAGE_H
#define _REMAPPEDPANOIMAGE_H

#include <fstream>
#include <algorithm>

#include <vigra/basicimage.hxx>
#include <vigra/impex.hxx>
#include <vigra_ext/impexalpha.hxx>
#include <vigra/flatmorphology.hxx>
#include <vigra/transformimage.hxx>
//#include <vigra/functorexpression.hxx>

#include <common/math.h>

#include <vigra_ext/Interpolators.h>
#include <vigra_ext/ROIImage.h>
#include <vigra_ext/utils.h>
#include <vigra_ext/VignettingCorrection.h>
#include <vigra_ext/ImageTransforms.h>
#include <vigra_ext/FunctorAccessor.h>

#include <PT/Panorama.h>
#include <PT/PanoToolsInterface.h>
//#include <PT/SpaceTransform.h>


namespace PT
{

template <class TRANSFORM>
void estimateImageAlpha(const SrcPanoImage & src,
                        const PanoramaOptions & dest,
                       TRANSFORM & transf,
                       vigra::Rect2D & imgRect,
                       vigra::BImage & alpha,
                       double & scale)
{
    FDiff2D ul,lr;
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
    vigra::Size2D destSz = dest.getSize() * scale;
    vigra::Rect2D destRect = dest.getROI() * scale;
    destRect = destRect & vigra::Rect2D(destSz);

    FDiff2D cropCenter;
    double radius2;
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
                sx = sx - cropCenter.x;
                sy = sy - cropCenter.y;
                if (sx*sx + sy*sy > radius2) {
                        valid = false;
                }
            } else if (!src.getCropRect().contains(vigra::Point2D(utils::roundi(sx), utils::roundi(sy))) ) {
                valid = false;
            }

            if (valid) {
                img(x,y) = 255;
                if ( ul.x > (x-1)/scale ) {
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
            } else {
                img(x,y) = 0;
            }
        }
    }

    // check if we have found some pixels..
    if ( ul.x == DBL_MAX || ul.y == DBL_MAX || lr.x == -DBL_MAX || lr.y == -DBL_MAX ) {
        // no valid pixel.. strange.. either there is no image here, or we have
        // overlooked some pixel.. to be on the safe side. remap the whole image here...
        imgRect = dest.getROI();
        alpha.resize(img.size().x, img.size().y, 0);
        initImage(img.upperLeft()+destRect.upperLeft(), 
                  img.upperLeft()+destRect.lowerRight(),
                  img.accessor(),255);
    } else {
        imgRect.setUpperLeft(vigra::Point2D(utils::roundi(ul.x), utils::roundi(ul.y)));
        imgRect.setLowerRight(vigra::Point2D(utils::roundi(lr.x), utils::roundi(lr.y)));

        // ensure that the roi is inside the destination rect
        imgRect = dest.getROI() & imgRect;
        DEBUG_DEBUG("bounding box: " << imgRect);

        alpha.resize(img.size());
        // dilate alpha image, to cover neighbouring pixels,
        // that may be valid in the full resolution image
        vigra::discDilation(vigra::srcImageRange(img),
                            vigra::destImage(alpha), 1);
    }

}

/** calculate the outline of the image
 *
 *  @param destSize  Size of the dest picture (panorama)
 *  @param srcSize   Size of source picture
 *  @param transf    Transformation from image to pano
 *  @param result    insert border point into result container
 *  @param ul        Upper Left corner of the image roi
 *  @param lr        Lower right corner of the image roi
 */
template <class TRANSFORM>
void estimateImageRect(const SrcPanoImage & src, const PanoramaOptions & dest,
                       TRANSFORM & transf, vigra::Rect2D & imgRect)
{
    vigra::BImage img;
    double scale;
    estimateImageAlpha(src, dest, transf, imgRect, img, scale);
}


/** struct to hold a image state for stitching
 *
 */
template <class RemapImage, class AlphaImage>
class RemappedPanoImage : public vigra_ext::ROIImage<RemapImage, AlphaImage>
{
public:
// typedefs for the children types
    typedef typename RemapImage::value_type      image_value_type;
    typedef typename RemapImage::traverser       image_traverser;
    typedef typename RemapImage::const_traverser const_image_traverser;
    typedef typename RemapImage::Accessor        ImageAccessor;
    typedef typename RemapImage::ConstAccessor   ConstImageAccessor;

    typedef typename AlphaImage::value_type       mask_value_type;
    typedef typename AlphaImage::traverser        mask_traverser;
    typedef typename AlphaImage::const_traverser  const_mask_traverser;
    typedef typename AlphaImage::Accessor         MaskAccessor;
    typedef typename AlphaImage::ConstAccessor    ConstMaskAccessor;

    typedef vigra_ext::ROIImage<RemapImage, AlphaImage> Base;

    /** create a remapped pano image
     *
     *  the actual remapping is done by the remapImage() function.
     */
    RemappedPanoImage()
    {
    }

    void setPanoImage(const SrcPanoImage & src,
                      const PanoramaOptions & dest)
    {
        m_srcImg = src;
        m_destImg = dest;
        m_transf.createTransform(src, dest);
        vigra::Rect2D imageRect;
        estimateImageRect(src, dest, m_transf, imageRect);

        // restrict to panorama size
        Base::resize(imageRect);
        DEBUG_DEBUG("after resize: " << Base::m_region);
    }

#if 0
    /** set a new image or panorama options
     *
     *  This is needed before any of the remap functions can be used.
     *
     *  calculates bounding box, and outline
     */
    void setPanoImage(const vigra::Size2D & srcSize,
                      const PT::VariableMap & srcVars,
                      PT::Lens::LensProjectionFormat srcProj,
                      const PT::PanoImage & img,
                      const vigra::Diff2D &destSize,
                      PT::PanoramaOptions::ProjectionFormat destProj,
                      double destHFOV)
    {
        m_srcSize = srcSize;
        m_srcOrigSize.x = img.getWidth();
        m_srcOrigSize.y = img.getHeight();
        m_srcProj = m_srcProj;


        m_srcPanoImg = img;
        m_destProj = destProj;
        m_destHFOV = destHFOV;
        // create transforms
        //    SpaceTransform t;
        //    SpaceTransform invT;
        /*
        m_invTransf.createInvTransform(srcSize, srcVars, srcProj,
                                       destSize, destProj, destHFOV,
                                       m_srcOrigSize);
        */
        // calculate ROI for this image.
        m_transf.createTransform(srcSize, srcVars, srcProj,
                                 destSize, destProj, destHFOV,
                                 m_srcOrigSize);

        ImageOptions imgOpts = img.getOptions();

        // todo: resize crop!
        bool circCrop = srcProj == Lens::CIRCULAR_FISHEYE;
        estimateImageRect(destSize, m_srcOrigSize,
                          imgOpts.docrop, imgOpts.cropRect, circCrop,
                          m_transf,
                          imageRect);

        m_warparound = (destProj == PanoramaOptions::EQUIRECTANGULAR && m_destHFOV == 360);


    }

    void setPanoImage(const PT::Panorama & pano, unsigned int imgNr,
                      vigra::Size2D srcSize, const PT::PanoramaOptions & opts)
    {
        const PT::PanoImage & img = pano.getImage(imgNr);

        m_srcSize = srcSize;
        m_srcOrigSize.x = img.getWidth();
        m_srcOrigSize.y = img.getHeight();
        m_srcProj = pano.getLens(pano.getImage(imgNr).getLensNr()).getProjection();

        m_destProj = opts.getProjection();
        m_destHFOV = opts.getHFOV();
        m_warparound = (opts.getProjection() == PanoramaOptions::EQUIRECTANGULAR && opts.getHFOV() == 360);

        // create transforms
        //    SpaceTransform t;
        //    SpaceTransform invT;

//        m_invTransf.createInvTransform(pano, imgNr, opts, m_srcSize);
        m_transf.createTransform(pano, imgNr, opts, m_srcSize);

        // calculate ROI for this image.
        m_srcPanoImg = pano.getImage(imgNr);
        ImageOptions imgOpts = pano.getImage(imgNr).getOptions();
        vigra::Rect2D imageRect;
        // todo: resize crop!
        bool circCrop = pano.getLens(pano.getImage(imgNr).getLensNr()).getProjection() == Lens::CIRCULAR_FISHEYE;
        estimateImageRect(vigra::Size2D(opts.getWidth(), opts.getHeight()), srcSize,
                          imgOpts.docrop, imgOpts.cropRect, circCrop,  
                          m_transf,
                          imageRect);


        // restrict to panorama size
        Base::resize(imageRect);
        DEBUG_DEBUG("after resize: " << Base::m_region);
    }
#endif

    /** calculate distance map. pixels contain distance from image center
     *
     *  setPanoImage() has to be called before!
     */
    template<class DistImgType>
    void calcSrcCoordImgs(DistImgType & imgX, DistImgType & imgY)
    {
        imgX.resize(Base::boundingBox().size());
        imgY.resize(Base::boundingBox().size());
        // calculate the alpha channel,
        int xstart = Base::boundingBox().left();
        int xend   = Base::boundingBox().right();
        int ystart = Base::boundingBox().top();
        int yend   = Base::boundingBox().bottom();

        // create dist y iterator
        typename DistImgType::Iterator yImgX(imgX.upperLeft());
        typename DistImgType::Iterator yImgY(imgY.upperLeft());
        typename DistImgType::Accessor accX = imgX.accessor();
        typename DistImgType::Accessor accY = imgY.accessor();
        // loop over the image and transform
        for(int y=ystart; y < yend; ++y, ++yImgX.y, ++yImgY.y)
        {
            // create x iterators
            typename DistImgType::Iterator xImgX(yImgX);
            typename DistImgType::Iterator xImgY(yImgY);
            for(int x=xstart; x < xend; ++x, ++xImgY.x, ++xImgX.x)
            {
                double sx,sy;
                m_transf.transformImgCoord(sx,sy,x,y);
                accX.set(sx, xImgX);
                accY.set(sy, xImgY);
            }
        }
    }

    /** calculate only the alpha channel.
     *  works for arbitrary transforms, with holes and so on,
     *  but is very crude and slow (remapps all image pixels...)
     *
     *  better transform all images, and get the alpha channel for free!
     *
     *  setPanoImage() should be called before.
     */
    void calcAlpha()
    {
        Base::m_mask.resize(Base::boundingBox().size());
        // calculate the alpha channel,
        int xstart = Base::boundingBox().left();
        int xend   = Base::boundingBox().right();
        int ystart = Base::boundingBox().top();
        int yend   = Base::boundingBox().bottom();

// DGSW FIXME - Unreferenced
//		int interpolHalfWidth=0;
        // create dist y iterator
        typename AlphaImage::Iterator yalpha(Base::m_mask.upperLeft());
        // loop over the image and transform
        for(int y=ystart; y < yend; ++y, ++yalpha.y)
        {
            // create x iterators
            typename AlphaImage::Iterator xalpha(yalpha);
            for(int x=xstart; x < xend; ++x, ++xalpha.x)
            {
                double sx,sy;
                m_transf.transformImgCoord(sx,sy,x,y);
                if (m_srcImg.isInside(vigra::Point2D(utils::roundi(sx),utils::roundi(sy)))) {
                    *xalpha = 255;
                } else {
                    *xalpha = 0;
                }
            }
        }
    }

    /** remap a image without alpha channel*/
    template <class ImgIter, class ImgAccessor>
    void remapImage(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                    vigra_ext::Interpolator interpol,
                    utils::MultiProgressDisplay & progress)
    {
        //        std::ostringstream msg;
        //        msg <<"remapping image "  << imgNr;
        //        progress.setMessage(msg.str().c_str());

        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;
        vigra_precondition(srcImgSize == m_srcImg.getSize(), 
                           "RemappedPanoImage::remapImage(): image sizes not consistent");


        if (m_srcImg.getCropMode() != SrcPanoImage::NO_CROP) {
            // need to create and additional alpha image for the crop mask...
            // not very efficient during the remapping phase, but works.
            vigra::BImage alpha(srcImgSize.x, srcImgSize.y);

            vigra::Rect2D cR = m_srcImg.getCropRect();
            switch (m_srcImg.getCropMode()) {
            case SrcPanoImage::CROP_CIRCLE:
                {
                    FDiff2D m( (cR.left() + cR.width()/2.0),
                            (cR.top() + cR.height()/2.0) );

                    double radius = std::min(cR.width(), cR.height())/2.0;
                    initImage(vigra::destImageRange(alpha),255);
                    vigra_ext::circularCrop(vigra::destImageRange(alpha), m, radius);
                    break;
                }
            case SrcPanoImage::CROP_RECTANGLE:
                {
                    initImage(vigra::destImageRange(alpha),0);
                    // make sure crop is inside the image..
                    cR &= vigra::Rect2D(0,0, srcImgSize.x, srcImgSize.y);
                    initImage(alpha.upperLeft()+cR.upperLeft(), 
                              alpha.upperLeft()+cR.lowerRight(),
                              alpha.accessor(),255);
                    break;
                }
            default:
                break;
            }
            if (m_srcImg.getGamma() != 1.0) {
                // do gamma correction on the fly
                double gMaxVal = vigra_ext::VigCorrTraits<image_value_type>::max();
                vigra_ext::GammaFunctor gf(1/m_srcImg.getGamma(), gMaxVal);
                vigra_ext::WriteFunctorAccessor<vigra_ext::GammaFunctor, typename Base::ImageAccessor> wfa(gf, Base::m_image.accessor());
                transformImageAlpha(srcImg,
                                    vigra::srcImage(alpha),
                                    vigra::destIterRange(Base::m_image.upperLeft(),
                                                    Base::m_image.lowerRight(),
                                                    wfa),
                                    destImage(Base::m_mask),
                                    Base::boundingBox().upperLeft(),
                                    m_transf,
                                    m_srcImg.horizontalWarpNeeded(),
                                    interpol,
                                    progress);

            } else {
                transformImageAlpha(srcImg,
                                    vigra::srcImage(alpha),
                                    destImageRange(Base::m_image),
                                    destImage(Base::m_mask),
                                    Base::boundingBox().upperLeft(),
                                    m_transf,
                                    m_srcImg.horizontalWarpNeeded(),
                                    interpol,
                                    progress);
            }
        } else {
            if (m_srcImg.getGamma() != 1.0) {
                // do gamma correction on the fly
                double gMaxVal = vigra_ext::VigCorrTraits<image_value_type>::max();
                vigra_ext::GammaFunctor gf(1/m_srcImg.getGamma(), gMaxVal);
                vigra_ext::WriteFunctorAccessor<vigra_ext::GammaFunctor, typename Base::ImageAccessor> wfa(gf, Base::m_image.accessor());
                transformImage(srcImg,
                            vigra::destIterRange(Base::m_image.upperLeft(),
                                            Base::m_image.lowerRight(),
                                            wfa),
                            destImage(Base::m_mask),
                            Base::boundingBox().upperLeft(),
                            m_transf,
                            m_srcImg.horizontalWarpNeeded(),
                            interpol,
                            progress);
            } else {
                transformImage(srcImg,
                            destImageRange(Base::m_image),
                            destImage(Base::m_mask),
                            Base::boundingBox().upperLeft(),
                            m_transf,
                            m_srcImg.horizontalWarpNeeded(),
                            interpol,
                            progress);
            }
        }
    }


    /** remap a image, with alpha channel */
    template <class ImgIter, class ImgAccessor,
              class AlphaIter, class AlphaAccessor>
    void remapImage(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                    std::pair<AlphaIter, AlphaAccessor> alphaImg,
                    vigra_ext::Interpolator interp,
                    utils::MultiProgressDisplay & progress)
    {

        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;

        vigra_precondition(srcImgSize == m_srcImg.getSize(), 
                           "RemappedPanoImage::remapImage(): image sizes not consistent");

        if (m_srcImg.getCropMode() != SrcPanoImage::NO_CROP) {
            vigra::BImage alpha(srcImgSize);
            vigra::copyImage(vigra::make_triple(alphaImg.first, alphaImg.first + srcImgSize, alphaImg.second),
                             vigra::destImage(alpha));

            vigra::Rect2D cR = m_srcImg.getCropRect();
            switch (m_srcImg.getCropMode()) {
                case SrcPanoImage::CROP_CIRCLE:
                {
                    FDiff2D m( (cR.left() + cR.width()/2.0),
                                (cR.top() + cR.height()/2.0) );

                    double radius = std::min(cR.width(), cR.height())/2.0;
                    vigra_ext::circularCrop(vigra::destImageRange(alpha), m, radius);
                    break;
                }
                case SrcPanoImage::CROP_RECTANGLE:
                {
                    cR &= vigra::Rect2D(0,0, srcImgSize.x, srcImgSize.y);
                    initImageIf(alpha.upperLeft()+cR.upperLeft(), 
                                alpha.upperLeft()+cR.lowerRight(),
                                alpha.accessor(),
                                alpha.upperLeft()+cR.upperLeft(), 
                                alpha.accessor(), 255);
                    break;
                }
                default:
                    break;
            }
            if (m_srcImg.getGamma() != 1.0) {
                // do gamma correction on the fly
                double gMaxVal = vigra_ext::VigCorrTraits<image_value_type>::max();
                vigra_ext::GammaFunctor gf(1/m_srcImg.getGamma(), gMaxVal);
                vigra_ext::WriteFunctorAccessor<vigra_ext::GammaFunctor, typename Base::ImageAccessor> wfa(gf, Base::m_image.accessor());

                vigra_ext::transformImageAlpha(srcImg,
                                                vigra::srcImage(alpha),
                                                vigra::destIterRange(Base::m_image.upperLeft(),
                                                               Base::m_image.lowerRight(),
                                                               wfa),
                                                destImage(Base::m_mask),
                                                Base::boundingBox().upperLeft(),
                                                m_transf,
                                                m_srcImg.horizontalWarpNeeded(),
                                                interp,
                                                progress);
            } else {
                vigra_ext::transformImageAlpha(srcImg,
                                                vigra::srcImage(alpha),
                                                destImageRange(Base::m_image),
                                                destImage(Base::m_mask),
                                                Base::boundingBox().upperLeft(),
                                                m_transf,
                                                m_srcImg.horizontalWarpNeeded(),
                                                interp,
                                                progress);
            }

        } else {
            if (m_srcImg.getGamma() != 1.0) {
                // do gamma correction on the fly
                double gMaxVal = vigra_ext::VigCorrTraits<image_value_type>::max();
                vigra_ext::GammaFunctor gf(1/m_srcImg.getGamma(), gMaxVal);
                vigra_ext::WriteFunctorAccessor<vigra_ext::GammaFunctor, typename Base::ImageAccessor> wfa(gf, Base::m_image.accessor());
                vigra_ext::transformImageAlpha(srcImg,
                            alphaImg,
                            vigra::destIterRange(Base::m_image.upperLeft(),
                                            Base::m_image.lowerRight(),
                                            wfa),
                            destImage(Base::m_mask),
                            Base::boundingBox().upperLeft(),
                            m_transf,
                            m_srcImg.horizontalWarpNeeded(),
                            interp,
                            progress);
            } else {
                vigra_ext::transformImageAlpha(srcImg,
                            alphaImg,
                            destImageRange(Base::m_image),
                            destImage(Base::m_mask),
                            Base::boundingBox().upperLeft(),
                            m_transf,
                            m_srcImg.horizontalWarpNeeded(),
                            interp,
                            progress);
            }
        }

    }

//    const std::vector<FDiff2D> & getOutline()
//    {
//        return m_outline;
//    }

    vigra::ICCProfile m_ICCProfile;

protected:
    SrcPanoImage m_srcImg;
    PanoramaOptions m_destImg;
    PTools::Transform m_transf;
//    PT::SpaceTransform m_transf;
};

/** remap a single image
 *
 *  Be careful, might modify srcImg (vignetting and brightness correction)
 *
 */
template <class SrcImgType, class FlatImgType, class DestImgType, class MaskImgType>
void remapImage(SrcImgType & srcImg,
                const MaskImgType & srcAlpha,
                const FlatImgType & srcFlat,
                const PT::SrcPanoImage & src,
                const PT::PanoramaOptions & dest,
//                vigra_ext::Interpolator interpolator,
                RemappedPanoImage<DestImgType, MaskImgType> & remapped,
                utils::MultiProgressDisplay & progress)
{
    typedef typename SrcImgType::value_type SrcPixelType;
    typedef typename DestImgType::value_type DestPixelType;

    typedef typename vigra::NumericTraits<SrcPixelType>::RealPromote RSrcPixelType;

    // prepare some information required by multiple types of vignetting correction
    bool vigCorrDivision = (src.getVigCorrMode() & PT::SrcPanoImage::VIGCORR_DIV)>0;

    RSrcPixelType ka,kb;
    bool doBrightnessConversion = convertKParams(src.getBrightnessFactor(),
                                                 src.getBrightnessOffset(),
                                                 ka, kb);
    bool dither = ditheringNeeded(SrcPixelType());

    double gMaxVal = vigra_ext::VigCorrTraits<typename DestImgType::value_type>::max();

    if (src.getVigCorrMode() & SrcPanoImage::VIGCORR_FLATFIELD) {
        vigra_ext::flatfieldVigCorrection(vigra::srcImageRange(srcImg),
                                          vigra::srcImage(srcFlat), 
                                          vigra::destImage(srcImg), src.getGamma(), gMaxVal,
                                          vigCorrDivision, ka, kb, dither);
    } else if (src.getVigCorrMode() & SrcPanoImage::VIGCORR_RADIAL) {
        progress.setMessage(std::string("radial vignetting correction ") + utils::stripPath(src.getFilename()));

        vigra_ext::radialVigCorrection(srcImageRange(srcImg), destImage(srcImg),
                                       src.getGamma(), gMaxVal,
                                       src.getRadialVigCorrCoeff(), 
                                       src.getRadialVigCorrCenter(),
                                       vigCorrDivision, ka, kb, dither);
    } else if (src.getGamma() != 1.0 && doBrightnessConversion ) {
        progress.setMessage(std::string("inverse gamma & brightness corr") + utils::stripPath(src.getFilename()));
        vigra_ext::applyGammaAndBrightCorrection(srcImageRange(srcImg), destImage(srcImg),
                src.getGamma(), gMaxVal, ka,kb);
    } else if (doBrightnessConversion ) {
        progress.setMessage(std::string("brightness correction ") + utils::stripPath(src.getFilename()));
        vigra_ext::applyBrightnessCorrection(srcImageRange(srcImg), destImage(srcImg),
                                             ka,kb);
    } else if (src.getGamma() != 1.0 ) {
        progress.setMessage(std::string("inverse gamma correction ") + utils::stripPath(src.getFilename()));
        vigra_ext::applyGammaCorrection(srcImageRange(srcImg), destImage(srcImg),
                                        src.getGamma(), gMaxVal);
    }

    progress.setMessage(std::string("remapping ") + utils::stripPath(src.getFilename()));
    remapped.setPanoImage(src, dest);
    if (srcAlpha.size().x > 0) {
        remapped.remapImage(vigra::srcImageRange(srcImg),
                            vigra::srcImage(srcAlpha), dest.interpolator,
                            progress);
    } else {
        remapped.remapImage(vigra::srcImageRange(srcImg), dest.interpolator, progress);
    }
//        progress.setMessage(std::string("gamma correction ") + utils::stripPath(src.getFilename()));
//        vigra_ext::applyGammaCorrection(srcImageRange(remapped.m_image),
//                                        destImage(remapped.m_image),
//                                        1/src.getGamma(), gMaxVal);
//    }
}


/** functor to create a remapped image */
template <typename ImageType, typename AlphaType>
class SingleImageRemapper
{
public:

    /** create a remapped pano image.
     *
     *  The image ownership is transferred to the caller.
     */
    virtual
    RemappedPanoImage<ImageType, AlphaType> *
    getRemapped(const Panorama & pano,
                const PanoramaOptions & opts,
                unsigned int imgNr, utils::MultiProgressDisplay & progress) = 0;

    virtual	void
    release(RemappedPanoImage<ImageType,AlphaType> * d) = 0;
    virtual ~SingleImageRemapper() {};
};

/// load a flatfield image and apply the correction
template <class FFType, class SrcIter, class SrcAccessor, class DestIter, class DestAccessor>
void applyFlatfield(vigra::triple<SrcIter, SrcIter, SrcAccessor> srcImg,
                    vigra::pair<DestIter, DestAccessor> destImg,
                    vigra::ImageImportInfo & ffInfo,
                    double gamma,
                    double gammaMaxVal,
                    bool division,
                    typename vigra::NumericTraits<typename SrcAccessor::value_type>::RealPromote a,
                    typename vigra::NumericTraits<typename SrcAccessor::value_type>::RealPromote b,
                    bool dither)
{
    FFType ffImg(ffInfo.width(), ffInfo.height());
    vigra::importImage(ffInfo, vigra::destImage(ffImg));
    vigra_ext::flatfieldVigCorrection(srcImg, vigra::srcImage(ffImg), 
                                      destImg, gamma, gammaMaxVal, division, a, b, dither);
}

// 3 channel images
template <class T>
bool convertKParams(const std::vector<double> & ka, const std::vector<double> & kb,
                    T & a,
                    T & b,
                    vigra::VigraFalseType)
{
    DEBUG_ASSERT(a.size() == 3);
    bool ret(false);
    for (unsigned int i=0; i< 3; i++) {
        a[i] = ka[i];
        b[i] = kb[i];
        ret = ret || ( a[i] != 1.0 || b[i] != 0.0);
    }

    return ret;
}

// singe channel images
template <class T>
bool convertKParams(const std::vector<double> & ka, const std::vector<double> & kb,
                    T & a,
                    T & b,
                    vigra::VigraTrueType)
{
    a = ka[0];
    b = kb[0];
    return (a != 1.0 || b != 0.0);
}

// get k coefficents, and return if brightness correction needs to be done.
template <class T>
bool convertKParams(const std::vector<double> & ka, const std::vector<double> & kb,
                    T & a,
                    T & b)
{
    typedef typename vigra::NumericTraits<T>::isScalar is_scalar;
    return convertKParams(ka, kb, a, b, is_scalar());
}


/** functor to create a remapped image, loads image from disk */
template <typename ImageType, typename AlphaType>
class FileRemapper : public SingleImageRemapper<ImageType, AlphaType>
{

private:
public:

    FileRemapper()
    {
        m_remapped = 0;
    }

    virtual ~FileRemapper() {};

    virtual RemappedPanoImage<ImageType, AlphaType> *
    getRemapped(const Panorama & pano, const PanoramaOptions & opts,
                unsigned int imgNr, utils::MultiProgressDisplay & progress)
    {
//        typedef typename ImageType::value_type PixelType;

        //typedef typename vigra::NumericTraits<PixelType>::RealPromote RPixelType;
//        typedef typename vigra::BasicImage<RPixelType> RImportImageType;
        typedef typename vigra::BasicImage<float> FlatImgType;

        FlatImgType ffImg;
        AlphaType srcAlpha;

        // choose image type...
        const PT::PanoImage & img = pano.getImage(imgNr);
        const ImageOptions iopts = img.getOptions();

        vigra::Size2D origSrcSize(img.getWidth(), img.getHeight());
// DGSW FIXME - Unreferenced
//		const PT::VariableMap & srcVars = pano.getImageVariables(imgNr);
//		const Lens & lens = pano.getLens(img.getLensNr());

        vigra::Size2D destSize(opts.getWidth(), opts.getHeight());

        m_remapped = new RemappedPanoImage<ImageType, AlphaType>;

        // load image

        vigra::ImageImportInfo info(img.getFilename().c_str());
        ImageType srcImg(info.width(), info.height());

        m_remapped->m_ICCProfile = info.getICCProfile();
        // import the image
        progress.setMessage(std::string("loading ") + utils::stripPath(img.getFilename()));
        if (info.numExtraBands() > 0) {
            // process with mask
            srcAlpha.resize(info.width(), info.height());
            // import with alpha channel
            vigra::importImageAlpha(info, vigra::destImage(srcImg),
                                    vigra::destImage(srcAlpha));
        } else {
            // process without mask
            // import without alpha channel
            vigra::importImage(info, vigra::destImage(srcImg));
        }

        // load flatfield, if needed.
        if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
            // load flatfield image.
            vigra::ImageImportInfo ffInfo(iopts.m_flatfield.c_str());
            progress.setMessage(std::string("flatfield vignetting correction ") + utils::stripPath(img.getFilename()));
            vigra_precondition(( ffInfo.numBands() == 1),
                                    "flatfield vignetting correction: "
                                            "Only single channel flatfield images are supported\n");
            ffImg.resize(ffInfo.width(), ffInfo.height());
            vigra::importImage(ffInfo, vigra::destImage(ffImg));
        }
        // remap the image

        remapImage(srcImg, srcAlpha, ffImg,
                   pano.getSrcImage(imgNr), opts,
                   *m_remapped,
                   progress);
        return m_remapped;
    }

#if 0
    /** create a remapped pano image.
     *
     *  load the file from disk, and remap in memory
     *  
     *  the image needs to be deallocated with a call to release()
     */
    virtual RemappedPanoImage<ImageType, AlphaType> *
    getRemapped(const Panorama & pano, const PanoramaOptions & opts,
                unsigned int imgNr, utils::MultiProgressDisplay & progress)
    {
        typedef typename ImageType::value_type PixelType;
        typedef typename vigra::NumericTraits<PixelType>::RealPromote RealPixelType;

        // choose image type...
        const PT::PanoImage & img = pano.getImage(imgNr);
        const ImageOptions iopts = img.getOptions();
        if (opts.gamma != 1.0 || iopts.m_vigCorrMode != 0) {
            // load image

            typedef typename ImageType::value_type PixelType;
            typedef typename vigra::NumericTraits<PixelType>::RealPromote RPixelType;
            typedef typename vigra::BasicImage<RPixelType> ImportImageType;
            vigra::ImageImportInfo info(img.getFilename().c_str());
            ImportImageType srcImg(info.width(), info.height());

            AlphaType srcAlpha;

            // import the image
            progress.setMessage(std::string("loading ") + utils::stripPath(img.getFilename()));
            if (info.numExtraBands() > 0) {
                // process with mask
                srcAlpha.resize(info.width(), info.height());
                // import with alpha channel
                vigra::importImageAlpha(info, vigra::destImage(srcImg),
                                        vigra::destImage(srcAlpha));
            } else {
                // process without mask
                // import without alpha channel
                vigra::importImage(info, vigra::destImage(srcImg));
            }

            // load flatfield, if needed.
            if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
             // load flatfield image.
                vigra::ImageImportInfo ffInfo(iopts.m_flatfield.c_str());
                progress.setMessage(std::string("flatfield vignetting correction ") + utils::stripPath(img.getFilename()));
                vigra::ImageImportInfo ffInfo(iopts.m_flatfield.c_str());
                vigra_precondition(( ffInfo.numBands() == 1),
                                     "flatfield vignetting correction: "
                                     "Only single channel flatfield images are supported\n");

                if (strcmp(ffInfo.getPixelType(), "UINT8") == 0 ) {
                    remapWithFlat<vigra::BImage>(vigra::srcImageRange(srcImg), ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
                } else if (strcmp(ffInfo.getPixelType(), "INT16") == 0 ) {
                    applyFlatfield<vigra::SImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                            ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "UINT16") == 0 ) {
                applyFlatfield<vigra::USImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                        ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "UINT32") == 0 ) {
                applyFlatfield<vigra::IImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                        ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "INT32") == 0 ) {
                applyFlatfield<vigra::UIImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                        ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "FLOAT") == 0 ) {
                applyFlatfield<vigra::FImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                        ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "DOUBLE") == 0 ) {
                applyFlatfield<vigra::DImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                        ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else {
                DEBUG_FATAL("Unsupported pixel type: " << ffInfo.getPixelType());
                vigra_fail("flatfield vignetting correction: unsupported pixel type");
            }

            void remapImage(srcImg, srcAlpha, srcFlat,
                            const vigra::Diff2D & origSrcSize,
                            const PT::VariableMap & srcVars,
                            PT::Lens::LensProjectionFormat srcProj,
                            double srcGamma,
                            const PT::PanoImage & img,
                            const vigra::Diff2D &destSize,
                            PT::PanoramaOptions::ProjectionFormat destProj,
                            double destHFOV,
                            RemappedPanoImage<DestImgType, MaskImgType> & remapped,
                            utils::MultiProgressDisplay & progress)
            {

            return remapImage(pano, opts, imgNr, progress);
        } else {
            return getRemappedIntern<PixelType>(pano, opts, imgNr, progress);
        }
    }

    /** further specialisation to be able to process integer images without rounding problems */
    template <class PixelType>
    RemappedPanoImage<ImageType, AlphaType> *
    getRemappedIntern(const Panorama & pano, const PanoramaOptions & opts,
                      unsigned int imgNr, utils::MultiProgressDisplay & progress)
    {
        DEBUG_TRACE("Image: " << imgNr);

        typedef typename ImageType::value_type SrcPixelType;

        typedef vigra::BasicImage<PixelType> ImportImageType;

        typedef typename ImportImageType::traverser sI;
        typedef typename ImportImageType::Accessor sA;
        typedef typename ImportImageType::const_traverser csI;
        typedef typename ImportImageType::ConstAccessor csA;

        typedef typename vigra::NumericTraits<PixelType>::RealPromote RPixelType;
        
        // load image
        const PT::PanoImage & img = pano.getImage(imgNr);
        const ImageOptions iopts = img.getOptions();

        vigra::ImageImportInfo info(img.getFilename().c_str());

        m_remapped = new RemappedPanoImage<ImageType, AlphaType>;

        // create an image of the right size
        ImportImageType srcImg(info.width(), info.height());

        AlphaType srcAlpha;

        // import the image
        progress.setMessage(std::string("loading ") + utils::stripPath(img.getFilename()));
        if (info.numExtraBands() > 0) {
            // process with mask
            srcAlpha.resize(info.width(), info.height());
            // import with alpha channel
            vigra::importImageAlpha(info, vigra::destImage(srcImg),
                                    vigra::destImage(srcAlpha));
        } else {
            // process without mask
            // import without alpha channel
            vigra::importImage(info, vigra::destImage(srcImg));
        }

        // prepare some information required by multiple types of vignetting correction
        bool vigCorrDivision = (iopts.m_vigCorrMode & ImageOptions::VIGCORR_DIV)>0;

        RPixelType ka,kb;
        bool doBrightnessConversion = convertKParams(pano.getImageVariables(imgNr), ka, kb);

        bool dither = ditheringNeeded(SrcPixelType());

        double gMaxVal = vigra_ext::VigCorrTraits<typename ImageType::value_type>::max();
        if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
             // load flatfield image.

             progress.setMessage(std::string("flatfield vignetting correction ") + utils::stripPath(img.getFilename()));
             vigra::ImageImportInfo ffInfo(iopts.m_flatfield.c_str());
             vigra_precondition(( ffInfo.numBands() == 1),
                                 "flatfield vignetting correction: "
                                 "Only single channel flatfield images are supported\n");

             if (strcmp(ffInfo.getPixelType(), "UINT8") == 0 ) {
                 applyFlatfield<vigra::BImage, csI, csA, sI, sA>(vigra::srcImageRange(srcImg),
                                vigra::destImage(srcImg), ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "INT16") == 0 ) {
                applyFlatfield<vigra::SImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "UINT16") == 0 ) {
                 applyFlatfield<vigra::USImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "UINT32") == 0 ) {
                 applyFlatfield<vigra::IImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "INT32") == 0 ) {
                 applyFlatfield<vigra::UIImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "FLOAT") == 0 ) {
                 applyFlatfield<vigra::FImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else if (strcmp(ffInfo.getPixelType(), "DOUBLE") == 0 ) {
                 applyFlatfield<vigra::DImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb, dither);
            } else {
                 DEBUG_FATAL("Unsupported pixel type: " << ffInfo.getPixelType());
                 vigra_fail("flatfield vignetting correction: unsupported pixel type");
            }
        } else if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_RADIAL) {
            progress.setMessage(std::string("radial vignetting correction ") + utils::stripPath(img.getFilename()));
            double radCoeff[4];
            radCoeff[0] = const_map_get(pano.getImageVariables(imgNr),"Va").getValue();
            radCoeff[1] = const_map_get(pano.getImageVariables(imgNr),"Vb").getValue();
            radCoeff[2] = const_map_get(pano.getImageVariables(imgNr),"Vc").getValue();
            radCoeff[3] = const_map_get(pano.getImageVariables(imgNr),"Vd").getValue();

            double scale = (double) srcImg.width() / img.getWidth();

            double centerShiftX = const_map_get(pano.getImageVariables(imgNr),"Vx").getValue();
            double centerShiftY = const_map_get(pano.getImageVariables(imgNr),"Vy").getValue();
            // take scale factor into accout..
            double cx = (img.getWidth()/2 + centerShiftX) * scale;
            double cy = (img.getHeight()/2 + centerShiftY) * scale;

            vigra_ext::radialVigCorrection(srcImageRange(srcImg), destImage(srcImg),
                                           opts.gamma, gMaxVal,
                                           radCoeff, cx, cy,
                                           vigCorrDivision, ka, kb, dither);
        } else if (opts.gamma != 1.0 && doBrightnessConversion ) {
            progress.setMessage(std::string("inverse gamma & brightness corr") + utils::stripPath(img.getFilename()));
            vigra_ext::applyGammaAndBrightCorrection(srcImageRange(srcImg), destImage(srcImg),
                                                     opts.gamma, gMaxVal, ka,kb);
        } else if (doBrightnessConversion ) {
            progress.setMessage(std::string("brightness correction ") + utils::stripPath(img.getFilename()));
            vigra_ext::applyBrightnessCorrection(srcImageRange(srcImg), destImage(srcImg),
                                                 ka,kb);
        } else if (opts.gamma != 1.0 ) {
            progress.setMessage(std::string("inverse gamma correction ") + utils::stripPath(img.getFilename()));
            vigra_ext::applyGammaCorrection(srcImageRange(srcImg), destImage(srcImg),
                                            opts.gamma, gMaxVal);
        }

        DEBUG_TRACE("starting remap of image: " << imgNr);
        progress.setMessage(std::string("remapping ") + utils::stripPath(img.getFilename()));
        if (info.numExtraBands() > 0) {
            m_remapped->remapImage(pano, opts,
                                   vigra::srcImageRange(srcImg),
                                   vigra::srcImage(srcAlpha),
                                   imgNr, progress);
        } else {
            m_remapped->remapImage(pano, opts,
                                   vigra::srcImageRange(srcImg),
                                   imgNr, progress);
        }
        if (opts.gamma != 1.0) {
            progress.setMessage(std::string("gamma correction ") + utils::stripPath(img.getFilename()));
            vigra_ext::applyGammaCorrection(srcImageRange(m_remapped->m_image), destImage(m_remapped->m_image),
                                            1/opts.gamma, gMaxVal);
        }

        return m_remapped;
    }
#endif

    virtual void release(RemappedPanoImage<ImageType,AlphaType> * d)
    {
        delete d;
    }

protected:
	RemappedPanoImage<ImageType,AlphaType> * m_remapped;
};



}; // namespace

#endif

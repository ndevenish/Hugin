// -*- c-basic-offset: 4 -*-
/** @file RemappedPanoImage.h
 *
 *  Contains functions to transform whole images.
 *  Can use PTools::Transform or PT::SpaceTransform for the calculations
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: RemappedPanoImage.h 1987 2007-05-08 22:50:14Z dangelo $
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
#include <vigra/functorexpression.hxx>

#include <common/math.h>

#include <vigra_ext/Interpolators.h>
#include <vigra_ext/ROIImage.h>
#include <vigra_ext/utils.h>
#include <vigra_ext/VignettingCorrection.h>
#include <vigra_ext/ImageTransforms.h>
#include <vigra_ext/FunctorAccessor.h>
#include <vigra_ext/lut.h>

#include <PT/Panorama.h>
#include <PT/PanoToolsInterface.h>
//#include <PT/SpaceTransform.h>

#ifdef DEBUG
#define DEBUG_REMAP 1
#endif

#ifdef DEBUG_REMAP
#ifdef WIN32
#define DEBUG_FILE_PREFIX "C:/temp/"
#else
#define DEBUG_FILE_PREFIX "/tmp/"
#endif
#endif

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
    vigra::Size2D destSz;
    destSz.x = utils::ceili(dest.getSize().x * scale);
    destSz.y = utils::ceili(dest.getSize().y * scale);
    vigra::Rect2D destRect;
    destRect.setUpperLeft(vigra::Point2D (utils::floori(dest.getROI().left() * scale),
                          utils::floori(dest.getROI().top() * scale)));
    destRect.setLowerRight(vigra::Point2D (utils::ceili(dest.getROI().right() * scale),
                   utils::ceili(dest.getROI().bottom() * scale)));
    destRect = destRect & vigra::Rect2D(destSz);

    DEBUG_DEBUG("scale " << scale);
    DEBUG_DEBUG("dest roi " << dest.getROI());
    DEBUG_DEBUG("dest Sz: " << destSz);
    DEBUG_DEBUG("dest rect: " << destRect);

    FDiff2D cropCenter;
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
    /*
#ifdef DEBUG
{
    vigra::ImageExportInfo exinfo( DEBUG_FILE_PREFIX "mask.png");
    vigra::exportImage(srcImageRange(img), exinfo);
}
{
    vigra::ImageExportInfo exinfo( DEBUG_FILE_PREFIX "mask_dilated.png");
    vigra::exportImage(srcImageRange(alpha), exinfo);
}
#endif
    */
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
        // no valid pixel.. strange.. either there is no image here, or we have
        // overlooked some pixel.. to be on the safe side. remap the whole image here...
        imgRect = dest.getROI();
        alpha.resize(img.size().x, img.size().y, 0);
        initImage(img.upperLeft()+destRect.upperLeft(), 
                  img.upperLeft()+destRect.lowerRight(),
                  img.accessor(),255);
    } else {
        // bounding rect after scan
        DEBUG_DEBUG("ul: " << ul << "  lr: " << lr);
        ul.x = (ul.x)/scale;
        ul.y = (ul.y)/scale;
        lr.x = (lr.x+1)/scale;
        lr.y = (lr.y+1)/scale;
        imgRect.setUpperLeft(vigra::Point2D(utils::roundi(ul.x), utils::roundi(ul.y)));
        imgRect.setLowerRight(vigra::Point2D(utils::roundi(lr.x), utils::roundi(lr.y)));
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

    typedef typename vigra_ext::ValueTypeTraits<image_value_type>::value_type component_type;
    
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

        typedef typename ImgAccessor::value_type input_value_type;
        typedef typename vigra_ext::ValueTypeTraits<input_value_type>::value_type input_component_type;

        // setup photometric transform for this image type
        // this corrects for response curve, white balance, exposure and 
        // radial vignetting
        vigra_ext::InvResponseTransform<input_component_type, double> invResponse(m_srcImg);
        if (m_destImg.outputMode == PanoramaOptions::OUTPUT_LDR) {
            // select exposure and response curve for LDR output
            // TODO: use a the same response curve for all output images.
            std::vector<double> outLut;
            vigra_ext::EMoR::createEMoRLUT(m_destImg.outputEMoRParams, outLut);
            invResponse.setOutput(1.0/pow(2.0,m_destImg.outputExposureValue), outLut,
                                  vigra_ext::LUTTraits<input_value_type>::max());
        } else {
            invResponse.setHDROutput();
        }


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


            transformImageAlpha(srcImg,
                                vigra::srcImage(alpha),
                                destImageRange(Base::m_image),
                                destImage(Base::m_mask),
                                Base::boundingBox().upperLeft(),
                                m_transf,
                                invResponse,
                                m_srcImg.horizontalWarpNeeded(),
                                interpol,
                                progress);
        } else {
            transformImage(srcImg,
                        destImageRange(Base::m_image),
                        destImage(Base::m_mask),
                        Base::boundingBox().upperLeft(),
                        m_transf,
                        invResponse,
                        m_srcImg.horizontalWarpNeeded(),
                        interpol,
                        progress);
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

        typedef typename ImgAccessor::value_type input_value_type;
        typedef typename vigra_ext::ValueTypeTraits<input_value_type>::value_type input_component_type;

        // setup photometric transform for this image type
        // this corrects for response curve, white balance, exposure and 
        // radial vignetting
        vigra_ext::InvResponseTransform<input_component_type, double> invResponse(m_srcImg);
        if (m_destImg.outputMode == PanoramaOptions::OUTPUT_LDR) {
            // select exposure and response curve for LDR output
            // TODO: use a the same response curve for all output images.
            std::vector<double> outLut;
            vigra_ext::EMoR::createEMoRLUT(m_destImg.outputEMoRParams, outLut);
            invResponse.setOutput(1.0/pow(2.0,m_destImg.outputExposureValue), outLut,
                                  vigra_ext::LUTTraits<input_value_type>::max());
        } else {
            invResponse.setHDROutput();
        }


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
            vigra_ext::transformImageAlpha(srcImg,
                                            vigra::srcImage(alpha),
                                            destImageRange(Base::m_image),
                                            destImage(Base::m_mask),
                                            Base::boundingBox().upperLeft(),
                                            m_transf,
                                            invResponse,
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
                                           invResponse,
                                           m_srcImg.horizontalWarpNeeded(),
                                           interp,
                                           progress);
        }
    }

//    const std::vector<FDiff2D> & getOutline()
//    {
//        return m_outline;
//    }

    vigra::ImageImportInfo::ICCProfile m_ICCProfile;

protected:
    SrcPanoImage m_srcImg;
    PanoramaOptions m_destImg;
    PTools::Transform m_transf;
//    PT::SpaceTransform m_transf;
};


/** remap a single image
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

#ifdef DEBUG_REMAP
{
    vigra::ImageExportInfo exi( DEBUG_FILE_PREFIX "hugin03_BeforeRemap.tif");
            vigra::exportImage(vigra::srcImageRange(srcImg), exi);
}
{
    if (srcAlpha.width() > 0) {
        vigra::ImageExportInfo exi(DEBUG_FILE_PREFIX "hugin04_BeforeRemapAlpha.tif");
                vigra::exportImage(vigra::srcImageRange(srcAlpha), exi);
    }
}
#endif

    progress.setMessage(std::string("remapping ") + utils::stripPath(src.getFilename()));
    remapped.setPanoImage(src, dest);
    // TODO: add provide support for flatfield images.
    if (srcAlpha.size().x > 0) {
        remapped.remapImage(vigra::srcImageRange(srcImg),
                            vigra::srcImage(srcAlpha), dest.interpolator,
                            progress);
    } else {
        remapped.remapImage(vigra::srcImageRange(srcImg), dest.interpolator, progress);
    }

#ifdef DEBUG_REMAP
{
    vigra::ImageExportInfo exi( DEBUG_FILE_PREFIX "hugin04_AfterRemap.tif"); 
            vigra::exportImage(vigra::srcImageRange(remapped.m_image), exi); 
}
{
    vigra::ImageExportInfo exi(DEBUG_FILE_PREFIX "hugin04_AfterRemapAlpha.tif");
            vigra::exportImage(vigra::srcImageRange(remapped.m_mask), exi);
}
#endif
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

/** functor to create a remapped image, loads image from disk */
template <typename ImageType, typename AlphaType>
class FileRemapper : public SingleImageRemapper<ImageType, AlphaType>
{
public:
    FileRemapper()
    {
        m_remapped = 0;
    }

    virtual ~FileRemapper() {};

    
#if 0

#define HUGIN_REMAP_IMGLOAD(TYPE, lut) \
{ \
    vigra::TYPE tmpImg(info.width(), info.height()); \
    if (alpha) { \
        vigra::importImageAlpha(info, vigra::destImage(tmpImg), \
                                vigra::destImage(srcAlpha)); \
{ \
    vigra::ImageExportInfo exi(DEBUG_FILE_PREFIX "hugin01_original_mask.tif"); \
    vigra::exportImage(vigra::srcImageRange(srcAlpha), exi); \
} \
} else { \
        vigra::importImage(info, vigra::destImage(tmpImg)); \
} \
{ \
    vigra::ImageExportInfo exi(DEBUG_FILE_PREFIX "hugin01_original.tif"); \
    vigra::exportImage(vigra::srcImageRange(tmpImg), exi); \
} \
}

    typedef std::vector<float> LUT;

#endif

    void loadImage(const PanoramaOptions & opts,
                 vigra::ImageImportInfo & info, ImageType & srcImg,
                 AlphaType & srcAlpha)
    {
    }

    virtual RemappedPanoImage<ImageType, AlphaType> *
    getRemapped(const Panorama & pano, const PanoramaOptions & opts,
                unsigned int imgNr, utils::MultiProgressDisplay & progress)
    {
        typedef typename ImageType::value_type PixelType;

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

        if (info.numExtraBands() > 0) {
            srcAlpha.resize(info.width(), info.height());
        }
        //int nb = info.numBands() - info.numExtraBands();
        bool alpha = info.numExtraBands() > 0;
        std::string type = info.getPixelType();

        SrcPanoImage src = pano.getSrcImage(imgNr);

        // import the image
        progress.setMessage(std::string("loading ") + utils::stripPath(img.getFilename()));

        if (alpha) {
            vigra::importImageAlpha(info, vigra::destImage(srcImg),
                                    vigra::destImage(srcAlpha));
        } else {
            vigra::importImage(info, vigra::destImage(srcImg));
        }
        // check if the image needs to be scaled to 0 .. 1,
        // this only works for int -> float, since the image
        // has already been loaded into the output container
        double maxv = vigra_ext::getMaxValForPixelType(info.getPixelType());
        if (maxv != vigra_ext::LUTTraits<PixelType>::max()) {
            double scale = ((double)vigra_ext::LUTTraits<PixelType>::max()) /  maxv;
            std::cout << "Scaling input image (pixel type: " << info.getPixelType() << " with: " << scale << std::endl;
            transformImage(vigra::srcImageRange(srcImg), destImage(srcImg),
                           vigra::functor::Arg1()*vigra::functor::Param(scale));
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

    virtual void release(RemappedPanoImage<ImageType,AlphaType> * d)
    {
        delete d;
    }

protected:
	RemappedPanoImage<ImageType,AlphaType> * m_remapped;
};



}; // namespace

#endif

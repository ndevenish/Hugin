// -*- c-basic-offset: 4 -*-
/** @file nona/RemappedPanoImage.h
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

#ifndef _NONA_REMAPPEDPANOIMAGE_H
#define _NONA_REMAPPEDPANOIMAGE_H

#include <vigra/impex.hxx>
#include <vigra/flatmorphology.hxx>
#include <vigra_ext/ROIImage.h>

#include <appbase/ProgressDisplayOld.h>

#include <panodata/SrcPanoImage.h>
#include <panodata/Mask.h>
#include <panodata/PanoramaOptions.h>
#include <panotools/PanoToolsInterface.h>


namespace HuginBase {
namespace Nona {


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


/** struct to hold a image state for stitching
 *
 */
template <class RemapImage, class AlphaImage>
class RemappedPanoImage : public vigra_ext::ROIImage<RemapImage, AlphaImage>
{

        typedef vigra_ext::ROIImage<RemapImage, AlphaImage> Base;

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

        
    public:
        /** create a remapped pano image
         *
         *  the actual remapping is done by the remapImage() function.
         */
        RemappedPanoImage()
        {};

        
    public:
        ///
        void setPanoImage(const SrcPanoImage & src,
                          const PanoramaOptions & dest,
                          vigra::Rect2D roi);

    //    /** set a new image or panorama options
    //     *
    //     *  This is needed before any of the remap functions can be used.
    //     *
    //     *  calculates bounding box, and outline
    //     */
    //    void setPanoImage(const vigra::Size2D & srcSize,
    //                      const PT::VariableMap & srcVars,
    //                      PT::Lens::LensProjectionFormat srcProj,
    //                      const PT::PanoImage & img,
    //                      const vigra::Diff2D &destSize,
    //                      PT::PanoramaOptions::ProjectionFormat destProj,
    //                      double destHFOV);
    //    
    //    ///
    //    void setPanoImage(const PT::Panorama & pano, unsigned int imgNr,
    //                      vigra::Size2D srcSize, const PT::PanoramaOptions & opts);
        

    public:
        /** calculate distance map. pixels contain distance from image center
         *
         *  setPanoImage() has to be called before!
         */
        template<class DistImgType>
            void calcSrcCoordImgs(DistImgType & imgX, DistImgType & imgY);

        /** calculate only the alpha channel.
         *  works for arbitrary transforms, with holes and so on,
         *  but is very crude and slow (remapps all image pixels...)
         *
         *  better transform all images, and get the alpha channel for free!
         *
         *  setPanoImage() should be called before.
         */
        void calcAlpha();

        /** remap a image without alpha channel*/
        template <class ImgIter, class ImgAccessor>
        void remapImage(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                        vigra_ext::Interpolator interpol,
                        AppBase::MultiProgressDisplay & progress);


        /** remap a image, with alpha channel */
        template <class ImgIter, class ImgAccessor,
                  class AlphaIter, class AlphaAccessor>
        void remapImage(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                        std::pair<AlphaIter, AlphaAccessor> alphaImg,
                        vigra_ext::Interpolator interp,
                        AppBase::MultiProgressDisplay & progress);
        
        
    public:
        ///
        vigra::ImageImportInfo::ICCProfile m_ICCProfile;

    protected:
        SrcPanoImage m_srcImg;
        PanoramaOptions m_destImg;
        PTools::Transform m_transf;

};



/** remap a single image
 */
template <class SrcImgType, class FlatImgType, class DestImgType, class MaskImgType>
void remapImage(SrcImgType & srcImg,
                const MaskImgType & srcAlpha,
                const FlatImgType & srcFlat,
                const SrcPanoImage & src,
                const PanoramaOptions & dest,
                vigra::Rect2D outputRect,
                RemappedPanoImage<DestImgType, MaskImgType> & remapped,
                AppBase::MultiProgressDisplay & progress);


} // namespace
} // namespace



//==============================================================================
// templated implementations


#include <photometric/ResponseTransform.h>
#include <vigra_ext/ImageTransforms.h>
#include <vigra_ext/ImageTransformsGPU.h>

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


namespace HuginBase {
namespace Nona {


template <class RemapImage, class AlphaImage>
void RemappedPanoImage<RemapImage,AlphaImage>::setPanoImage(const SrcPanoImage & src,
                  const PanoramaOptions & dest, vigra::Rect2D roi)
{
    // restrict to panorama size
    m_srcImg = src;
    m_destImg = dest;

    if (m_destImg.remapUsingGPU) {
        // Make width multiple of 8 for fast GPU transfers.
        const int r = roi.width() % 8;
        if (r != 0) roi.addSize(vigra::Size2D(8 - r, 0));
    }

    Base::resize(roi);
    m_transf.createTransform(src, dest);

    DEBUG_DEBUG("after resize: " << Base::m_region);
    DEBUG_DEBUG("m_srcImg size: " << m_srcImg.getSize());
}


#if 0
/** set a new image or panorama options
 *
 *  This is needed before any of the remap functions can be used.
 *
 *  calculates bounding box, and outline
 */
template <class RemapImage, class AlphaImage>
void RemappedPanoImage<RemapImage,AlphaImage>::setPanoImage(const vigra::Size2D & srcSize,
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

template <class RemapImage, class AlphaImage>
void RemappedPanoImage<RemapImage,AlphaImage>::setPanoImage(const PT::Panorama & pano, unsigned int imgNr,
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
template<class RemapImage, class AlphaImage>
template<class DistImgType>
void RemappedPanoImage<RemapImage,AlphaImage>::calcSrcCoordImgs(DistImgType & imgX, DistImgType & imgY)
{
    if (Base::boundingBox().isEmpty()) return;
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
template<class RemapImage, class AlphaImage>
void RemappedPanoImage<RemapImage,AlphaImage>::calcAlpha()
{
    if (Base::boundingBox().isEmpty())
        return;

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
            if(m_transf.transformImgCoord(sx,sy,x,y))
            {
                if (m_srcImg.isInside(vigra::Point2D(hugin_utils::roundi(sx),hugin_utils::roundi(sy))))
                {
                    *xalpha = 255;
                }
                else
                {
                    *xalpha = 0;
                };
            }
            else
            {
                *xalpha = 0;
            };
        }
    }
}

/** remap a image without alpha channel*/
template<class RemapImage, class AlphaImage>
template<class ImgIter, class ImgAccessor>
void RemappedPanoImage<RemapImage,AlphaImage>::remapImage(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                                                          vigra_ext::Interpolator interpol,
                                                          AppBase::MultiProgressDisplay & progress)
{

    //        std::ostringstream msg;
    //        msg <<"remapping image "  << imgNr;
    //        progress.setMessage(msg.str().c_str());

    const bool useGPU = m_destImg.remapUsingGPU;

    if (Base::boundingBox().isEmpty())
        return;

    vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;

    vigra::Size2D expectedSize = m_srcImg.getSize();
    if (useGPU) {
        const int r = expectedSize.width() % 8;
        if (r != 0) expectedSize += vigra::Diff2D(8 - r, 0);
    }

    DEBUG_DEBUG("srcImgSize: " << srcImgSize << " m_srcImgSize: " << m_srcImg.getSize());
    vigra_precondition(srcImgSize == expectedSize, 
                       "RemappedPanoImage<RemapImage,AlphaImage>::remapImage(): image sizes not consistent");

    typedef typename ImgAccessor::value_type input_value_type;
    typedef typename vigra_ext::ValueTypeTraits<input_value_type>::value_type input_component_type;

    // setup photometric transform for this image type
    // this corrects for response curve, white balance, exposure and 
    // radial vignetting
    Photometric::InvResponseTransform<input_component_type, double> invResponse(m_srcImg);
    invResponse.enforceMonotonicity();
    if (m_destImg.outputMode == PanoramaOptions::OUTPUT_LDR) {
        // select exposure and response curve for LDR output
        std::vector<double> outLut;
        vigra_ext::EMoR::createEMoRLUT(m_destImg.outputEMoRParams, outLut);
        double maxVal = vigra_ext::LUTTraits<input_value_type>::max();
        if (m_destImg.outputPixelType.size() > 0) {
            maxVal = vigra_ext::getMaxValForPixelType(m_destImg.outputPixelType);
        }

        invResponse.setOutput(1.0/pow(2.0,m_destImg.outputExposureValue), outLut,
                              maxVal);
    } else {
        invResponse.setHDROutput(true,1.0/pow(2.0,m_destImg.outputExposureValue));
    }


    if ((m_srcImg.hasActiveMasks()) || (m_srcImg.getCropMode() != SrcPanoImage::NO_CROP))
    {
        // need to create and additional alpha image for the crop mask...
        // not very efficient during the remapping phase, but works.
        vigra::BImage alpha(srcImgSize.x, srcImgSize.y);

        switch (m_srcImg.getCropMode()) {
        case SrcPanoImage::NO_CROP:
            {
                if (useGPU) {
                    if (srcImgSize != m_srcImg.getSize()) {
                        // src image with was increased for alignment reasons.
                        // Need to make an alpha image to mask off the extended region.
                        initImage(vigra::destImageRange(alpha),0);
                        initImage(alpha.upperLeft(), 
                            alpha.upperLeft()+m_srcImg.getSize(),
                            alpha.accessor(),255);
                    }
                    else
                        initImage(vigra::destImageRange(alpha),255);
                }
                else
                    initImage(vigra::destImageRange(alpha),255);
                break;
            }
        case SrcPanoImage::CROP_CIRCLE:
            {
                vigra::Rect2D cR = m_srcImg.getCropRect();
                hugin_utils::FDiff2D m( (cR.left() + cR.width()/2.0),
                        (cR.top() + cR.height()/2.0) );

                double radius = std::min(cR.width(), cR.height())/2.0;
                // Default the entire alpha channel to opaque..
                initImage(vigra::destImageRange(alpha),255);
                //..crop everything outside the circle
                vigra_ext::circularCrop(vigra::destImageRange(alpha), m, radius);
                break;
            }
        case SrcPanoImage::CROP_RECTANGLE:
            {
                vigra::Rect2D cR = m_srcImg.getCropRect();
                // Default the entire alpha channel to transparent..
                initImage(vigra::destImageRange(alpha),0);
                // Make sure crop is inside the image..
                cR &= vigra::Rect2D(0,0, srcImgSize.x, srcImgSize.y);
                // Opaque only the area within the crop rectangle..
                initImage(alpha.upperLeft()+cR.upperLeft(), 
                          alpha.upperLeft()+cR.lowerRight(),
                          alpha.accessor(),255);
                break;
            }
        default:
            break;
        }
        if(m_srcImg.hasActiveMasks())
            vigra_ext::applyMask(vigra::destImageRange(alpha), m_srcImg.getActiveMasks());
        if (useGPU) {
            transformImageAlphaGPU(srcImg,
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
        }
    } else {
        if (useGPU) {
            if (srcImgSize != m_srcImg.getSize()) {
                // src image with was increased for alignment reasons.
                // Need to make an alpha image to mask off the extended region.
                vigra::BImage alpha(srcImgSize.x, srcImgSize.y, vigra::UInt8(0));
                initImage(alpha.upperLeft(), 
                          alpha.upperLeft()+m_srcImg.getSize(),
                          alpha.accessor(),255);
                transformImageAlphaGPU(srcImg,
                                       vigra::srcImage(alpha),
                                       destImageRange(Base::m_image),
                                       destImage(Base::m_mask),
                                       Base::boundingBox().upperLeft(),
                                       m_transf,
                                       invResponse,
                                       m_srcImg.horizontalWarpNeeded(),
                                       interpol,
                                       progress);

            }
            else {
                transformImageGPU(srcImg,
                                  destImageRange(Base::m_image),
                                  destImage(Base::m_mask),
                                  Base::boundingBox().upperLeft(),
                                  m_transf,
                                  invResponse,
                                  m_srcImg.horizontalWarpNeeded(),
                                  interpol,
                                  progress);
            }
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
}



/** remap a image, with alpha channel */
template<class RemapImage, class AlphaImage>
template<class ImgIter, class ImgAccessor,
         class AlphaIter, class AlphaAccessor>
void RemappedPanoImage<RemapImage,AlphaImage>::remapImage(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                                                          std::pair<AlphaIter, AlphaAccessor> alphaImg,
                                                          vigra_ext::Interpolator interp,
                                                          AppBase::MultiProgressDisplay & progress)
{
    const bool useGPU = m_destImg.remapUsingGPU;

    if (Base::boundingBox().isEmpty())
        return;

    vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;

    vigra::Size2D expectedSize = m_srcImg.getSize();
    if (useGPU) {
        const int r = expectedSize.width() % 8;
        if (r != 0) expectedSize += vigra::Diff2D(8 - r, 0);
    }

    vigra_precondition(srcImgSize == expectedSize, 
                       "RemappedPanoImage<RemapImage,AlphaImage>::remapImage(): image sizes not consistent");

    typedef typename ImgAccessor::value_type input_value_type;
    typedef typename vigra_ext::ValueTypeTraits<input_value_type>::value_type input_component_type;

    // setup photometric transform for this image type
    // this corrects for response curve, white balance, exposure and 
    // radial vignetting
    Photometric::InvResponseTransform<input_component_type, double> invResponse(m_srcImg);
    if (m_destImg.outputMode == PanoramaOptions::OUTPUT_LDR) {
        // select exposure and response curve for LDR output
        std::vector<double> outLut;
        // scale up to desired output format
        double maxVal = vigra_ext::LUTTraits<input_value_type>::max();
        if (m_destImg.outputPixelType.size() > 0) {
            maxVal = vigra_ext::getMaxValForPixelType(m_destImg.outputPixelType);
        }
        vigra_ext::EMoR::createEMoRLUT(m_destImg.outputEMoRParams, outLut);
		vigra_ext::enforceMonotonicity(outLut);
        invResponse.setOutput(1.0/pow(2.0,m_destImg.outputExposureValue), outLut,
                              maxVal);
    } else {
        invResponse.setHDROutput(true,1.0/pow(2.0,m_destImg.outputExposureValue));
    }

    if (((m_srcImg.hasActiveMasks()) || (m_srcImg.getCropMode() != SrcPanoImage::NO_CROP))) {
        vigra::BImage alpha(srcImgSize);
        vigra::Rect2D cR = m_srcImg.getCropRect();
        switch (m_srcImg.getCropMode()) {
            case SrcPanoImage::NO_CROP:
            {
                // Just copy the source alpha channel and crop it down.
                vigra::copyImage(vigra::make_triple(alphaImg.first,
                        alphaImg.first + srcImgSize, alphaImg.second),
                        vigra::destImage(alpha));
                break;
            }
            case SrcPanoImage::CROP_CIRCLE:
            {
                // Just copy the source alpha channel and crop it down.
                vigra::copyImage(vigra::make_triple(alphaImg.first,
                        alphaImg.first + srcImgSize, alphaImg.second),
                        vigra::destImage(alpha));
                hugin_utils::FDiff2D m( (cR.left() + cR.width()/2.0),
                            (cR.top() + cR.height()/2.0) );
                double radius = std::min(cR.width(), cR.height())/2.0;
                vigra_ext::circularCrop(vigra::destImageRange(alpha), m, radius);
                break;
            }
            case SrcPanoImage::CROP_RECTANGLE:
            {
                // Intersect the cropping rectangle with the one from the base
                // image to ensure it fits inside.
                cR &= vigra::Rect2D(0,0, srcImgSize.x, srcImgSize.y);

                // Start with a blank alpha channel for the destination
                initImage(vigra::destImageRange(alpha),0);

                // Copy in only the area inside the rectangle
                vigra::copyImage(alphaImg.first + cR.upperLeft(),
                        alphaImg.first + cR.lowerRight(),
                        alphaImg.second,
                        alpha.upperLeft() + cR.upperLeft(),alpha.accessor());
                break;
            }
            default:
                break;
        }
        if(m_srcImg.hasActiveMasks())
            vigra_ext::applyMask(vigra::destImageRange(alpha), m_srcImg.getActiveMasks());

        if (useGPU) {
            vigra_ext::transformImageAlphaGPU(srcImg,
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
                                           vigra::srcImage(alpha),
                                           destImageRange(Base::m_image),
                                           destImage(Base::m_mask),
                                           Base::boundingBox().upperLeft(),
                                           m_transf,
                                           invResponse,
                                           m_srcImg.horizontalWarpNeeded(),
                                           interp,
                                           progress);
        }
    } else {
        if (useGPU) {
            // extended region (if any) should already be cleared since ImportImageAlpha shouldn't have touched it.
            vigra_ext::transformImageAlphaGPU(srcImg,
                                              alphaImg,
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
}






/** remap a single image
 */
template <class SrcImgType, class FlatImgType, class DestImgType, class MaskImgType>
void remapImage(SrcImgType & srcImg,
                const MaskImgType & srcAlpha,
                const FlatImgType & srcFlat,
                const SrcPanoImage & src,
                const PanoramaOptions & dest,
                vigra::Rect2D outputROI,
//                vigra_ext::Interpolator interpolator,
                RemappedPanoImage<DestImgType, MaskImgType> & remapped,
                AppBase::MultiProgressDisplay & progress)
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

    progress.setMessage(std::string("remapping ") + hugin_utils::stripPath(src.getFilename()));
    // set pano image
    DEBUG_DEBUG("setting src image with size: " << src.getSize());
    remapped.setPanoImage(src, dest, outputROI);
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


} //namespace
} //namespace

#endif // _H

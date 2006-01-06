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
#include <vigra/impexalpha.hxx>
#include <vigra/flatmorphology.hxx>
#include <vigra/transformimage.hxx>
//#include <vigra/functorexpression.hxx>

#include <vigra_ext/Interpolators.h>
#include <vigra_ext/ROIImage.h>
#include <vigra_ext/utils.h>
#include <vigra_ext/VignettingCorrection.h>

#include <PT/Panorama.h>

#include <PT/PanoToolsInterface.h>
#include <common/math.h>

namespace PT
{

template <class TRANSFORM>
void estimateImageAlpha(vigra::Diff2D destSize,
                       vigra::Diff2D srcSize,
                       bool doCrop,
                       vigra::Rect2D cropRect,
                       bool circularCrop,
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
    scale = std::min(maxLength/destSize.x, maxLength/destSize.y);

    vigra::Diff2D destSz( utils::roundi(ceil(destSize.x * scale)), 
		          utils::roundi(ceil(destSize.y * scale)));

    FDiff2D cropCenter;
    double radius2;
    if (doCrop && circularCrop) {
        cropCenter.x = cropRect.left() + cropRect.width()/2.0;
        cropCenter.y = cropRect.top() + cropRect.height()/2.0;
        radius2 = std::min(cropRect.width()/2.0, cropRect.height()/2.0);
        radius2 = radius2 * radius2;
    }

    // remap image
    vigra::BImage img(destSz);
    for (int y=0; y < destSz.y; y++) {
        for (int x=0; x < destSz.x; x++) {
            // sample image
            // coordinates in real image pixels
            double sx,sy;
            transf.transformImgCoord(sx,sy, x/scale, y/scale);
            if (sx >= 0 && sx < srcSize.x && sy >= 0 && sy < srcSize.y) {
                // remapped point is inside image
//                img(y,x) = 255;
                bool valid=true;
                if (doCrop) {
                    if (circularCrop) {
                        sx = sx - cropCenter.x;
                        sy = sy - cropCenter.y;
                        if (sx*sx + sy*sy > radius2) {
                            valid = false;
                        }
                    } else {
                        if (!cropRect.contains(vigra::Point2D(utils::roundi(sx), utils::roundi(sy))) ) {
                            valid = false;
                        }
                    }
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
    }

    imgRect.setUpperLeft(vigra::Point2D(utils::roundi(ul.x), utils::roundi(ul.y)));
    imgRect.setLowerRight(vigra::Point2D(utils::roundi(lr.x), utils::roundi(lr.y)));

    // ensure that the roi is inside the panorama
    vigra::Rect2D panoRect(0,0, destSize.x, destSize.y);
    imgRect = panoRect & imgRect;
    DEBUG_DEBUG("bounding box: " << imgRect);

    alpha.resize(img.size());
    // dilate alpha image, to cover neighbouring pixels,
    // that may be valid in the full resolution image
    vigra::discDilation(vigra::srcImageRange(img),
                        vigra::destImage(alpha), 1);

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
void estimateImageRect(vigra::Diff2D destSize,
                       vigra::Diff2D srcSize,
                       bool doCrop,
                       vigra::Rect2D cropRect,
                       bool circularCrop,
                       TRANSFORM & transf,
                       vigra::Rect2D & imgRect)
{
    vigra::BImage img;
    double scale;
    estimateImageAlpha(destSize, srcSize, doCrop, cropRect, circularCrop, transf, imgRect, img, scale);
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

    /** set a new image or panorama options
     *
     *  calculates bounding box, and outline
     */
    void setPanoImage(const PT::Panorama & pano, unsigned int imgNr, const PT::PanoramaOptions & opts)
    {
    const PT::PanoImage & img = pano.getImage(imgNr);

	vigra::Diff2D srcSize;
	srcSize.x = img.getWidth();
	srcSize.y = img.getHeight();

	// create transforms
	//    SpaceTransform t;
	//    SpaceTransform invT;
	PTools::Transform transf;
	PTools::Transform invTransf;
	
	transf.createTransform(pano, imgNr, opts, srcSize);
	invTransf.createInvTransform(pano, imgNr, opts, srcSize);

	// calculate ROI for this image.

    ImageOptions imgOpts = pano.getImage(imgNr).getOptions();
    vigra::Rect2D imageRect;
    bool circCrop = pano.getLens(pano.getImage(imgNr).getLensNr()).getProjection() == Lens::CIRCULAR_FISHEYE;
    estimateImageRect(vigra::Size2D(opts.getWidth(), opts.getHeight()), srcSize,
                      imgOpts.docrop, imgOpts.cropRect, circCrop,  
                      transf,
                      imageRect);

    // restrict to panorama size
	Base::resize(imageRect);
	DEBUG_DEBUG("after resize: " << Base::m_region);
    }


    /** calculate distance map. pixels contain distance from image center
     */
    template<class DistImgType>
    void calcDistMap(const PT::Panorama & pano, const PT::PanoramaOptions & opts,
		          unsigned int imgNr, DistImgType & img)
    {
        setPanoImage(pano, imgNr, opts);
        vigra::Diff2D srcSize(pano.getImage(imgNr).getWidth(),
            pano.getImage(imgNr).getHeight());
        img.resize(Base::boundingBox().size());
        // calculate the alpha channel,
        int xstart = Base::boundingBox().left();
        int xend   = Base::boundingBox().right();
        int ystart = Base::boundingBox().top();
        int yend   = Base::boundingBox().bottom();

        PTools::Transform transf;
	
    	transf.createTransform(pano, imgNr, opts, srcSize);

        vigra::Diff2D srcMiddle = srcSize / 2;

	    // create dist y iterator
	    typename DistImgType::Iterator yalpha(img.upperLeft());
	    // loop over the image and transform
	    for(int y=ystart; y < yend; ++y, ++yalpha.y)
	    {
	        // create x iterators
            typename DistImgType::Iterator xalpha(yalpha);
            for(int x=xstart; x < xend; ++x, ++xalpha.x)
            {
                double sx,sy;
                transf.transformImgCoord(sx,sy,x,y);
                // check if a value could have been calculated here
                // add 0.5 pixels of padding, in case of rounding error
                // during the transform. The interpolation will fill the
                // pixels up to half of the interpolation kernel width
                // outside of the source image
                if (sx < -0.5
		            || sx > srcSize.x +0.5
		            || sy < -0.5
		            || sy > srcSize.y+0.5)
                {
                    *xalpha = 0;
                    // nothing..
                } else {
                    double mx = sx - srcMiddle.x;
                    double my = sy - srcMiddle.y;
                    *xalpha = sqrt(mx*mx + my*my);
                }
            }
        }
    }

    /** calculate only the alpha channel.
     *  works for arbitrary transforms, with holes and so on,
     *  but is very crude and slow (remapps all image pixels...)
     *
     *  better transform all images, and get the alpha channel for free!
     *
     *  this is a hack!
     */
    void calcAlpha(const PT::Panorama & pano, const PT::PanoramaOptions & opts,
		   unsigned int imgNr)
    {
        DEBUG_DEBUG("imgNr: " << imgNr);
	setPanoImage(pano, imgNr, opts);
	vigra::Diff2D srcSize(pano.getImage(imgNr).getWidth(),
			      pano.getImage(imgNr).getHeight());
	Base::m_mask.resize(Base::boundingBox().size());
	// calculate the alpha channel,
	int xstart = Base::boundingBox().left();
	int xend   = Base::boundingBox().right();
	int ystart = Base::boundingBox().top();
	int yend   = Base::boundingBox().bottom();

	// hack.. doesn't belong here..
	int interpolHalfWidth=0;
	
	PTools::Transform transf;
	
	transf.createTransform(pano, imgNr, opts, srcSize);

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
		 transf.transformImgCoord(sx,sy,x,y);
         // check if a value could have been calculated here
         // add 0.5 pixels of padding, in case of rounding error
         // during the transform. The interpolation will fill the
         // pixels up to half of the interpolation kernel width
         // outside of the source image
         if (sx < -0.5
		     || sx > srcSize.x +0.5
		     || sy < -0.5
		     || sy > srcSize.y+0.5)
		 {
		     *xalpha = 0;
		     // nothing..
		 } else {
		     *xalpha = 255;
		 }
	     }
	 }
    }

    /** remap a image without alpha channel*/
    template <class ImgIter, class ImgAccessor>
    void remapImage(const PT::Panorama & pano,
		    const PT::PanoramaOptions & opts,
            vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
		    unsigned int imgNr,
		    utils::MultiProgressDisplay & progress)
    {
        DEBUG_TRACE("Image: " << imgNr);
        setPanoImage(pano, imgNr, opts);
        //        std::ostringstream msg;
        //        msg <<"remapping image "  << imgNr;
        //        progress.setMessage(msg.str().c_str());

        PTools::Transform transf;
        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;
        transf.createTransform(pano, imgNr, opts, srcImgSize);

        const PanoImage & img = pano.getImage(imgNr);
        const ImageOptions & imgOpts = img.getOptions();

        double scale = srcImgSize.x / (double) img.getWidth();

        bool warparound = (opts.getProjection() == PT::PanoramaOptions::EQUIRECTANGULAR && opts.getHFOV() == 360);

        if (imgOpts.docrop) {
            vigra::BImage alpha(srcImgSize.x, srcImgSize.y, 255);
            if (pano.getLens(img.getLensNr()).getProjection() == Lens::CIRCULAR_FISHEYE) {
                FDiff2D m( (imgOpts.cropRect.left() + imgOpts.cropRect.width()/2.0) * scale,
                           (imgOpts.cropRect.top() + imgOpts.cropRect.height()/2.0) * scale);

                double radius = std::min(imgOpts.cropRect.width(), imgOpts.cropRect.height())/2.0;
                radius = radius * scale;
                vigra_ext::circularCrop(vigra::destImageRange(alpha), m, radius);
            } else {
                //todo implement rectangular crop
            }

            transformImageAlpha(srcImg,
                                vigra::srcImage(alpha),
                                destImageRange(Base::m_image),
                                destImage(Base::m_mask),
                                Base::boundingBox().upperLeft(),
                                transf,
                                warparound,
                                opts.interpolator,
                                progress);
        } else {
            transformImage(srcImg,
                           destImageRange(Base::m_image),
                           destImage(Base::m_mask),
                           Base::boundingBox().upperLeft(),
                           transf,
                           warparound,
                           opts.interpolator,
                           progress);
        }
    }


    /** remap a image, with alpha channel */
    template <class ImgIter, class ImgAccessor,
              class AlphaIter, class AlphaAccessor>
    void remapImage(const PT::Panorama & pano,
                    const PT::PanoramaOptions & opts,
                    vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                    std::pair<AlphaIter, AlphaAccessor> alphaImg,
                    unsigned int imgNr,
                    utils::MultiProgressDisplay & progress)
    {
        DEBUG_TRACE("Image: " << imgNr);
        setPanoImage(pano, imgNr, opts);

        PTools::Transform transf;
        vigra::Diff2D srcImgSize = srcImg.second - srcImg.first;
        transf.createTransform(pano, imgNr, opts, srcImgSize);
        
        const PanoImage & img = pano.getImage(imgNr);
        const ImageOptions & imgOpts = img.getOptions();

        double scale = srcImgSize.x / (double) img.getWidth();
        
        bool warparound = (opts.getProjection() == PanoramaOptions::EQUIRECTANGULAR && opts.getHFOV() == 360);

        if (imgOpts.docrop) {
            vigra::BImage alpha(srcImgSize);
            vigra::copyImage(vigra::make_triple(alphaImg.first, alphaImg.first + srcImgSize, alphaImg.second),
                             vigra::destImage(alpha));
            if (pano.getLens(img.getLensNr()).getProjection() == Lens::CIRCULAR_FISHEYE) {
                FDiff2D m( (imgOpts.cropRect.left() + imgOpts.cropRect.width()/2.0) * scale,
                           (imgOpts.cropRect.top() + imgOpts.cropRect.height()/2.0) * scale);

                double radius = std::min(imgOpts.cropRect.width(), imgOpts.cropRect.height())/2.0;
                radius = radius * scale;
                vigra_ext::circularCrop(vigra::destImageRange(alpha), m, radius);
            } else {
                //todo implement rectangular crop
            }

            transformImageAlpha(srcImg,
                                vigra::srcImage(alpha),
                                destImageRange(Base::m_image),
                                destImage(Base::m_mask),
                                Base::boundingBox().upperLeft(),
                                transf,
                                warparound,
                                opts.interpolator,
                                progress);
        } else {
            transformImageAlpha(srcImg,
                        alphaImg,
                        destImageRange(Base::m_image),
                        destImage(Base::m_mask),
                        Base::boundingBox().upperLeft(),
                        transf,
                        warparound,
                        opts.interpolator,
                        progress);
        }

    }


//    const std::vector<FDiff2D> & getOutline()
//    {
//        return m_outline;
//    }

protected:
    /// outline of panorama
//    std::vector<FDiff2D> m_outline;
};


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
                    typename vigra::NumericTraits<typename SrcAccessor::value_type>::RealPromote b)
{
    FFType ffImg(ffInfo.width(), ffInfo.height());
    vigra::importImage(ffInfo, vigra::destImage(ffImg));
    vigra_ext::flatfieldVigCorrection(srcImg, vigra::srcImage(ffImg), 
                                      destImg, gamma, gammaMaxVal, division, a, b);
}

// 3 channel images
template <class T>
bool convertKParams(const VariableMap & vars,
                    T & a,
                    T & b,
                    vigra::VigraFalseType)
{
    DEBUG_ASSERT(a.size() == 3);
    bool ret(true);
    for (unsigned int i=0; i< 3; i++) {
        char vn[6];
        snprintf(vn, 6,"K%da",i);
        a[i] = const_map_get(vars, vn).getValue();
        snprintf(vn, 6,"K%db",i);
        b[i] = const_map_get(vars, vn).getValue();
        ret = ret && a[i] == 1.0 && b[i] == 0.0;
    }

    return ret;
}

// singe channel images
template <class T>
bool convertKParams(const VariableMap & vars,
                    T & a,
                    T & b,
                    vigra::VigraTrueType)
{
    a = const_map_get(vars, "K0a").getValue();
    b = const_map_get(vars, "K0b").getValue();
    return (a == 1.0 && b == 0.0);
}

// singe channel images
template <class T>
bool convertKParams(const VariableMap & vars,
                    T & a,
                    T & b)
{
    typedef typename vigra::NumericTraits<T>::isScalar is_scalar;
    return convertKParams(vars, a, b, is_scalar());
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
            return getRemappedIntern<RealPixelType>(pano, opts, imgNr, progress);
        } else {
            return getRemappedIntern<PixelType>(pano, opts, imgNr, progress);
        }
    }

    /** further specialisation to be able to process integer images without cast problems */
    template <class PixelType>
    RemappedPanoImage<ImageType, AlphaType> *
    getRemappedIntern(const Panorama & pano, const PanoramaOptions & opts,
                      unsigned int imgNr, utils::MultiProgressDisplay & progress)
    {
        DEBUG_TRACE("Image: " << imgNr);

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
                                vigra::destImage(srcImg), ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb);
            } else if (strcmp(ffInfo.getPixelType(), "INT16") == 0 ) {
                applyFlatfield<vigra::SImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb);
            } else if (strcmp(ffInfo.getPixelType(), "UINT16") == 0 ) {
                 applyFlatfield<vigra::USImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb);
            } else if (strcmp(ffInfo.getPixelType(), "UINT32") == 0 ) {
                 applyFlatfield<vigra::IImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb);
            } else if (strcmp(ffInfo.getPixelType(), "INT32") == 0 ) {
                 applyFlatfield<vigra::UIImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb);
            } else if (strcmp(ffInfo.getPixelType(), "FLOAT") == 0 ) {
                 applyFlatfield<vigra::FImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb);
            } else if (strcmp(ffInfo.getPixelType(), "DOUBLE") == 0 ) {
                 applyFlatfield<vigra::DImage, csI, csA, sI, sA>(srcImageRange(srcImg), destImage(srcImg), 
                                ffInfo, opts.gamma, gMaxVal, vigCorrDivision, ka, kb);
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
                                           vigCorrDivision, ka, kb);
        } else if (opts.gamma != 1.0 && doBrightnessConversion ) {
            progress.setMessage(std::string("inverse brightness & gamma correction ") + utils::stripPath(img.getFilename()));
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

    virtual void release(RemappedPanoImage<ImageType,AlphaType> * d)
    {
        delete d;
    }

protected:
	RemappedPanoImage<ImageType,AlphaType> * m_remapped;
};



}; // namespace

#endif

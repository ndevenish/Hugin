// -*- c-basic-offset: 4 ; tab-width: 4 -*-
/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* <http://www.gnu.org/licenses/>.
*/

#include "ImageImport.h"

#include "PanoDetector.h"
#include <iostream>
#include <fstream>
#include <vigra/distancetransform.hxx>
#include "vigra_ext/impexalpha.hxx"
#include "vigra_ext/cms.h"

#include <localfeatures/Sieve.h>
#include <localfeatures/PointMatch.h>
#include <localfeatures/RansacFiltering.h>
#include <localfeatures/KeyPointIO.h>
#include <localfeatures/CircularKeyPointDescriptor.h>

/*
#include "KDTree.h"
#include "KDTreeImpl.h"
*/
#include "Utils.h"
#include "Tracer.h"

#include <algorithms/nona/ComputeImageROI.h>
#include <algorithms/optimizer/PTOptimizer.h>
#include <nona/RemappedPanoImage.h>
#include <nona/ImageRemapper.h>

#include <time.h>

#define TRACE_IMG(X) {if (iPanoDetector.getVerbose() > 1) { TRACE_INFO("i" << ioImgInfo._number << " : " << X << std::endl);} }
#define TRACE_PAIR(X) {if (iPanoDetector.getVerbose() > 1){ TRACE_INFO("i" << ioMatchData._i1->_number << " <> " \
                "i" << ioMatchData._i2->_number << " : " << X << std::endl)}}

// define a Keypoint insertor
class KeyPointVectInsertor : public lfeat::KeyPointInsertor
{
public:
    explicit KeyPointVectInsertor(lfeat::KeyPointVect_t& iVect) : _v(iVect) {};
    inline virtual void operator()(const lfeat::KeyPoint& k)
    {
        _v.push_back(lfeat::KeyPointPtr(new lfeat::KeyPoint(k)));
    }

private:
    lfeat::KeyPointVect_t& _v;

};


// define a sieve extractor
class SieveExtractorKP : public lfeat::SieveExtractor<lfeat::KeyPointPtr>
{
public:
    explicit SieveExtractorKP(lfeat::KeyPointVect_t& iV) : _v(iV) {};
    inline virtual void operator()(const lfeat::KeyPointPtr& k)
    {
        _v.push_back(k);
    }
private:
    lfeat::KeyPointVect_t& _v;
};

class SieveExtractorMatch : public lfeat::SieveExtractor<lfeat::PointMatchPtr>
{
public:
    explicit SieveExtractorMatch(lfeat::PointMatchVector_t& iM) : _m(iM) {};
    inline virtual void operator()(const lfeat::PointMatchPtr& m)
    {
        _m.push_back(m);
    }
private:
    lfeat::PointMatchVector_t& _m;
};

bool PanoDetector::LoadKeypoints(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Loading keypoints...");

    lfeat::ImageInfo info = lfeat::loadKeypoints(ioImgInfo._keyfilename, ioImgInfo._kp);
    ioImgInfo._loadFail = (info.filename.size() == 0);

    // update ImgData
    if(ioImgInfo._needsremap)
    {
        ioImgInfo._detectWidth = std::max(info.width,info.height);
        ioImgInfo._detectHeight = std::max(info.width,info.height);
        ioImgInfo._projOpts.setWidth(ioImgInfo._detectWidth);
        ioImgInfo._projOpts.setHeight(ioImgInfo._detectHeight);
    }
    else
    {
        ioImgInfo._detectWidth = info.width;
        ioImgInfo._detectHeight = info.height;
    };
    ioImgInfo._descLength = info.dimensions;

    return true;
}

/** apply the mask and the crop of the given SrcImg to given mask image */ 
template <class SrcImageIterator, class SrcAccessor>
void applyMaskAndCrop(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> img, const HuginBase::SrcPanoImage& SrcImg)
{
    vigra::Diff2D imgSize = img.second - img.first;

    // create dest y iterator
    SrcImageIterator yd(img.first);
    // loop over the image and transform
    for(int y=0; y < imgSize.y; ++y, ++yd.y)
    {
        // create x iterators
        SrcImageIterator xd(yd);
        for(int x=0; x < imgSize.x; ++x, ++xd.x)
        {
            if(!SrcImg.isInside(vigra::Point2D(x,y)))
            {
                *xd=0;
            };
        }
    }
}

/** functor to scale image on the fly during other operations */
template <class T>
struct ScaleFunctor
{
    typedef T result_type;
    explicit ScaleFunctor(double scale) { m_scale = scale; };

    T operator()(const T & a) const
    {
        return m_scale*a;
    }

    template <class T2>
    T2 operator()(const T2 & a, const hugin_utils::FDiff2D & p) const
    {
        return m_scale*a;
    }

    template <class T2, class A>
    A hdrWeight(T2 v, A a) const
    {
        return a;
    }

private:
    double m_scale;
};

/** helper function to remap image to given projection, you can supply a pixelTransform, 
 *  which will be applied during remapping, this is intended for scaling a image during 
 *  remapping, but this means also, that no photometric corrections are applied,
 *  if this is wanted you need to supply a suitable pixelTransform */
template <class ImageType, class PixelTransform>
void RemapImage(const HuginBase::SrcPanoImage& srcImage, const HuginBase::PanoramaOptions& options,
    size_t detectWidth, size_t detectHeight,
    ImageType*& image, vigra::BImage*& mask,
    const PixelTransform& pixelTransform,
    ImageType*& finalImage, vigra::BImage*& finalMask)
{
    AppBase::DummyProgressDisplay dummy;
    HuginBase::PTools::Transform transform;
    transform.createTransform(srcImage, options);
    finalImage = new ImageType(detectWidth, detectHeight);
    finalMask = new vigra::BImage(detectWidth, detectHeight, vigra::UInt8(0));
    if (mask)
    {
        vigra_ext::transformImageAlpha(vigra::srcImageRange(*image), vigra::srcImage(*mask), vigra::destImageRange(*finalImage), vigra::destImage(*finalMask),
            options.getROI().upperLeft(), transform, pixelTransform, false, vigra_ext::INTERP_CUBIC, &dummy);
        delete mask;
        mask = NULL;
    }
    else
    {
        vigra_ext::transformImage(vigra::srcImageRange(*image), vigra::destImageRange(*finalImage), vigra::destImage(*finalMask),
            options.getROI().upperLeft(), transform, pixelTransform, false, vigra_ext::INTERP_CUBIC, &dummy);
    };
    delete image;
    image = NULL;
}

/** downscale image if requested, optimized code for non-downscale version to prevent unnecessary copying 
 *  the image data */
template <class ImageType>
void HandleDownscaleImage(const HuginBase::SrcPanoImage& srcImage, ImageType*& image, vigra::BImage*& mask, 
    size_t detectWidth, size_t detectHeight, bool downscale,
    ImageType*& finalImage, vigra::BImage*& finalMask)
{
    if (srcImage.hasActiveMasks() || (srcImage.getCropMode() != HuginBase::SrcPanoImage::NO_CROP && !srcImage.getCropRect().isEmpty()))
    {
        if (!mask)
        {
            // image has no mask, create full mask
            mask = new vigra::BImage(image->size(), vigra::UInt8(255));
        };
        //copy mask and crop from pto file into alpha layer
        applyMaskAndCrop(vigra::destImageRange(*mask), srcImage);
    };
    if (downscale)
    {
        // Downscale image
        finalImage = new ImageType(detectWidth, detectHeight);
        vigra::resizeImageNoInterpolation(vigra::srcImageRange(*image), vigra::destImageRange(*finalImage));
        delete image;
        image = NULL;
        //downscale mask
        if (mask)
        {
            finalMask = new vigra::BImage(detectWidth, detectHeight);
            vigra::resizeImageNoInterpolation(vigra::srcImageRange(*mask), vigra::destImageRange(*finalMask));
            delete mask;
            mask = NULL;
        };
    }
    else
    {
        // simply copy pointer instead of copying the whole image data
        finalImage = image;
        if (mask)
        {
            finalMask = mask;
        };
    };
};

// save some intermediate images to disc if defined
// #define DEBUG_LOADING_REMAPPING
bool PanoDetector::AnalyzeImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    vigra::DImage* final_img = NULL;
    vigra::BImage* final_mask = NULL;

    try
    {
        ioImgInfo._loadFail=false;

        TRACE_IMG("Load image...");
        vigra::ImageImportInfo aImageInfo(ioImgInfo._name.c_str());
        if (aImageInfo.numExtraBands() > 1)
        {
            TRACE_INFO("Image with multiple alpha channels are not supported");
            ioImgInfo._loadFail = true;
            return false;
        };
        // remark: it would be possible to handle all cases with the same code
        // but this would mean that in some cases there are unnecessary
        // range conversions and image data copying actions needed
        // so we use specialed code for several cases to reduce memory usage
        // and prevent unnecessary range adaptions
        if (aImageInfo.isGrayscale())
        {
            // gray scale image
            vigra::DImage* image = new vigra::DImage(aImageInfo.size());
            vigra::BImage* mask = NULL;
            // load gray scale image
            if (aImageInfo.numExtraBands() == 1)
            {
                mask=new vigra::BImage(aImageInfo.size());
                vigra::importImageAlpha(aImageInfo, vigra::destImage(*image), vigra::destImage(*mask));
            }
            else
            {
                vigra::importImage(aImageInfo, vigra::destImage(*image));
            };
            // adopt range
            double minVal = 0;
            double maxVal;
            if (aImageInfo.getPixelType() == std::string("FLOAT") || aImageInfo.getPixelType() == std::string("DOUBLE"))
            {
                vigra::FindMinMax<float> minmax;   // init functor
                vigra::inspectImage(vigra::srcImageRange(*image), minmax);
                minVal = minmax.min;
                maxVal = minmax.max;
            }
            else
            {
                maxVal = vigra_ext::getMaxValForPixelType(aImageInfo.getPixelType());
            };
            bool range255 = (fabs(maxVal - 255) < 0.01 && fabs(minVal) < 0.01);
            if (aImageInfo.getICCProfile().empty())
            {
                // no icc profile, cpfind expects images in 0 ..255 range
                TRACE_IMG("Rescale range...");
                if (!range255)
                {
                    vigra::transformImage(vigra::srcImageRange(*image), vigra::destImage(*image),
                        vigra::linearRangeMapping(minVal, maxVal, 0.0, 255.0));
                };
                range255 = true;
            }
            else
            {
                // apply ICC profile
                TRACE_IMG("Applying icc profile...");
                // lcms expects for double datatype all values between 0 and 1
                vigra::transformImage(vigra::srcImageRange(*image), vigra::destImage(*image),
                    vigra::linearRangeMapping(minVal, maxVal, 0.0, 1.0));
                range255 = false;
                HuginBase::Color::ApplyICCProfile(*image, aImageInfo.getICCProfile(), TYPE_GRAY_DBL);
            };
            if (ioImgInfo._needsremap)
            {
                // remap image
                TRACE_IMG("Remapping image...");
                if (range255)
                {
                    RemapImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), ioImgInfo._projOpts,
                        ioImgInfo._detectWidth, ioImgInfo._detectHeight, image, mask, vigra_ext::PassThroughFunctor<double>(),
                        final_img, final_mask);
                }
                else
                {
                    // images has been scaled to 0..1 range before, scale back to 0..255 range
                    RemapImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), ioImgInfo._projOpts,
                        ioImgInfo._detectWidth, ioImgInfo._detectHeight, image, mask, ScaleFunctor<double>(255.0),
                        final_img, final_mask);
                };
            }
            else
            {
                if (range255)
                {
                    TRACE_IMG("Downscale and transform to suitable grayscale...");
                    HandleDownscaleImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), image, mask,
                        ioImgInfo._detectWidth, ioImgInfo._detectHeight, iPanoDetector._downscale, 
                        final_img, final_mask);
                }
                else
                {
                    TRACE_IMG("Transform to suitable grayscale...");
                    HandleDownscaleImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), image, mask,
                        ioImgInfo._detectWidth, ioImgInfo._detectHeight, iPanoDetector._downscale,
                        final_img, final_mask);
                    vigra::transformImage(vigra::srcImageRange(*final_img), vigra::destImage(*final_img), vigra::linearRangeMapping(0, 1, 0, 255));
                };
            };
            if (iPanoDetector.getCeleste())
            {
                TRACE_IMG("Celeste does not work with grayscale images. Skipping...");
            };
        }
        else
        {
            if (aImageInfo.isColor())
            {
                // rgb images
                // prepare radius parameter for celeste
                int radius = 1;
                if (iPanoDetector.getCeleste())
                {
                    radius = iPanoDetector.getCelesteRadius();
                    if (iPanoDetector._downscale)
                    {
                        radius >>= 1;
                    };
                    if (radius < 2)
                    {
                        radius = 2;
                    };
                };
                switch (aImageInfo.pixelType())
                {
                    case vigra::ImageImportInfo::UINT8:
                        // special variant for unsigned 8 bit images
                        {
                            vigra::BRGBImage* rgbImage=new vigra::BRGBImage(aImageInfo.size());
                            vigra::BImage* mask = NULL;
                            // load image
                            if (aImageInfo.numExtraBands() == 1)
                            {
                                mask=new vigra::BImage(aImageInfo.size());
                                vigra::importImageAlpha(aImageInfo, vigra::destImage(*rgbImage), vigra::destImage(*mask));
                            }
                            else
                            {
                                vigra::importImage(aImageInfo, vigra::destImage(*rgbImage));
                            };
                            // apply icc profile
                            if (!aImageInfo.getICCProfile().empty())
                            {
                                TRACE_IMG("Applying icc profile...");
                                HuginBase::Color::ApplyICCProfile(*rgbImage, aImageInfo.getICCProfile(), TYPE_RGB_8);
                            };
                            vigra::BRGBImage* scaled = NULL;
                            if (ioImgInfo._needsremap)
                            {
                                // remap image
                                TRACE_IMG("Remapping image...");
                                RemapImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), ioImgInfo._projOpts,
                                    ioImgInfo._detectWidth, ioImgInfo._detectHeight, rgbImage, mask, 
                                    vigra_ext::PassThroughFunctor<vigra::RGBValue<vigra::UInt8> >(),
                                    scaled, final_mask);
                            }
                            else
                            {
                                if (iPanoDetector._downscale)
                                {
                                    TRACE_IMG("Downscale image...");
                                };
                                HandleDownscaleImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), rgbImage, mask,
                                    ioImgInfo._detectWidth, ioImgInfo._detectHeight, iPanoDetector._downscale, 
                                    scaled, final_mask);
                            };
                            if (iPanoDetector.getCeleste())
                            {
                                TRACE_IMG("Mask areas with clouds...");
                                vigra::UInt16RGBImage* image16=new vigra::UInt16RGBImage(scaled->size());
                                vigra::transformImage(vigra::srcImageRange(*scaled), vigra::destImage(*image16),
                                    vigra::linearIntensityTransform<vigra::RGBValue<vigra::UInt16> >(255));
                                vigra::BImage* celeste_mask = celeste::getCelesteMask(iPanoDetector.svmModel, *image16, radius, iPanoDetector.getCelesteThreshold(), 800, true, false);
#ifdef DEBUG_LOADING_REMAPPING
                                // DEBUG: export celeste mask
                                std::ostringstream maskfilename;
                                maskfilename << ioImgInfo._name << "_celeste_mask.JPG";
                                vigra::ImageExportInfo maskexinfo(maskfilename.str().c_str());
                                vigra::exportImage(vigra::srcImageRange(*celeste_mask), maskexinfo);
#endif
                                delete image16;
                                if (final_mask)
                                {
                                    vigra::copyImageIf(vigra::srcImageRange(*celeste_mask), vigra::srcImage(*final_mask), vigra::destImage(*final_mask));
                                }
                                else
                                {
                                    final_mask = celeste_mask;
                                };
                            };
                            // scale to greyscale
                            TRACE_IMG("Convert to greyscale double...");
                            final_img = new vigra::DImage(scaled->size());
                            vigra::copyImage(vigra::srcImageRange(*scaled, vigra::RGBToGrayAccessor<vigra::RGBValue<vigra::UInt8> >()),
                                vigra::destImage(*final_img));
                            delete scaled;
                        };
                        break;
                    case vigra::ImageImportInfo::UINT16:
                        // special variant for unsigned 16 bit images
                        {
                            vigra::UInt16RGBImage* rgbImage = new vigra::UInt16RGBImage(aImageInfo.size());
                            vigra::BImage* mask = NULL;
                            // load image
                            if (aImageInfo.numExtraBands() == 1)
                            {
                                mask = new vigra::BImage(aImageInfo.size());
                                vigra::importImageAlpha(aImageInfo, vigra::destImage(*rgbImage), vigra::destImage(*mask));
                            }
                            else
                            {
                                vigra::importImage(aImageInfo, vigra::destImage(*rgbImage));
                            };
                            // apply icc profile
                            if (!aImageInfo.getICCProfile().empty())
                            {
                                TRACE_IMG("Applying icc profile...");
                                HuginBase::Color::ApplyICCProfile(*rgbImage, aImageInfo.getICCProfile(), TYPE_RGB_16);
                            };
                            vigra::UInt16RGBImage* scaled = NULL;
                            if (ioImgInfo._needsremap)
                            {
                                // remap image
                                TRACE_IMG("Remapping image...");
                                RemapImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), ioImgInfo._projOpts,
                                    ioImgInfo._detectWidth, ioImgInfo._detectHeight, rgbImage, mask,
                                    vigra_ext::PassThroughFunctor<vigra::RGBValue<vigra::UInt16> >(),
                                    scaled, final_mask);
                            }
                            else
                            {
                                if (iPanoDetector._downscale)
                                {
                                    TRACE_IMG("Downscale image...");
                                };
                                HandleDownscaleImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), rgbImage, mask,
                                    ioImgInfo._detectWidth, ioImgInfo._detectHeight, iPanoDetector._downscale,
                                    scaled, final_mask);
                            };
                            if (iPanoDetector.getCeleste())
                            {
                                TRACE_IMG("Mask areas with clouds...");
                                vigra::BImage* celeste_mask = celeste::getCelesteMask(iPanoDetector.svmModel, *scaled, radius, iPanoDetector.getCelesteThreshold(), 800, true, false);
#ifdef DEBUG_LOADING_REMAPPING
                                // DEBUG: export celeste mask
                                std::ostringstream maskfilename;
                                maskfilename << ioImgInfo._name << "_celeste_mask.JPG";
                                vigra::ImageExportInfo maskexinfo(maskfilename.str().c_str());
                                vigra::exportImage(vigra::srcImageRange(*celeste_mask), maskexinfo);
#endif
                                if (final_mask)
                                {
                                    vigra::copyImageIf(vigra::srcImageRange(*celeste_mask), vigra::srcImage(*final_mask), vigra::destImage(*final_mask));
                                }
                                else
                                {
                                    final_mask = celeste_mask;
                                };
                            };
                            // scale to greyscale
                            TRACE_IMG("Convert to greyscale double...");
                            final_img = new vigra::DImage(scaled->size());
                            // keypoint finder expext 0..255 range
                            vigra::transformImage(vigra::srcImageRange(*scaled, vigra::RGBToGrayAccessor<vigra::RGBValue<vigra::UInt16> >()),
                                vigra::destImage(*final_img), vigra::functor::Arg1() / vigra::functor::Param(255.0));
                            delete scaled;
                        };
                        break;
                    default:
                        // double variant for all other cases
                        {
                            vigra::DRGBImage* rgbImage = new vigra::DRGBImage(aImageInfo.size());
                            vigra::BImage* mask = NULL;
                            // load image
                            if (aImageInfo.numExtraBands() == 1)
                            {
                                mask = new vigra::BImage(aImageInfo.size());
                                vigra::importImageAlpha(aImageInfo, vigra::destImage(*rgbImage), vigra::destImage(*mask));
                            }
                            else
                            {
                                vigra::importImage(aImageInfo, vigra::destImage(*rgbImage));
                            };
                            // range adaption
                            double minVal = 0;
                            double maxVal;
                            const bool isDouble = aImageInfo.getPixelType() == std::string("FLOAT") || aImageInfo.getPixelType() == std::string("DOUBLE");
                            if (isDouble)
                            {
                                vigra::FindMinMax<float> minmax;   // init functor
                                vigra::inspectImage(vigra::srcImageRange(*rgbImage, vigra::RGBToGrayAccessor<vigra::RGBValue<double> >()), minmax);
                                minVal = minmax.min;
                                maxVal = minmax.max;
                            }
                            else
                            {
                                maxVal = vigra_ext::getMaxValForPixelType(aImageInfo.getPixelType());
                            };
                            bool range255 = (fabs(maxVal - 255) < 0.01 && fabs(minVal) < 0.01);
                            if (aImageInfo.getICCProfile().empty())
                            {
                                // no icc profile, cpfind expects images in 0 ..255 range
                                TRACE_IMG("Rescale range...");
                                if (!range255)
                                {
                                    int mapping = 0;
                                    if (isDouble && iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number).getResponseType() == HuginBase::BaseSrcPanoImage::RESPONSE_LINEAR)
                                    {
                                        // switch to log mapping for double/float images with linear response type
                                        mapping = 1;
                                    };
                                    vigra_ext::applyMapping(vigra::srcImageRange(*rgbImage), vigra::destImage(*rgbImage), minVal, maxVal, mapping);
                                };
                                range255 = true;
                            }
                            else
                            {
                                // apply ICC profile
                                TRACE_IMG("Applying icc profile...");
                                // lcms expects for double datatype all values between 0 and 1
                                vigra::transformImage(vigra::srcImageRange(*rgbImage), vigra::destImage(*rgbImage),
                                    vigra_ext::LinearTransform<vigra::RGBValue<double> >(1.0 / maxVal - minVal, -minVal));
                                range255 = false;
                                HuginBase::Color::ApplyICCProfile(*rgbImage, aImageInfo.getICCProfile(), TYPE_RGB_DBL);
                            };
                            vigra::DRGBImage* scaled;
                            if (ioImgInfo._needsremap)
                            {
                                // remap image
                                TRACE_IMG("Remapping image...");
                                RemapImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), ioImgInfo._projOpts,
                                    ioImgInfo._detectWidth, ioImgInfo._detectHeight, rgbImage, mask, vigra_ext::PassThroughFunctor<double>(),
                                    scaled, final_mask);
                            }
                            else
                            {
                                TRACE_IMG("Transform to suitable grayscale...");
                                HandleDownscaleImage(iPanoDetector._panoramaInfoCopy.getImage(ioImgInfo._number), rgbImage, mask,
                                    ioImgInfo._detectWidth, ioImgInfo._detectHeight, iPanoDetector._downscale,
                                    scaled, final_mask);
                            };
                            if (iPanoDetector.getCeleste())
                            {
                                TRACE_IMG("Mask areas with clouds...");
                                vigra::UInt16RGBImage* image16 = new vigra::UInt16RGBImage(scaled->size());
                                if (range255)
                                {
                                    vigra::transformImage(vigra::srcImageRange(*scaled), vigra::destImage(*image16),
                                        vigra::linearIntensityTransform<vigra::RGBValue<vigra::UInt16> >(255));
                                }
                                else
                                {
                                    vigra::transformImage(vigra::srcImageRange(*scaled), vigra::destImage(*image16),
                                        vigra::linearIntensityTransform<vigra::RGBValue<vigra::UInt16> >(65535));
                                };
                                vigra::BImage* celeste_mask = celeste::getCelesteMask(iPanoDetector.svmModel, *image16, radius, iPanoDetector.getCelesteThreshold(), 800, true, false);
#ifdef DEBUG_LOADING_REMAPPING
                                // DEBUG: export celeste mask
                                std::ostringstream maskfilename;
                                maskfilename << ioImgInfo._name << "_celeste_mask.JPG";
                                vigra::ImageExportInfo maskexinfo(maskfilename.str().c_str());
                                vigra::exportImage(vigra::srcImageRange(*celeste_mask), maskexinfo);
#endif
                                delete image16;
                                if (final_mask)
                                {
                                    vigra::copyImageIf(vigra::srcImageRange(*celeste_mask), vigra::srcImage(*final_mask), vigra::destImage(*final_mask));
                                }
                                else
                                {
                                    final_mask = celeste_mask;
                                };
                            };
                            // scale to greyscale
                            TRACE_IMG("Convert to greyscale double...");
                            final_img = new vigra::DImage(scaled->size());
                            // keypoint finder expext 0..255 range
                            if (range255)
                            {
                                vigra::copyImage(vigra::srcImageRange(*scaled, vigra::RGBToGrayAccessor<vigra::RGBValue<double> >()), vigra::destImage(*final_img));
                            }
                            else
                            {
                                vigra::transformImage(vigra::srcImageRange(*scaled, vigra::RGBToGrayAccessor<vigra::RGBValue<double> >()),
                                    vigra::destImage(*final_img), vigra::functor::Arg1() * vigra::functor::Param(255.0));
                            };
                            delete scaled;
                        };
                        break;
                };
            }
            else
            {
                TRACE_INFO("Cpfind works only with grayscale or RGB images");
                ioImgInfo._loadFail = true;
                return false;
            };
        };

#ifdef DEBUG_LOADING_REMAPPING
        // DEBUG: export remapped
        std::ostringstream filename;
        filename << ioImgInfo._name << "_grey.JPG";
        vigra::ImageExportInfo exinfo(filename.str().c_str());
        vigra::exportImage(vigra::srcImageRange(*final_img), exinfo);
#endif

        // Build integral image
        TRACE_IMG("Build integral image...");
        ioImgInfo._ii.init(*final_img);
        delete final_img;

        // compute distance map
        if(final_mask)
        {
            TRACE_IMG("Build distance map...");
            //apply threshold, in case loaded mask contains other values than 0 and 255
            vigra::transformImage(vigra::srcImageRange(*final_mask), vigra::destImage(*final_mask),
                                  vigra::Threshold<vigra::BImage::PixelType, vigra::BImage::PixelType>(1, 255, 0, 255));
            ioImgInfo._distancemap.resize(final_mask->width(), final_mask->height(), 0);
            vigra::distanceTransform(vigra::srcImageRange(*final_mask), vigra::destImage(ioImgInfo._distancemap), 255, 2);
#ifdef DEBUG_LOADING_REMAPPING
            std::ostringstream maskfilename;
            maskfilename << ioImgInfo._name << "_mask.JPG";
            vigra::ImageExportInfo maskexinfo(maskfilename.str().c_str());
            vigra::exportImage(vigra::srcImageRange(*final_mask), maskexinfo);
            std::ostringstream distfilename;
            distfilename << ioImgInfo._name << "_distancemap.JPG";
            vigra::ImageExportInfo distexinfo(distfilename.str().c_str());
            vigra::exportImage(vigra::srcImageRange(ioImgInfo._distancemap), distexinfo);
#endif
            delete final_mask;
        };
    }
    catch (std::exception& e)
    {
        TRACE_INFO("An error happened while loading image : caught exception: " << e.what() << std::endl);
        ioImgInfo._loadFail=true;
        return false;
    }

    return true;
}


bool PanoDetector::FindKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Find keypoints...");

    // setup the detector
    KeyPointDetector aKP;

    // detect the keypoints
    KeyPointVectInsertor aInsertor(ioImgInfo._kp);
    aKP.detectKeypoints(ioImgInfo._ii, aInsertor);

    TRACE_IMG("Found "<< ioImgInfo._kp.size() << " interest points.");

    return true;
}

bool PanoDetector::FilterKeyPointsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Filtering keypoints...");

    lfeat::Sieve<lfeat::KeyPointPtr, lfeat::KeyPointPtrSort > aSieve(iPanoDetector.getSieve1Width(),
            iPanoDetector.getSieve1Height(),
            iPanoDetector.getSieve1Size());

    // insert the points in the Sieve if they are not masked
    double aXF = (double)iPanoDetector.getSieve1Width() / (double)ioImgInfo._detectWidth;
    double aYF = (double)iPanoDetector.getSieve1Height() / (double)ioImgInfo._detectHeight;

    const bool distmap_valid=(ioImgInfo._distancemap.width()>0 && ioImgInfo._distancemap.height()>0);
    for (size_t i = 0; i < ioImgInfo._kp.size(); ++i)
    {
        lfeat::KeyPointPtr& aK = ioImgInfo._kp[i];
        if(distmap_valid)
        {
            if(aK->_x > 0 && aK->_x < ioImgInfo._distancemap.width() && aK->_y > 0 && aK->_y < ioImgInfo._distancemap.height()
                    && ioImgInfo._distancemap((int)(aK->_x),(int)(aK->_y)) >aK->_scale*8)
            {
                //cout << " dist from border:" << ioImgInfo._distancemap((int)(aK->_x),(int)(aK->_y)) << " required dist: " << aK->_scale*12 << std::endl;
                aSieve.insert(aK, (int)(aK->_x * aXF), (int)(aK->_y * aYF));
            }
        }
        else
        {
            aSieve.insert(aK, (int)(aK->_x * aXF), (int)(aK->_y * aYF));
        };
    }

    // pull remaining values from the sieve
    ioImgInfo._kp.clear();

    // make an extractor and pull the points
    SieveExtractorKP aSieveExt(ioImgInfo._kp);
    aSieve.extract(aSieveExt);

    TRACE_IMG("Kept " << ioImgInfo._kp.size() << " interest points.");

    return true;

}

bool PanoDetector::MakeKeyPointDescriptorsInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Make keypoint descriptors...");

    // build a keypoint descriptor
    lfeat::CircularKeyPointDescriptor aKPD(ioImgInfo._ii);

    // vector for keypoints with more than one orientation
    lfeat::KeyPointVect_t kp_new_ori;
    for (size_t j = 0; j < ioImgInfo._kp.size(); ++j)
    {
        lfeat::KeyPointPtr& aK = ioImgInfo._kp[j];
        double angles[4];
        int nAngles = aKPD.assignOrientation(*aK, angles);
        for (int i=0; i < nAngles; i++)
        {
            // duplicate Keypoint with additional angles
            lfeat::KeyPointPtr aKn = lfeat::KeyPointPtr ( new lfeat::KeyPoint ( *aK ) );
            aKn->_ori = angles[i];
            kp_new_ori.push_back(aKn);
        }
    }
    ioImgInfo._kp.insert(ioImgInfo._kp.end(), kp_new_ori.begin(), kp_new_ori.end());

    for (size_t i = 0; i < ioImgInfo._kp.size(); ++i)
    {
        aKPD.makeDescriptor(*(ioImgInfo._kp[i]));
    }
    // store the descriptor length
    ioImgInfo._descLength = aKPD.getDescriptorLength();
    return true;
}

bool PanoDetector::RemapBackKeypoints(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{

    double scale=iPanoDetector._downscale ? 2.0:1.0;

    if (!ioImgInfo._needsremap)
    {
        if(scale != 1.0)
        {
            for (size_t i = 0; i < ioImgInfo._kp.size(); ++i)
            {
                lfeat::KeyPointPtr& aK = ioImgInfo._kp[i];
                aK->_x *= scale;
                aK->_y *= scale;
                aK->_scale *= scale;
            }
        };
    }
    else
    {
        TRACE_IMG("Remapping back keypoints...");
        HuginBase::PTools::Transform trafo1;
        trafo1.createTransform(iPanoDetector._panoramaInfoCopy.getSrcImage(ioImgInfo._number),
                               ioImgInfo._projOpts);

        int dx1 = ioImgInfo._projOpts.getROI().left();
        int dy1 = ioImgInfo._projOpts.getROI().top();

        for (size_t i = 0; i < ioImgInfo._kp.size(); ++i)
        {
            lfeat::KeyPointPtr& aK = ioImgInfo._kp[i];
            double xout, yout;
            if(trafo1.transformImgCoord(xout, yout, aK->_x + dx1, aK->_y+ dy1))
            {
                // downscaling is take care of by the remapping transform
                // no need for multiplying the scale factor...
                aK->_x=xout;
                aK->_y=yout;
                aK->_scale *= scale;
            }
        }
    }
    return true;
}

bool PanoDetector::BuildKDTreesInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Build KDTree...");

    if(ioImgInfo._kp.size()==0)
    {
        return false;
    };
    // build a vector of KDElemKeyPointPtr

    // create feature vector matrix for flann
    ioImgInfo._flann_descriptors = flann::Matrix<double>(new double[ioImgInfo._kp.size()*ioImgInfo._descLength],
                                   ioImgInfo._kp.size(), ioImgInfo._descLength);
    for (size_t i = 0; i < ioImgInfo._kp.size(); ++i)
    {
        memcpy(ioImgInfo._flann_descriptors[i], ioImgInfo._kp[i]->_vec, sizeof(double)*ioImgInfo._descLength);
    }

    // build query structure
    ioImgInfo._flann_index = new flann::Index<flann::L2<double> > (ioImgInfo._flann_descriptors, flann::KDTreeIndexParams(4));
    ioImgInfo._flann_index->buildIndex();

    return true;
}

bool PanoDetector::FreeMemoryInImage(ImgData& ioImgInfo, const PanoDetector& iPanoDetector)
{
    TRACE_IMG("Freeing memory...");

    ioImgInfo._ii.clean();
    ioImgInfo._distancemap.resize(0,0);

    return true;
}


bool PanoDetector::FindMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("Find Matches...");

    // retrieve the KDTree of image 2
    flann::Index<flann::L2<double> > * index2 = ioMatchData._i2->_flann_index;

    // retrieve query points from image 1
    flann::Matrix<double> & query = ioMatchData._i1->_flann_descriptors;

    // storage for sorted 2 best matches
    int nn = 2;
    flann::Matrix<int> indices(new int[query.rows*nn], query.rows, nn);
    flann::Matrix<double> dists(new double[query.rows*nn], query.rows, nn);

    // perform matching using flann
    index2->knnSearch(query, indices, dists, nn, flann::SearchParams(iPanoDetector.getKDTreeSearchSteps()));

    //typedef KDTreeSpace::BestMatch<KDElemKeyPoint>		BM_t;
    //std::set<BM_t, std::greater<BM_t> >	aBestMatches;

    // store the matches already found to avoid 2 points in image1
    // match the same point in image2
    // both matches will be removed.
    std::set<int> aAlreadyMatched;
    std::set<int> aBadMatch;

    // unfiltered vector of matches
    typedef std::pair<lfeat::KeyPointPtr, int> TmpPair_t;
    std::vector<TmpPair_t>	aUnfilteredMatches;

    //PointMatchVector_t aMatches;

    // go through all the keypoints of image 1
    for (unsigned aKIt = 0; aKIt < query.rows; ++aKIt)
    {
        // accept the match if the second match is far enough
        // put a lower value for stronger matching default 0.15
        if (dists[aKIt][0] > iPanoDetector.getKDTreeSecondDistance()  * dists[aKIt][1])
        {
            continue;
        }

        // check if the kdtree match number is already in the already matched set
        if (aAlreadyMatched.find(indices[aKIt][0]) != aAlreadyMatched.end())
        {
            // add to delete list and continue
            aBadMatch.insert(indices[aKIt][0]);
            continue;
        }

        // TODO: add check for duplicate matches (can happen if a keypoint gets multiple orientations)

        // add the match number in already matched set
        aAlreadyMatched.insert(indices[aKIt][0]);

        // add the match to the unfiltered list
        aUnfilteredMatches.push_back(TmpPair_t(ioMatchData._i1->_kp[aKIt], indices[aKIt][0]));
    }

    // now filter and fill the vector of matches
    for (size_t i = 0; i < aUnfilteredMatches.size(); ++i)
    {
        TmpPair_t& aP = aUnfilteredMatches[i];
        // if the image2 match number is in the badmatch set, skip it.
        if (aBadMatch.find(aP.second) != aBadMatch.end())
        {
            continue;
        }

        // add the match in the output vector
        ioMatchData._matches.push_back(lfeat::PointMatchPtr( new lfeat::PointMatch(aP.first, ioMatchData._i2->_kp[aP.second])));
    }

    delete[] indices.ptr();
    delete[] dists.ptr();
    TRACE_PAIR("Found " << ioMatchData._matches.size() << " matches.");
    return true;
}

bool PanoDetector::RansacMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    // Use panotools model for wide angle lenses
    HuginBase::RANSACOptimizer::Mode rmode = iPanoDetector._ransacMode;
    if (rmode == HuginBase::RANSACOptimizer::HOMOGRAPHY ||
        (rmode == HuginBase::RANSACOptimizer::AUTO && iPanoDetector._panoramaInfo->getImage(ioMatchData._i1->_number).getHFOV() < 65 &&
             iPanoDetector._panoramaInfo->getImage(ioMatchData._i2->_number).getHFOV() < 65))
    {
        return RansacMatchesInPairHomography(ioMatchData, iPanoDetector);
    }
    else
    {
        return RansacMatchesInPairCam(ioMatchData, iPanoDetector);
    }
}

// new code with fisheye aware ransac
bool PanoDetector::RansacMatchesInPairCam(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("RANSAC Filtering with Panorama model...");

    if (ioMatchData._matches.size() < (unsigned int)iPanoDetector.getMinimumMatches())
    {
        TRACE_PAIR("Too few matches ... removing all of them.");
        ioMatchData._matches.clear();
        return true;
    }

    if (ioMatchData._matches.size() < 6)
    {
        TRACE_PAIR("Not enough matches for RANSAC filtering.");
        return true;
    }

    // setup a panorama project with the two images.
    // is this threadsafe (is this read only access?)
    HuginBase::UIntSet imgs;
    int pano_i1 = ioMatchData._i1->_number;
    int pano_i2 = ioMatchData._i2->_number;
    imgs.insert(pano_i1);
    imgs.insert(pano_i2);
    int pano_local_i1 = 0;
    int pano_local_i2 = 1;
    if (pano_i1 > pano_i2)
    {
        pano_local_i1 = 1;
        pano_local_i2 = 0;
    }

    // perform ransac matching.
    // ARGH the panotools optimizer uses global variables is not reentrant
    std::vector<int> inliers;
#pragma omp critical
    {
        HuginBase::PanoramaData* panoSubset = iPanoDetector._panoramaInfo->getNewSubset(imgs);

        // create control point vector
        HuginBase::CPVector controlPoints(ioMatchData._matches.size());
        for (size_t i = 0; i < ioMatchData._matches.size(); ++i)
        {
            lfeat::PointMatchPtr& aM=ioMatchData._matches[i];
            controlPoints[i] = HuginBase::ControlPoint(pano_local_i1, aM->_img1_x, aM->_img1_y,
                                            pano_local_i2, aM->_img2_x, aM->_img2_y);
        }
        panoSubset->setCtrlPoints(controlPoints);


        PT_setProgressFcn(ptProgress);
        PT_setInfoDlgFcn(ptinfoDlg);

        HuginBase::RANSACOptimizer::Mode rmode = iPanoDetector._ransacMode;
        if (rmode == HuginBase::RANSACOptimizer::AUTO)
        {
            rmode = HuginBase::RANSACOptimizer::RPY;
        }
        inliers = HuginBase::RANSACOptimizer::findInliers(*panoSubset, pano_local_i1, pano_local_i2,
                  iPanoDetector.getRansacDistanceThreshold(), rmode);
        PT_setProgressFcn(NULL);
        PT_setInfoDlgFcn(NULL);
        delete panoSubset;
    }

    TRACE_PAIR("Removed " << ioMatchData._matches.size() - inliers.size() << " matches. " << inliers.size() << " remaining.");
    if (inliers.size() < 0.5 * ioMatchData._matches.size())
    {
        // more than 50% of matches were removed, ignore complete pair...
        TRACE_PAIR("RANSAC found more than 50% outliers, removing all matches");
        ioMatchData._matches.clear();
        return true;
    }


    if (inliers.size() < (unsigned int)iPanoDetector.getMinimumMatches())
    {
        TRACE_PAIR("Too few matches ... removing all of them.");
        ioMatchData._matches.clear();
        return true;
    }

    // keep only inlier matches
    lfeat::PointMatchVector_t aInlierMatches;
    aInlierMatches.reserve(inliers.size());

    for (size_t i = 0; i < inliers.size(); ++i)
    {
        aInlierMatches.push_back(ioMatchData._matches[inliers[i]]);
    }
    ioMatchData._matches = aInlierMatches;

    /*
    if (iPanoDetector.getTest())
    	TestCode::drawRansacMatches(ioMatchData._i1->_name, ioMatchData._i2->_name, ioMatchData._matches,
    								aRemovedMatches, aRansacFilter, iPanoDetector.getDownscale());
    */

    return true;
}

// homography based ransac matching
bool PanoDetector::RansacMatchesInPairHomography(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("RANSAC Filtering...");

    if (ioMatchData._matches.size() < (unsigned int)iPanoDetector.getMinimumMatches())
    {
        TRACE_PAIR("Too few matches ... removing all of them.");
        ioMatchData._matches.clear();
        return true;
    }

    if (ioMatchData._matches.size() < 6)
    {
        TRACE_PAIR("Not enough matches for RANSAC filtering.");
        return true;
    }

    lfeat::PointMatchVector_t aRemovedMatches;

    lfeat::Ransac aRansacFilter;
    aRansacFilter.setIterations(iPanoDetector.getRansacIterations());
    int thresholdDistance=iPanoDetector.getRansacDistanceThreshold();
    //increase RANSAC distance if the image were remapped to not exclude
    //too much points in this case
    if(ioMatchData._i1->_needsremap || ioMatchData._i2->_needsremap)
    {
        thresholdDistance*=5;
    }
    aRansacFilter.setDistanceThreshold(thresholdDistance);
    aRansacFilter.filter(ioMatchData._matches, aRemovedMatches);


    TRACE_PAIR("Removed " << aRemovedMatches.size() << " matches. " << ioMatchData._matches.size() << " remaining.");

    if (aRemovedMatches.size() > ioMatchData._matches.size())
    {
        // more than 50% of matches were removed, ignore complete pair...
        TRACE_PAIR("More than 50% outliers, removing all matches");
        ioMatchData._matches.clear();
        return true;
    }

    if (iPanoDetector.getTest())
        TestCode::drawRansacMatches(ioMatchData._i1->_name, ioMatchData._i2->_name, ioMatchData._matches,
                                    aRemovedMatches, aRansacFilter, iPanoDetector.getDownscale());

    return true;

}


bool PanoDetector::FilterMatchesInPair(MatchData& ioMatchData, const PanoDetector& iPanoDetector)
{
    TRACE_PAIR("Clustering matches...");

    if (ioMatchData._matches.size() < 2)
    {
        return true;
    }

    // compute min,max of x,y for image1

    double aMinX = std::numeric_limits<double>::max();
    double aMinY = std::numeric_limits<double>::max();
    double aMaxX = -std::numeric_limits<double>::max();
    double aMaxY = -std::numeric_limits<double>::max();

    for (size_t i = 0; i < ioMatchData._matches.size(); ++i)
    {
        lfeat::PointMatchPtr& aM = ioMatchData._matches[i];
        if (aM->_img1_x < aMinX)
        {
            aMinX = aM->_img1_x;
        }
        if (aM->_img1_x > aMaxX)
        {
            aMaxX = aM->_img1_x;
        }

        if (aM->_img1_y < aMinY)
        {
            aMinY = aM->_img1_y;
        }
        if (aM->_img1_y > aMaxY)
        {
            aMaxY = aM->_img1_y;
        }
    }

    double aSizeX = aMaxX - aMinX + 2; // add 2 so max/aSize is strict < 1
    double aSizeY = aMaxY - aMinY + 2;

    //

    lfeat::Sieve<lfeat::PointMatchPtr, lfeat::PointMatchPtrSort> aSieve(iPanoDetector.getSieve2Width(),
            iPanoDetector.getSieve2Height(),
            iPanoDetector.getSieve2Size());

    // insert the points in the Sieve
    double aXF = (double)iPanoDetector.getSieve2Width() / aSizeX;
    double aYF = (double)iPanoDetector.getSieve2Height() / aSizeY;
    for (size_t i = 0; i < ioMatchData._matches.size(); ++i)
    {
        lfeat::PointMatchPtr& aM = ioMatchData._matches[i];
        aSieve.insert(aM, (int)((aM->_img1_x - aMinX) * aXF), (int)((aM->_img1_y - aMinY) * aYF));
    }

    // pull remaining values from the sieve
    ioMatchData._matches.clear();

    // make an extractor and pull the points
    SieveExtractorMatch aSieveExt(ioMatchData._matches);
    aSieve.extract(aSieveExt);

    TRACE_PAIR("Kept " << ioMatchData._matches.size() << " matches.");
    return true;
}

void PanoDetector::writeOutput()
{
    // Write output pto file

    std::ofstream aOut(_outputFile.c_str(), std::ios_base::trunc);
    if( !aOut )
    {
        std::cerr << "ERROR : "
             << "Couldn't open file '" << _outputFile << "'!" << std::endl; //STS
        return;
    }

    aOut << "# pto project file generated by Hugin's cpfind" << std::endl << std::endl;

    _panoramaInfo->removeDuplicateCtrlPoints();
    AppBase::DocumentData::ReadWriteError err = _panoramaInfo->writeData(aOut);
    if (err != AppBase::DocumentData::SUCCESSFUL)
    {
        std::cerr << "ERROR couldn't write to output file '" << _outputFile << "'!" << std::endl;
        return;
    }
}

void PanoDetector::writeKeyfile(ImgData& imgInfo)
{
    // Write output keyfile

    std::ofstream aOut(imgInfo._keyfilename.c_str(), std::ios_base::trunc);

    lfeat::SIFTFormatWriter writer(aOut);

    int origImgWidth =  _panoramaInfo->getImage(imgInfo._number).getSize().width();
    int origImgHeight =  _panoramaInfo->getImage(imgInfo._number).getSize().height();

    lfeat::ImageInfo img_info(imgInfo._name, origImgWidth, origImgHeight);

    writer.writeHeader ( img_info, imgInfo._kp.size(), imgInfo._descLength );

    for(size_t i=0; i<imgInfo._kp.size(); ++i)
    {
        lfeat::KeyPointPtr& aK=imgInfo._kp[i];
        writer.writeKeypoint ( aK->_x, aK->_y, aK->_scale, aK->_ori, aK->_score,
                               imgInfo._descLength, aK->_vec );
    }
    writer.writeFooter();
}


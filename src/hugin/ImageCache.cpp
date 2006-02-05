// -*- c-basic-offset: 4 -*-

/** @file ImageCache.cpp
 *
 *  @brief implementation of ImageCache Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#include <config.h>
#include "panoinc_WX.h"

#include "panoinc.h"

#include <fstream>

#include <vigra/basicimage.hxx>
#include <vigra/basicimageview.hxx>
#include <vigra/rgbvalue.hxx>
#include <vigra/impex.hxx>
#include <vigra/impexalpha.hxx>
#include <vigra_ext/Pyramid.h>
#include <vigra_ext/ImageTransforms.h>

#include <PT/Stitcher.h>

#include "hugin/ImageCache.h"

using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace utils;
using namespace PT;

ImageCache * ImageCache::instance = 0;

ImageCache::ImageCache()
    : m_progress(0)
{
}

ImageCache::~ImageCache()
{
    images.clear();
//    delete instance;
    instance = 0;
}

// blubber

void ImageCache::removeImage(const std::string & filename)
{
    map<string, wxImage*>::iterator it = images.find(filename);
    if (it != images.end()) {
        delete it->second;
        images.erase(it);
    }

    string sfilename = filename + string("_small");
    it = images.find(sfilename);
    if (it != images.end()) {
        delete it->second;
        images.erase(it);
    }

    int level = 0;
    bool found = true;
    do {
        // found. xyz
        PyramidKey key(filename,level);
        map<string, vigra::BImage*>::iterator it = pyrImages.find(key.toString());
        found = (it != pyrImages.end());
        if (found) {
            delete it->second;
            pyrImages.erase(it);
        }
        level++;
    } while (found);
}


void ImageCache::flush()
{
    for (map<string, ImagePtr>::iterator it = images.begin();
         it != images.end();
         ++it)
    {
        delete it->second;
    }
    images.clear();

    for (map<string, vigra::BImage*>::iterator it = pyrImages.begin();
         it != pyrImages.end();
         ++it)
    {
        delete it->second;
    }
    pyrImages.clear();
}

void ImageCache::softFlush()
{
    long upperBound = wxConfigBase::Get()->Read(wxT("/ImageCache/UpperBound"), 75 * 1024 * 1024l);
    long purgeToSize = upperBound/2;

    // calculate used memory
    long imgMem = 0;

    std::map<std::string, ImagePtr>::iterator imgIt;
    for(imgIt=images.begin(); imgIt != images.end(); imgIt++) {
        imgMem += imgIt->second->GetWidth() * imgIt->second->GetHeight() * 3;
    }

    long pyrMem = 0;
    std::map<std::string, BImage*>::iterator pyrIt;
    for(pyrIt=pyrImages.begin(); pyrIt != pyrImages.end(); pyrIt++) {
        pyrMem += pyrIt->second->width() * pyrIt->second->height();
    }

    long usedMem = imgMem + pyrMem;

    DEBUG_DEBUG("total: " << (usedMem>>20) << " MB upper bound: " << (purgeToSize>>20) << " MB");

    if (usedMem > upperBound) {
        // we need to remove images.
        long purgeAmount = usedMem - purgeToSize;
        long purgedMem = 0;
        // remove images from cache, first the grey level image,
        // then the full size images
        while (purgeAmount > purgedMem) {
            bool deleted = false;
            if (pyrImages.size() > 0) {
                BImage * imgPtr = (*(pyrImages.begin())).second;
                purgedMem += imgPtr->width() * imgPtr->height();
                delete imgPtr;
                pyrImages.erase(pyrImages.begin());
                deleted = true;
            } else if (images.size() > 0) {
                // only remove full size images.
                for (map<string, ImagePtr>::iterator it = images.begin();
                     it != images.end();
                     ++it)
                {
                    if (it->first.substr(it->first.size()-6) != "_small") {
                        purgedMem += it->second->GetWidth() * it->second->GetHeight() * 3;
                        delete it->second;
                        images.erase(it);
                        deleted = true;
                        break;
                    }
                }
            }
            if (!deleted) {
                DEBUG_WARN("Purged all not preview images, but ImageCache still to big");
                break;
            }
        }
//        m_progress->progressMessage(
//            wxString::Format(_("Purged %d MB from image cache. Current cache usage: %d MB"),
//                             purgedMem>>20, (usedMem - purgedMem)>>20
//                ).c_str(),0);
        DEBUG_DEBUG("purged: " << (purgedMem>>20) << " MB, memory used for images: " << ((usedMem - purgedMem)>>20) << " MB");
    }
}

ImageCache & ImageCache::getInstance()
{
    if (!instance) {
        instance = new ImageCache();
    }
    return *instance;
}

template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertImage(const ImageImportInfo & info,
                           vigra::pair<DestIterator, DestAccessor> dest)
{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef typename SrcPixelType::value_type SrcComponentType;

    // load image into temporary buffer.
    BasicImage<SrcPixelType> tmp(info.width(), info.height());
    vigra::importImage(info, destImage(tmp));

    // convert to destination type
    typedef typename vigra::NumericTraits<SrcComponentType>::RealPromote ScaleType;

    ScaleType s = GetAlphaScaleFactor<SrcComponentType, DestComponentType>::get();
//    GetAlphaScaleFactor<float, unsigned char>::get();

    // get the scaling factor
    ScaleType scale = vigra::NumericTraits<DestComponentType>::one()/s;

    std::cerr << " import scale factor: " << scale << std::endl;

    // copy image to output
    transformImage(srcImageRange(tmp), dest,
                   linearIntensityTransform<DestPixelType>(scale));
}


template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertGrayImage(const ImageImportInfo & info,
                               vigra::pair<DestIterator, DestAccessor> dest)

{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef SrcPixelType SrcComponentType;

    // load image into temporary buffer.
    BasicImage<SrcPixelType> tmp(info.width(), info.height());
    vigra::importImage(info, destImage(tmp));

    // convert to destination type
    typedef typename vigra::NumericTraits<SrcComponentType>::RealPromote ScaleType;

    ScaleType s = GetAlphaScaleFactor<SrcComponentType, DestComponentType>::get();
//    GetAlphaScaleFactor<float, unsigned char>::get();

    // get the scaling factor
    ScaleType scale = vigra::NumericTraits<DestComponentType>::one()/s;

    std::cerr << " import scale factor: " << scale << std::endl;

    // copy image to output
    transformImage(srcImageRange(tmp),
                   make_pair(dest.first,
                             RedAccessor<DestPixelType>()),
                   linearIntensityTransform<DestComponentType>(scale));

    copyImage(dest.first, dest.first + tmp.size(),
              RedAccessor<DestPixelType>(),
              dest.first, GreenAccessor<DestPixelType>());

    copyImage(dest.first, dest.first + tmp.size(),
              RedAccessor<DestPixelType>(),
              dest.first, BlueAccessor<DestPixelType>());
}

template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertGrayAlphaImage(const ImageImportInfo & info,
                                    vigra::pair<DestIterator, DestAccessor> dest)
{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef typename SrcPixelType::value_type SrcComponentType;

    // load image into temporary buffer.
    BasicImage<SrcPixelType> tmp(info.width(), info.height());
    vigra::importImage(info, destImage(tmp));

    // convert to destination type
    typedef typename vigra::NumericTraits<SrcComponentType>::RealPromote ScaleType;

    ScaleType s = GetAlphaScaleFactor<SrcComponentType, DestComponentType>::get();
//    GetAlphaScaleFactor<float, unsigned char>::get();

    // get the scaling factor
    ScaleType scale = vigra::NumericTraits<DestComponentType>::one()/s;

    std::cerr << " import scale factor: " << scale << std::endl;

    // copy image to output
    transformImageIf(tmp.upperLeft(), tmp.lowerRight(),
                     VectorComponentAccessor<SrcPixelType>(0),
                     tmp.upperLeft(),
                     VectorComponentAccessor<SrcPixelType>(1),
                     dest.first, RedAccessor<DestPixelType>(),
                     linearIntensityTransform<DestComponentType>(scale));

    copyImage(dest.first, dest.first + tmp.size(),
              RedAccessor<DestPixelType>() ,
              dest.first, GreenAccessor<DestPixelType>());

    copyImage(dest.first, dest.first + tmp.size(),
              RedAccessor<DestPixelType>(),
              dest.first, BlueAccessor<DestPixelType>());
}


template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertAlphaImage(const ImageImportInfo & info,
                                vigra::pair<DestIterator, DestAccessor> dest)
{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef typename SrcPixelType::value_type SrcComponentType;

    // load image into temporary buffer.
    BasicImage<SrcPixelType> tmp(info.width(), info.height());
    vigra::importImage(info, destImage(tmp));

    tmp.upperLeft();

    // convert to destination type
    typedef typename vigra::NumericTraits<SrcComponentType>::RealPromote ScaleType;

    ScaleType s = GetAlphaScaleFactor<SrcComponentType, DestComponentType>::get();
//    GetAlphaScaleFactor<float, unsigned char>::get();

    // get the scaling factor
    ScaleType scale = vigra::NumericTraits<DestComponentType>::one()/s;

    std::cerr << " import scale factor: " << scale << std::endl;

    // copy image to output
    vigra::transformImageIf(tmp.upperLeft(),
                            tmp.lowerRight(),
                            VectorComponentAccessor<SrcPixelType>(0),

                            tmp.upperLeft(),
                            VectorComponentAccessor<SrcPixelType>(3),

                            dest.first, RedAccessor<DestPixelType>(),
                            linearIntensityTransform<DestComponentType>(scale));

    transformImageIf(tmp.upperLeft(), tmp.lowerRight(),
                     VectorComponentAccessor<SrcPixelType>(1),
                     tmp.upperLeft(),
                     VectorComponentAccessor<SrcPixelType>(3),
                     dest.first,
                     GreenAccessor<DestPixelType>(),
                     linearIntensityTransform<DestComponentType>(scale));

    transformImageIf(tmp.upperLeft(), tmp.lowerRight(),
                     VectorComponentAccessor<SrcPixelType>(2),
                     tmp.upperLeft(),
                     VectorComponentAccessor<SrcPixelType>(3),
                     dest.first,
                     BlueAccessor<DestPixelType>(),
                     linearIntensityTransform<DestComponentType>(scale));
}

ImagePtr ImageCache::getImage(const std::string & filename)
{
//    softFlush();

    std::map<std::string, wxImage *>::iterator it;
    it = images.find(filename);
    if (it != images.end()) {
        return it->second;
    } else {
        if (m_progress) {
            m_progress->pushTask(ProgressTask((const char *)wxString::Format(_("Loading image %s"),wxString(utils::stripPath(filename).c_str(), *wxConvCurrent).c_str()).mb_str(), "", 0));
        }
#if 1
        // load images with VIGRA impex, and scale to 8 bit
        wxImage * image;
        try {
            ImageImportInfo info(filename.c_str());

            image = new wxImage(info.width(), info.height());

            BasicImageView<RGBValue<unsigned char> > imgview((RGBValue<unsigned char> *)image->GetData(),
                image->GetWidth(),
                image->GetHeight());

            int bands = info.numBands();
            int extraBands = info.numExtraBands();
            const char * pixelType = info.getPixelType();


            if ( bands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    importImage(info, destImage(imgview));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertGrayImage<short> (info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertGrayImage<unsigned short >(info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertGrayImage<unsigned int>(info, destImage(imgview));
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertGrayImage<int>(info, destImage(imgview));
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertGrayImage<float>(info, destImage(imgview));
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertGrayImage<double>(info, destImage(imgview));
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if (bands == 3 && extraBands == 0) {
                DEBUG_DEBUG( pixelType);
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::importImage(info, destImage(imgview));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertImage<RGBValue<short> > (info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertImage<RGBValue<unsigned short> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertImage<RGBValue<unsigned int> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertImage<RGBValue<int> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertImage<RGBValue<float> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertImage<RGBValue<double> >(info, destImage(imgview));
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if ( bands == 4 && extraBands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    importAndConvertAlphaImage<TinyVector<unsigned char, 4> > (info, destImage(imgview));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertAlphaImage<TinyVector<short, 4> > (info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertAlphaImage<TinyVector<unsigned short, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertAlphaImage<TinyVector<unsigned int, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertAlphaImage<TinyVector<int, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertAlphaImage<TinyVector<float, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertAlphaImage<TinyVector<double, 4> >(info, destImage(imgview));
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if ( bands == 2 && extraBands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    importAndConvertGrayAlphaImage<TinyVector<unsigned char, 4> > (info, destImage(imgview));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertGrayAlphaImage<TinyVector<short, 4> > (info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertGrayAlphaImage<TinyVector<unsigned short, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertGrayAlphaImage<TinyVector<unsigned int, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertGrayAlphaImage<TinyVector<int, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertGrayAlphaImage<TinyVector<float, 4> >(info, destImage(imgview));
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertGrayAlphaImage<TinyVector<double, 4> >(info, destImage(imgview));
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }

            } else {
                DEBUG_ERROR("unsupported depth, only images with 1 or 3 channel images are supported.");
            }
        } catch (vigra::PreconditionViolation & e) {
            // could not load image..
            DEBUG_DEBUG("Could not load image information: " << e.what());
            return new wxImage;
        }

#else
        // use wx for image loading
        wxImage * image = new wxImage(filename.c_str());
        if (!image->Ok()){
            wxMessageBox(_("Cannot load image: ") + wxString(filename.c_str()),
                         "Image Cache error");
        }
#endif
        if (m_progress) {
            m_progress->popTask();
        }
        images[filename] = image;
        return image;
    }
}

ImagePtr ImageCache::getSmallImage(const std::string & filename)
{
//    softFlush();
    std::map<std::string, wxImage *>::iterator it;
    // "_small" is only used internally
    string name = filename + string("_small");
    it = images.find(name);
    if (it != images.end()) {
        return it->second;
    } else {
        if (m_progress) {
            m_progress->pushTask(ProgressTask((const char *)wxString::Format(_("Scaling image %s"),wxString(utils::stripPath(filename).c_str(), *wxConvCurrent).c_str()).mb_str(), "", 0));
        }
        DEBUG_DEBUG("creating small image " << name );
        ImagePtr image = getImage(filename);
        if (image->Ok()) {
            wxImage small_image;
            const int w = 512;
            double ratio = (double)image->GetWidth() / image->GetHeight();
            small_image = image->Scale(w, (int) (w/ratio));

            wxImage * tmp = new wxImage( &small_image );
            images[name] = tmp;
            DEBUG_INFO ( "created small image: " << name);
            if (m_progress) {
                m_progress->popTask();
            }
            return tmp;
        } else {
            if (m_progress) {
                m_progress->popTask();
            }
            return image;
        }
    }
}

const vigra::BImage & ImageCache::getPyramidImage(const std::string & filename,
                                                  int level)
{
//    softFlush();
    DEBUG_TRACE(filename << " level:" << level);
    std::map<std::string, vigra::BImage *>::iterator it;
    PyramidKey key(filename,level);
    it = pyrImages.find(key.toString());
    if (it != pyrImages.end()) {
        DEBUG_DEBUG("pyramid image already in cache");
        return *(it->second);
    } else {
        // the image is not in cache.. go and create it.
        vigra::BImage * img = 0;
        for(int i=0; i<=level; i++) {
            key.level=i;
            DEBUG_DEBUG("loop level:" << key.level);
            it = pyrImages.find(key.toString());
            if (it != pyrImages.end()) {
                // image is already known
                DEBUG_DEBUG("level " << key.level << " already in cache");
                img = (it->second);
            } else {
                // we need to create this resolution step
                if (key.level == 0) {
                    // special case, create first gray image
                    wxImage * srcImg = getImage(filename);
                    img = new vigra::BImage(srcImg->GetWidth(), srcImg->GetHeight());
                    DEBUG_DEBUG("creating level 0 pyramid image for "<< filename);
                    if (m_progress) {
        	      m_progress->pushTask(ProgressTask((const char *)wxString::Format(_("Creating grayscale %s"),wxString(filename.c_str(), *wxConvCurrent).c_str()).mb_str(), "", 0));
                    }
                    BasicImageView<RGBValue<unsigned char> > src((RGBValue<unsigned char> *)srcImg->GetData(),
                                                                 srcImg->GetWidth(),
                                                                 srcImg->GetHeight());
                    vigra::copyImage(src.upperLeft(),
                                     src.lowerRight(),
                                     RGBToGrayAccessor<RGBValue<unsigned char> >(),
                                     img->upperLeft(),
                                     BImage::Accessor());
                    if (m_progress) {
                        m_progress->popTask();
                    }
                } else {
                    // reduce previous level to current level
                    DEBUG_DEBUG("reducing level " << key.level-1 << " to level " << key.level);
                    assert(img);
                    if (m_progress) {
                        m_progress->pushTask(ProgressTask((const char *)wxString::Format(_("Creating pyramid image for %s, level %d"),wxString(filename.c_str(), *wxConvCurrent).c_str(), key.level).mb_str(), "",0));
                    }
                    BImage *smallImg = new BImage();
                    reduceToNextLevel(*img, *smallImg);
                    img = smallImg;
                    if (m_progress) {
                        m_progress->popTask();
                    }
                }
                pyrImages[key.toString()]=img;
            }
        }
        // we have found our image
        return *img;
    }
}


SmallRemappedImageCache::~SmallRemappedImageCache()
{
    invalidate();
}

#if 0
MRemappedImage *
SmallRemappedImageCache::getRemapped(const std::string & filename,
                                     const vigra::Diff2D & origSrcSize,
                                     const vigra::Diff2D & srcSize,
                                     PT::VariableMap srcVars,
                                     PT::Lens::LensProjectionFormat srcProj,
                                     const PT::PanoImage & imgOpts,
                                     const vigra::Diff2D &destSize,
                                     PT::PanoramaOptions::ProjectionFormat destProj,
                                     double destHFOV,
                                     utils::MultiProgressDisplay & progress)
{
    // return old image, if already in cache
    if (set_contains(m_images, img.getFilename())) {
        DEBUG_DEBUG("using cached remapped image " << img.getFilename);
        return m_images[imgNr];
    }
    typedef  BasicImageView<RGBValue<unsigned char> > BRGBImageView;

    typedef vigra::NumericTraits<PixelType>::RealPromote RPixelType;

    // remap image
    DEBUG_DEBUG("remapping image " << imgNr);

    // load image
    const PanoImage & img = pano.getImage(imgNr);
    const PT::ImageOptions & iopts = img.getOptions();

    wxImage * src = ImageCache::getInstance().getSmallImage(img.getFilename().c_str());
    if (!src->Ok()) {
        throw std::runtime_error("could not retrieve small source image for preview generation");
    }

    // image view
    BRGBImageView srcImg((RGBValue<unsigned char> *)src->GetData(),
                          src->GetWidth(),
                          src->GetHeight());
    MRemappedImage *remapped = new MRemappedImage;
    // convert image to grayscale

    // no alpha channel...
    BImage srcAlpha;

    // flatfield image, if required.
    BImage ffImg;
    if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
        // load flatfield image.

        wxImage * flatsrc = ImageCache::getInstance().getSmallImage(iopts.m_flatfield.c_str());
        if (!flatsrc->Ok()) {
            throw std::runtime_error("could not retrieve flatfield image for preview generation");
        }

        // image view
        BRGBImageView flatImg((RGBValue<unsigned char> *)flatsrc->GetData(),
                               flatsrc->GetWidth(),
                               flatsrc->GetHeight());
        // convert flatfield to gray image.
        ffImg.resize(flatImg.size());
        copyImage(vigra::srcImageRange(flatImg, vigra::RGB2GrayAccessor()

            } else if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_RADIAL) {
                progress.setMessage(std::string("vignetting correction ") + utils::stripPath(img.getFilename()));
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

                vigra_ext::radialVigCorrection(srcImageRange(srcImg), destImage(srcCorrImg),
                                               opts.gamma, gMaxVal,
                                               radCoeff, cx, cy,
                                               vigCorrDivision, ka, kb, true);
            } else if (opts.gamma != 1.0 && doBrightnessCorrection ) {
                progress.setMessage(std::string("inverse gamma correction ") + utils::stripPath(img.getFilename()));
                vigra_ext::applyGammaAndBrightCorrection(srcImageRange(srcImg), destImage(srcCorrImg),
                        opts.gamma, gMaxVal, ka,kb);
            } else if (doBrightnessCorrection ) {
                progress.setMessage(std::string("brightness correction ") + utils::stripPath(img.getFilename()));
                vigra_ext::applyBrightnessCorrection(srcImageRange(srcImg), destImage(srcCorrImg),
                        ka,kb);
            } else if (opts.gamma != 1.0 ) {
                progress.setMessage(std::string("inverse gamma correction ") + utils::stripPath(img.getFilename()));
                vigra_ext::applyGammaCorrection(srcImageRange(srcImg), destImage(srcCorrImg),
                        opts.gamma, gMaxVal);
            }

            remapped->remapImage(pano, opts,
                                 srcImageRange(srcCorrImg),
                                 imgNr, progress);

            if (opts.gamma != 1.0) {
                progress.setMessage(std::string("gamma correction ") + utils::stripPath(img.getFilename()));
                vigra_ext::applyGammaCorrection(srcImageRange(remapped->m_image), destImage(remapped->m_image),
                        1/opts.gamma, gMaxVal);
            }
        } else {
            remapped->remapImage(pano, opts,
                                 srcImageRange(srcImg),
                                 imgNr, progress);
        }
        m_images[imgNr] = remapped;
        return remapped;

}
#endif

SmallRemappedImageCache::MRemappedImage *
SmallRemappedImageCache::getRemapped(const PT::Panorama & pano,
                                    const PT::PanoramaOptions & opts,
                                    unsigned int imgNr,
                                    utils::MultiProgressDisplay & progress)
{
    // return old image, if already in cache
    if (set_contains(m_images, imgNr)) {
        DEBUG_DEBUG("using cached remapped image " << imgNr);
        return m_images[imgNr];
    }

    typedef  BasicImageView<RGBValue<unsigned char> > BRGBImageView;

//    typedef vigra::NumericTraits<PixelType>::RealPromote RPixelType;

    // remap image
    DEBUG_DEBUG("remapping image " << imgNr);

    // load image
    const PanoImage & img = pano.getImage(imgNr);
    const PT::ImageOptions & iopts = img.getOptions();

    wxImage * src = ImageCache::getInstance().getSmallImage(img.getFilename().c_str());
    if (!src->Ok()) {
        throw std::runtime_error("could not retrieve small source image for preview generation");
    }
    // image view
    BasicImageView<RGBValue<unsigned char> > srcImgInCache((RGBValue<unsigned char> *)src->GetData(),
                                    src->GetWidth(),
                                    src->GetHeight());
    BRGBImage srcImg(srcImgInCache.size());
    vigra::copyImage(vigra::srcImageRange(srcImgInCache),
                     vigra::destImage(srcImg));

    MRemappedImage *remapped = new MRemappedImage;
    SrcPanoImage srcPanoImg = pano.getSrcImage(imgNr);
    // adjust distortion parameters for small preview image
    srcPanoImg.resize(srcImg.size());

    BImage srcFlat;
    // use complete image..
    BImage srcMask;

    if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
            wxImage * flatsrc = ImageCache::getInstance().getSmallImage(iopts.m_flatfield.c_str());
            if (!flatsrc->Ok()) {
                throw std::runtime_error("could not retrieve flatfield image for preview generation");
            }

            // image view
            BRGBImageView flatImgRGB((RGBValue<unsigned char> *)flatsrc->GetData(),
                                        flatsrc->GetWidth(),
                                        flatsrc->GetHeight());
            srcFlat.resize(flatImgRGB.size());
            vigra::copyImage(vigra::srcImageRange(flatImgRGB, vigra::RGBToGrayAccessor<RGBValue<unsigned char> >()),
                         vigra::destImage(srcFlat));
    }
    // remap image
    remapImage(srcImg,
               srcMask,
               srcFlat,
               srcPanoImg,
               opts.getDestImage(),
               opts.interpolator,
               *remapped,
               progress);

    m_images[imgNr] = remapped;
    return remapped;
}


void SmallRemappedImageCache::invalidate()
{
    DEBUG_DEBUG("Clear remapped cache");
    for(std::map<unsigned int, MRemappedImage*>::iterator it = m_images.begin();
        it != m_images.end(); ++it)
    {
        delete (*it).second;
    }
    // remove all images
    m_images.clear();
}

void SmallRemappedImageCache::invalidate(unsigned int imgNr)
{
    DEBUG_DEBUG("Remove " << imgNr << " from remapped cache");
    if (set_contains(m_images, imgNr)) {
        delete (m_images[imgNr]);
        m_images.erase(imgNr);
    }
}


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
#include <vigra_ext/utils.h>
#include <vigra_ext/impexalpha.hxx>
#include <vigra_ext/Pyramid.h>
#include <vigra_ext/ImageTransforms.h>
#include <vigra_ext/FunctorAccessor.h>
#include <PT/Stitcher.h>
#include <vigra/functorexpression.hxx>

#include "hugin/ImageCache.h"
#include "hugin/config_defaults.h"

#include <vigra/sized_int.hxx>

using namespace std;
using namespace vigra;
using namespace vigra_ext;
using namespace utils;
using namespace PT;
using namespace vigra::functor;


template <class T1>
struct GetRange;

// define the scale factors to map from the alpha channel type (T2)
// to valid alpha channels in image type (T1)
// T1: image type
// T2: alpha type
//  S: scale factor (given as type of T1).
#define VIGRA_EXT_GETRANGE(T1, MI,MA) \
template<> \
struct GetRange<T1> \
{ \
    static T1 min() \
{ \
        return MI; \
} \
    static T1 max() \
{ \
        return MA; \
} \
};


// conversion from/to unsigned char
VIGRA_EXT_GETRANGE(vigra::UInt8,  0, 255);
VIGRA_EXT_GETRANGE(vigra::Int16,  0, 32767);
VIGRA_EXT_GETRANGE(vigra::UInt16, 0, 65535);
VIGRA_EXT_GETRANGE(vigra::Int32,  0, 2147483647);
VIGRA_EXT_GETRANGE(vigra::UInt32, 0, 4294967295u);
VIGRA_EXT_GETRANGE(float,  0, 1.0f);
VIGRA_EXT_GETRANGE(double, 0, 1.0);

#undef VIGRA_EXT_GETRANGE

ImageCache * ImageCache::instance = 0;

ImageCache::ImageCache()
    : m_progress(0), m_accessCounter(0)
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
    map<string, Entry*>::iterator it = images.find(filename);
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
    for (map<string, Entry*>::iterator it = images.begin();
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
    long purgeToSize = long(0.75 * upperBound);

    // calculate used memory
    long imgMem = 0;

    std::map<std::string, Entry*>::iterator imgIt;
    for(imgIt=images.begin(); imgIt != images.end(); imgIt++) {
        if (imgIt->second->image8) {
            imgMem += imgIt->second->image8->width() * imgIt->second->image8->height() * 3;
        }
        if (imgIt->second->imageFloat) {
            imgMem += imgIt->second->imageFloat->width() * imgIt->second->imageFloat->height() * 3 * 4;
        }
    }

    long pyrMem = 0;
    std::map<std::string, BImage*>::iterator pyrIt;
    for(pyrIt=pyrImages.begin(); pyrIt != pyrImages.end(); pyrIt++) {
        pyrMem += pyrIt->second->width() * pyrIt->second->height();
    }

    long usedMem = imgMem + pyrMem;

    DEBUG_DEBUG("total: " << (usedMem>>20) << " MB upper bound: " << (purgeToSize>>20) << " MB");
    cout << "ImageCache::softFlush: normal: " << images.size() << "pyr: " << pyrImages.size() << ", memory: " << (usedMem>>20) << " MB upper bound: " << (purgeToSize>>20) << " MB" << endl;

    if (usedMem > upperBound) 
    {
        // we need to remove images.
        long purgeAmount = usedMem - purgeToSize;
        long purgedMem = 0;
        // remove images from cache, first the grey level image,
        // then the full size images

        // use least recently uses strategy.
        // sort images by their access time
        std::map<int,std::string> accessMap;
        for (map<string, Entry*>::iterator it = images.begin();
             it != images.end();
             ++it)
        {
            if (it->first.substr(it->first.size()-6) != ":small") {
                // only consider full images
                accessMap.insert(make_pair(it->second->lastAccess, it->first));
            }
        }
        while (purgeAmount > purgedMem) {
            bool deleted = false;
            if (pyrImages.size() > 0) {
                BImage * imgPtr = (*(pyrImages.begin())).second;
                purgedMem += imgPtr->width() * imgPtr->height();
                delete imgPtr;
                pyrImages.erase(pyrImages.begin());
                deleted = true;
            } else if (accessMap.size() > 0) {
                std::map<int,std::string>::iterator accIt = accessMap.begin();
                map<string, Entry*>::iterator it = images.find(accIt->second);
                if (it != images.end()) {
                    DEBUG_DEBUG("soft flush: removing image: " << it->first);
                    if (it->second->image8) {
                        purgedMem += it->second->image8->width() * it->second->image8->height() * 3;
                    }
                    if (it->second->imageFloat) {
                        purgedMem += it->second->imageFloat->width() * it->second->imageFloat->height()*3*4;
                    }
                    delete it->second;
                    images.erase(it);
                    accessMap.erase(accIt);
                    deleted = true;
                } else {
                    DEBUG_ASSERT("internal error while purging cache");
                }
            }
            if (!deleted) {
                break;
            }
        }
//        m_progress->progressMessage(
//            wxString::Format(_("Purged %d MB from image cache. Current cache usage: %d MB"),
//                             purgedMem>>20, (usedMem - purgedMem)>>20
//                ).c_str(),0);
        DEBUG_DEBUG("purged: " << (purgedMem>>20) << " MB, memory used for images: " << ((usedMem - purgedMem)>>20) << " MB");
        cout << "purged: " << (purgedMem>>20) << " MB, memory used for images: " << ((usedMem - purgedMem)>>20) << " MB" << endl;

    }
}


ImageCache & ImageCache::getInstance()
{
    if (!instance) {
        instance = new ImageCache();
    }
    return *instance;
}



/*
struct ApplyGammaFunctor
{
    float minv;
    float maxv;
    float gamma;
    float scale;

    ApplyGammaFunctor(float min_, float max_, float gamma_)
    {
        minv = min_;
        maxv = max_;
        gamma = gamma_;
        scale = maxv - minv;
    }

    template <class T>
    unsigned char operator()(T v) const
    {
        typedef vigra::NumericTraits<vigra::UInt8>  DestTraits;
        return DestTraits::fromRealPromote(pow((float(v)-minv)/scale, gamma)*255);
    }

    template <class T, unsigned int R, unsigned int G, unsigned int B>
    RGBValue<vigra::UInt8,0,1,2> operator()(const RGBValue<T,R,G,B> & v) const
    {
        typedef vigra::NumericTraits< RGBValue<vigra::UInt8,0,1,2> >  DestTraits;
        typedef vigra::NumericTraits< RGBValue<T,R,G,B> >  SrcTraits;
        return DestTraits::fromRealPromote(pow((SrcTraits::toRealPromote(v)+(-minv))/scale, gamma)*255);
//        return DestTraits::fromRealPromote((log10(SrcTraits::toRealPromote(v)) + (-minv))/scale);
    }
};
*/


#if 0
/// add a scalar to all components, might break other stuff, therefore define just here
template <class V1, class V2>
inline
vigra::RGBValue<V1>
operator+(const vigra::RGBValue<V1> l, V2 const & r)
{
    return vigra::RGBValue<V1>(l.red() + r, l.green() + r, l.blue() + r);
}

/// subtract a scalar to all components, might break other stuff, therefore define just here
template <class V1, class V2>
inline
vigra::RGBValue<V1>
operator-(const vigra::RGBValue<V1> l, V2 const & r)
{
    return vigra::RGBValue<V1>(l.red() - r, l.green() - r, l.blue() - r);
}
#endif


void convertTo8Bit(ImageCache::Entry * entry, wxImage & img)
{
    DEBUG_ASSERT(entry->imageFloat);

    // code to apply the mapping to 8 bit
    // always scaled from 0..1 for integer images.

    img = wxImage(entry->imageFloat->size().x, entry->imageFloat->size().x, false);
    BasicImageView<RGBValue<unsigned char> > imgview((RGBValue<unsigned char> *)img.GetData(),
            img.GetWidth(),
            img.GetHeight());

    double min=0;
    double max=1;

    int mapping = wxConfigBase::Get()->Read(wxT("/ImageCache/MappingInteger"), HUGIN_IMGCACHE_MAPPING_INTEGER);

    // float needs to be from min ... max.
    if (entry->origType == "FLOAT" || entry->origType == "DOUBLE")
    {
        vigra::RGBToGrayAccessor<RGBValue<float> > ga;
        vigra::FindMinMax<float> minmax;   // init functor
        vigra::inspectImage(srcImageRange(*(entry->imageFloat), ga),
                            minmax);
        min = minmax.min;
        max = minmax.max;
        mapping = wxConfigBase::Get()->Read(wxT("/ImageCache/MappingFloat"), HUGIN_IMGCACHE_MAPPING_FLOAT);
    }
    applyMapping(srcImageRange(*(entry->imageFloat)), destImage(imgview), min, max, mapping);
}


struct MyMultFunc
{
    MyMultFunc(double f)
    {
        m = f;
    }
    double m;

    template<class T>
    T
    operator()(T v) const
    {
        return vigra::NumericTraits<T>::fromRealPromote(v*m);
    }
};

template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertImage(const ImageImportInfo & info,
                           vigra::pair<DestIterator, DestAccessor> dest,
                           const std::string & type)
{
    typedef typename DestAccessor::value_type DestPixelType;

    if (type == "FLOAT" || type == "DOUBLE" ) {
        // import image as it is
        vigra::importImage(info, dest);
    } else {
        vigra::importImage(info, dest);
        // integer image.. scale to 0 .. 1
        double scale = 1.0/vigra_ext::LUTTraits<SrcPixelType>::max();
//        DestPixelType factor(scale);
        Multiply<double> f(scale);
        transformImage(dest.first, dest.first+ Diff2D(info.width(), info.height()), dest.second,
                       dest.first, dest.second,
                       Arg1()*Param(scale));
    }
}

#if 0

template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertGrayImage(const ImageImportInfo & info,
                               vigra::pair<DestIterator, DestAccessor> dest,
                               wxString type)

{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef SrcPixelType SrcComponentType;

    // load image into temporary buffer.
    BasicImage<SrcPixelType> tmp(info.width(), info.height());
    vigra::importImage(info, destImage(tmp));

    SrcComponentType min,max;

    int range = wxConfigBase::Get()->Read(wxT("/ImageCache/Range"), HUGIN_IMGCACHE_RANGE);
    if (range == 0) {
        double t;
        wxConfigBase::Get()->Read(wxT("/ImageCache/") + type + wxT("/min"), &t, double(GetRange<SrcComponentType>::min()));
        min = SrcComponentType(t);
        wxConfigBase::Get()->Read(wxT("/ImageCache/") + type + wxT("/max"), &t, double(GetRange<SrcComponentType>::max()));
        max = SrcComponentType(t);
    } else if (range == 1) {
        vigra::FindMinMax<SrcComponentType> minmax;   // init functor
        vigra::inspectImage(srcImageRange(tmp),
                            minmax);
        min = minmax.min;
        max = minmax.max;
    } else{
        vigra_fail("Unknown image import range mode");
    }

    int mapping = wxConfigBase::Get()->Read(wxT("/ImageCache/Mapping"), HUGIN_IMGCACHE_MAPPING);
    applyMapping(srcImageRange(tmp), dest, min, max, mapping);
}

template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertGrayAlphaImage(const ImageImportInfo & info,
                                    vigra::pair<DestIterator, DestAccessor> dest,
                                    wxString type)
{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef SrcPixelType SrcComponentType;

    // load image into temporary buffer.
    BasicImage<SrcPixelType> tmp(info.width(), info.height());
    BImage mask(info.width(), info.height());
    vigra::importImageAlpha(info, destImage(tmp), destImage(mask));

    SrcComponentType min,max;

    int range = wxConfigBase::Get()->Read(wxT("/ImageCache/Range"), HUGIN_IMGCACHE_RANGE);
    if (range == 0) {
        double t;
        wxConfigBase::Get()->Read(wxT("/ImageCache/") + type + wxT("/min"), &t, double(GetRange<SrcComponentType>::min()));
        min = SrcComponentType(t);
        wxConfigBase::Get()->Read(wxT("/ImageCache/") + type + wxT("/max"), &t, double(GetRange<SrcComponentType>::max()));
        max = SrcComponentType(t);
    } else if (range == 1) {
        vigra::FindMinMax<SrcComponentType> minmax;   // init functor
        vigra::inspectImage(srcImageRange(tmp),
                            minmax);
        min = minmax.min;
        max = minmax.max;
    } else{
        vigra_fail("Unknown image import range mode");
    }

    int mapping = wxConfigBase::Get()->Read(wxT("/ImageCache/Mapping"), HUGIN_IMGCACHE_MAPPING);
    applyMapping(srcImageRange(tmp), dest, min, max, mapping);
}

#endif

template <class SrcPixelType,
          class DestIterator, class DestAccessor,
          class MaskIterator, class MaskAccessor>
void importAndConvertAlphaImage(const ImageImportInfo & info,
                                vigra::pair<DestIterator, DestAccessor> dest,
                                vigra::pair<MaskIterator, MaskAccessor> mask,
                                const std::string & type)
{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef SrcPixelType SrcComponentType;

    if (type == "FLOAT" || type == "DOUBLE" ) {
        // import image as it is
        vigra::importImageAlpha(info, dest, mask);
    } else {
        // integer image.. scale to 0 .. 1
        vigra::importImageAlpha(info, dest, mask);
        double scale = 1.0/vigra_ext::LUTTraits<SrcPixelType>::max();
        DestPixelType factor(scale);
        transformImage(dest.first, dest.first+ Diff2D(info.width(), info.height()),  dest.second, 
                       dest.first, dest.second,
                       Arg1()*Param(scale));
    }
}

ImageCache::Entry* ImageCache::getImage(const std::string & filename)
{
//    softFlush();

    std::map<std::string, Entry *>::iterator it;
    it = images.find(filename);
    if (it != images.end()) {
        m_accessCounter++;
        it->second->lastAccess = m_accessCounter;
        return it->second;
    } else {
        if (m_progress) {
            m_progress->pushTask(ProgressTask((const char *)wxString::Format(_("Loading image %s"),wxString(utils::stripPath(filename).c_str(), *wxConvCurrent).c_str()).mb_str(), "", 0));
        }
        wxBusyCursor wait;
#if 1
        // load images with VIGRA impex, and store either 8 bit or float images
        std::string pixelTypeStr;
        vigra::BRGBImage * img8 = 0;
        vigra::FRGBImage * imgFloat = 0;
        vigra::BImage * mask = 0;
        try {
            ImageImportInfo info(filename.c_str());

            /*
            image = new wxImage(info.width(), info.height());

            BasicImageView<RGBValue<unsigned char> > imgview((RGBValue<unsigned char> *)image->GetData(),
                image->GetWidth(),
                image->GetHeight());
            */

            int bands = info.numBands();
            int extraBands = info.numExtraBands();
            const char * pixelType = info.getPixelType();
            pixelTypeStr = pixelType;

            DEBUG_DEBUG(filename << ": bands: " << bands << "  extra bands: " << extraBands << "  type: " << pixelType);

            if (pixelTypeStr == "UINT8") {
                img8 = new vigra::BRGBImage(info.size());
            } else {
                imgFloat = new vigra::FRGBImage(info.size());
            }

            wxString pixelTypeWX(pixelType, *wxConvCurrent);
            if ( bands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::importImage(info, destImage(*img8,
                                       VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)));
                    copyImage(srcImageRange(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                              destImage(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(1)));
                    copyImage(srcImageRange(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                              destImage(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(2)));
                } else {
                    if (strcmp(pixelType, "INT16") == 0 ) {
                        importAndConvertImage<vigra::Int16> (info, destImage(*imgFloat,
                                VectorComponentAccessor<RGBValue<float> >(0)), pixelType);
                    } else if (strcmp(pixelType, "UINT16") == 0 ) {
                        importAndConvertImage<vigra::UInt16>(info, destImage(*imgFloat,
                                VectorComponentAccessor<RGBValue<float> >(0)), pixelType);
                    } else if (strcmp(pixelType, "UINT32") == 0 ) {
                        importAndConvertImage<vigra::UInt32>(info, destImage(*imgFloat,
                                VectorComponentAccessor<vigra::RGBValue<float> >(0)), pixelType);
                    } else if (strcmp(pixelType, "INT32") == 0 ) {
                        importAndConvertImage<vigra::Int32>(info, destImage(*imgFloat,
                                VectorComponentAccessor<RGBValue<float> >(0)), pixelType);
                    } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                        importAndConvertImage<float>(info, destImage(*imgFloat,
                                VectorComponentAccessor<RGBValue<float> >(0)), pixelType);
                    } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                        importAndConvertImage<double>(info, destImage(*imgFloat,
                                VectorComponentAccessor<RGBValue<float> >(0)), pixelType);
                    } else {
                        DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                    }
                    copyImage(srcImageRange(*imgFloat, VectorComponentAccessor<RGBValue<float> >(0)),
                              destImage(*imgFloat, VectorComponentAccessor<RGBValue<float> >(1)));
                    copyImage(srcImageRange(*imgFloat, VectorComponentAccessor<RGBValue<float> >(0)),
                              destImage(*imgFloat, VectorComponentAccessor<RGBValue<float> >(2)));
                }
            } else if (bands == 3 && extraBands == 0) {
                DEBUG_DEBUG( pixelType);
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::importImage(info, destImage(*img8));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertImage<RGBValue<vigra::Int16> > (info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertImage<RGBValue<vigra::UInt16> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertImage<RGBValue<vigra::UInt32> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertImage<RGBValue<vigra::Int32> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertImage<RGBValue<float> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertImage<RGBValue<double> >(info, destImage(*imgFloat), pixelType);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if ( bands == 4 && extraBands == 1) {
                // load and convert image to 8 bit, if needed
                mask = new vigra::BImage(info.size());
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::importImageAlpha(info, destImage(*img8), destImage(*mask));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertAlphaImage<short> (info, destImage(*imgFloat), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertAlphaImage<unsigned short>(info, destImage(*imgFloat), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertAlphaImage<unsigned int>(info, destImage(*imgFloat), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertAlphaImage<int>(info, destImage(*imgFloat), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertAlphaImage<float>(info, destImage(*imgFloat), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertAlphaImage<double>(info, destImage(*imgFloat), destImage(*mask), pixelType);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
#if 0
temporarily disabled
            } else if ( bands == 2 && extraBands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::BImage mask(imgview.size());
                    vigra::importImageAlpha(info, destImage(*img8,  
                                                    VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                                            destImage(mask));
                    copyImage(srcImageRange(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                              destImage(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(1)));
                    copyImage(srcImageRange(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                              destImage(*img8, VectorComponentAccessor<RGBValue<vigra::UInt8> >(2)));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertGrayAlphaImage<short> (info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertGrayAlphaImage<unsigned short>(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertGrayAlphaImage<unsigned int>(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertGrayAlphaImage<int>(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertGrayAlphaImage<float>(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertGrayAlphaImage<double>(info, destImage(*imgFloat), pixelType);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
#endif
            } else {
                DEBUG_ERROR("unsupported depth, only images with 1 or 3 channel images are supported.");
            }
        } catch (std::exception & e) {
            // could not load image..
            wxLogError(wxString::Format(_("Error during image reading: %s"), wxString(e.what(),*wxConvCurrent).c_str()));
            return 0;
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
        Entry * e = new Entry(img8, imgFloat, pixelTypeStr);
        images[filename] = e;
        return e;
    }
}

ImageCache::Entry * ImageCache::getImageWX(const std::string & filename, wxImage & img)
{
    Entry * e = getImage(filename);
    if (e == 0)
        return 0;
    if (e->image8) {
        // 8 bit image.
        img = wxImage(e->image8->width(), e->image8->height(), (unsigned char*) &(e->image8->operator()(0,0)), true);
        return e;
    } else if (e->imageFloat) {
        // convert to 8 bit
        convertTo8Bit(e, img);
        return e;
    } else {
        return 0;
    }
}


ImageCache::Entry * ImageCache::getSmallImage(const std::string & filename)
{
    softFlush();
    std::map<std::string, Entry*>::iterator it;
    // "_small" is only used internally
    string name = filename + string(":small");
    it = images.find(name);
    if (it != images.end()) {
        return it->second;
    } else {
        wxBusyCursor wait;
        if (m_progress) {
            m_progress->pushTask(ProgressTask((const char *)wxString::Format(_("Scaling image %s"),wxString(utils::stripPath(filename).c_str(), *wxConvCurrent).c_str()).mb_str(), "", 0));
        }
        DEBUG_DEBUG("creating small image " << name );
        Entry * entry = getImage(filename);
        if (entry) {
            // && entry->image8
            size_t w=0;
            size_t h=0;
            if (entry->image8) {
                w = entry->image8->width();
                h = entry->image8->height();
            } else if (entry->imageFloat) {
                w = entry->imageFloat->width();
                h = entry->imageFloat->height();
            } else {
                return 0;
            }

            size_t sz = w*h;
            size_t smallImageSize = wxConfigBase::Get()->Read(wxT("/ImageCache/SmallImageSize"), 500 * 500l);

            int nLevel=0;
            while(sz > smallImageSize) {
                sz /=4;
                nLevel++;
            }
            Entry * e = new Entry(0, 0, entry->origType);
            if (entry->image8) {
                e->image8 = new BRGBImage;
                if (entry->mask) {
                    e->mask = new BImage;
                    reduceNTimes(*(entry->image8), *(entry->mask), *(e->image8), *(e->mask), nLevel);
                } else {
                    reduceNTimes(*(entry->image8), *(e->image8), nLevel);
                }
            } else {
                e->imageFloat = new FRGBImage;
                if (entry->mask) {
                    e->mask = new BImage;
                    reduceNTimes(*(entry->imageFloat), *(entry->mask), *(e->imageFloat), *(e->mask), nLevel);
                } else {
                    reduceNTimes(*(entry->imageFloat), *(e->imageFloat), nLevel);
                }
            }
            images[name] = e;
            DEBUG_INFO ( "created small image: " << name);
            if (m_progress) {
                m_progress->popTask();
            }
            return e;
        } else {
            if (m_progress) {
                m_progress->popTask();
            }
            return 0;
        }
    }
}

ImageCache::Entry * ImageCache::getSmallImageWX(const std::string & filename, wxImage & img)
{
    Entry * e = getSmallImage(filename);
    if (e == 0)
        return 0;
    if (e->image8) {
        // 8 bit image.
        vigra_precondition(e->image8->width()> 0 &&  e->image8->height() > 0, "8 bit small image has zero size");
        img = wxImage(e->image8->width(), e->image8->height(), (unsigned char*) &(e->image8->operator()(0,0)), true);
        return e;
    } else if (e->imageFloat) {
        // convert to 8 bit
        vigra_precondition(e->imageFloat->width()> 0 &&  e->imageFloat->height() > 0, "float small image has zero size");
        convertTo8Bit(e, img);
        return e;
    } else {
        return 0;
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
                    wxImage srcImg;
                    Entry * e = getImageWX(filename, srcImg);
                    if (e == 0) {
                        vigra_fail("Error loading initial pyramid image");
                    }
                    img = new vigra::BImage(srcImg.GetWidth(), srcImg.GetHeight());
                    DEBUG_DEBUG("creating level 0 pyramid image for "<< filename);
                    if (m_progress) {
        	      m_progress->pushTask(ProgressTask((const char *)wxString::Format(_("Creating grayscale %s"),wxString(filename.c_str(), *wxConvCurrent).c_str()).mb_str(), "", 0));
                    }
                    BasicImageView<RGBValue<unsigned char> > src((RGBValue<unsigned char> *)srcImg.GetData(),
                                                                 srcImg.GetWidth(),
                                                                 srcImg.GetHeight());
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

SmallRemappedImageCache::MRemappedImage *
SmallRemappedImageCache::getRemapped(const PT::Panorama & pano,
                                    const PT::PanoramaOptions & popts,
                                    unsigned int imgNr,
                                    utils::MultiProgressDisplay & progress)
{
    // always map to HDR mode. curve and exposure is applied in preview window, for speed
    PanoramaOptions opts = popts;
    opts.outputMode = PanoramaOptions::OUTPUT_HDR;
    opts.outputExposureValue = 0.0;

    // return old image, if already in cache and if it has changed since the last rendering
    if (set_contains(m_images, imgNr)) {
        // return cached image if the parameters of the image have not changed
        SrcPanoImage oldParam = m_imagesParam[imgNr];
        if (oldParam == pano.getSrcImage(imgNr)
                && m_panoOpts[imgNr].getHFOV() == opts.getHFOV()
                && m_panoOpts[imgNr].getWidth() == opts.getWidth()
                && m_panoOpts[imgNr].getHeight() == opts.getHeight()
                && m_panoOpts[imgNr].getProjection() == opts.getProjection()
                && m_panoOpts[imgNr].getProjectionParameters() == opts.getProjectionParameters()
           )
        {
            DEBUG_DEBUG("using cached remapped image " << imgNr);
            return m_images[imgNr];
        }
    }

    // WARNING: this might invalidate images that are stored somewhere..
    ImageCache::getInstance().softFlush();
    
    typedef  BasicImageView<RGBValue<unsigned char> > BRGBImageView;

//    typedef vigra::NumericTraits<PixelType>::RealPromote RPixelType;

    // remap image
    DEBUG_DEBUG("remapping image " << imgNr);

    // load image
    const PanoImage & img = pano.getImage(imgNr);
    const PT::ImageOptions & iopts = img.getOptions();

    ImageCache::Entry * e = ImageCache::getInstance().getSmallImage(img.getFilename().c_str());
    if (!e) {
        throw std::runtime_error("could not retrieve small source image for preview generation");
    }
    Size2D srcImgSize;
    if (e->image8)
        srcImgSize = e->image8->size();
    else
        srcImgSize = e->imageFloat->size();

    MRemappedImage *remapped = new MRemappedImage;
    SrcPanoImage srcPanoImg = pano.getSrcImage(imgNr);
    // adjust distortion parameters for small preview image
    srcPanoImg.resize(srcImgSize);

    FImage srcFlat;
    // use complete image, by supplying an empty mask image
    BImage srcMask;

    if (iopts.m_vigCorrMode & ImageOptions::VIGCORR_FLATFIELD) {
        ImageCache::Entry * e = ImageCache::getInstance().getSmallImage(iopts.m_flatfield.c_str());
        if (!e) {
            throw std::runtime_error("could not retrieve flatfield image for preview generation");
        }
        if (e->image8) {
            srcFlat.resize(e->image8->size());
            vigra::copyImage(srcImageRange(*(e->image8),
                             RGBToGrayAccessor<RGBValue<vigra::UInt8> >()),
                             destImage(srcFlat));
        } else {
            srcFlat.resize(e->imageFloat->size());
            vigra::copyImage(srcImageRange(*(e->imageFloat),
                             RGBToGrayAccessor<RGBValue<float> >()),
                             destImage(srcFlat));
        }
    }
    progress.pushTask(ProgressTask("remapping", "", 0));

    if (e->imageFloat) {
        // remap image
        remapImage(*(e->imageFloat),
                   srcMask,
                   srcFlat,
                   srcPanoImg,
                   opts,
                   *remapped,
                   progress);
    } else {
        remapImage(*(e->image8),
                     srcMask,
                     srcFlat,
                     srcPanoImg,
                     opts,
                     *remapped,
                     progress);
    }

    progress.popTask();

    m_images[imgNr] = remapped;
    m_imagesParam[imgNr] = pano.getSrcImage(imgNr);
    m_panoOpts[imgNr] = opts;
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
    m_imagesParam.clear();
}

void SmallRemappedImageCache::invalidate(unsigned int imgNr)
{
    DEBUG_DEBUG("Remove " << imgNr << " from remapped cache");
    if (set_contains(m_images, imgNr)) {
        delete (m_images[imgNr]);
        m_images.erase(imgNr);
        m_imagesParam.erase(imgNr);
    }
}


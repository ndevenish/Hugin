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
    long purgeToSize = upperBound;

    // calculate used memory
    long imgMem = 0;

    std::map<std::string, Entry*>::iterator imgIt;
    for(imgIt=images.begin(); imgIt != images.end(); imgIt++) {
        imgMem += imgIt->second->image->GetWidth() * imgIt->second->image->GetHeight() * 3;
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

        // use least recently uses strategy.
        // sort images by their access time
        std::map<int,std::string> accessMap;
        for (map<string, Entry*>::iterator it = images.begin();
             it != images.end();
             ++it)
        {
            if (it->first.substr(it->first.size()-6) != "_small") {
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
                    purgedMem += it->second->image->GetWidth() * it->second->image->GetHeight() * 3;
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
    }
}

ImageCache & ImageCache::getInstance()
{
    if (!instance) {
        instance = new ImageCache();
    }
    return *instance;
}


template <class TIn, class TOut=vigra::UInt8>
struct ApplyGammaFunctor
{
    float minv;
    float maxv;
    float gamma;
    float scale;

    ApplyGammaFunctor(TIn min_, TIn max_, float gamma_)
    {
        minv = min_;
        maxv = max_;
        gamma = gamma_;
        scale = float(maxv) - minv;
    }

    TOut operator()(TIn v) const
    {
        typedef vigra::NumericTraits<TOut>  DestTraits;
        return DestTraits::fromRealPromote(pow((float(v)-minv)/scale, gamma)*255);
    }

    RGBValue<TOut> operator()(const RGBValue<TIn> & v) const
    {
        typedef vigra::NumericTraits< RGBValue<TOut> >  DestTraits;
        typedef vigra::NumericTraits< RGBValue<TIn> >  SrcTraits;
        return DestTraits::fromRealPromote(pow((SrcTraits::toRealPromote(v)+(-minv))/scale, gamma)*255);
    }
};

// gamma correction with lookup table
template <>
struct ApplyGammaFunctor<vigra::UInt16, vigra::UInt8>
{
    vigra::UInt8 lut[65536];

    ApplyGammaFunctor(vigra::UInt16 min, vigra::UInt16 max, float gamma)
    {
        float scale = float(max) - min;
        for (int i=0; i<65536; i++) {
            lut[i] = roundi(pow((float(i)-min)/scale, gamma)*255);
        }
    }

    vigra::UInt8 operator()(vigra::UInt16 v) const
    {
        return lut[v];
    }

    RGBValue<vigra::UInt8> operator()(const RGBValue<vigra::UInt16> & v) const
    {
        return RGBValue<vigra::UInt8>(lut[v[0]], lut[v[1]], lut[v[2]]);
    }
};

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

struct ApplyLogFunctor
{
    float minv;
    float maxv;
    float scale;

    ApplyLogFunctor(float min_, float max_)
    {
        // protect against zeros in image data
        if (min_ == 0.0f) {
            min_ = 1e-5;
        }
        minv = log10(min_);
        maxv = log10(max_);
        scale = (maxv - minv)/255;
        DEBUG_DEBUG("gray range: " << min_ << " " << max_ << "  log range: " << minv << " " << maxv << "  scale:" << scale );
    }

    template <class T>
    unsigned char operator()(T v) const
    {
        typedef vigra::NumericTraits<vigra::UInt8>  DestTraits;
        return DestTraits::fromRealPromote((log10(float(v))-minv)/scale);
    }

    template <class T, unsigned int R, unsigned int G, unsigned int B>
    RGBValue<vigra::UInt8,0,1,2> operator()(const RGBValue<T,R,G,B> & v) const
    {
        typedef vigra::NumericTraits< RGBValue<vigra::UInt8,0,1,2> >  DestTraits;
        typedef vigra::NumericTraits< RGBValue<T,R,G,B> >  SrcTraits;
        return DestTraits::fromRealPromote((log10(SrcTraits::toRealPromote(v)) + (-minv))/scale);
    }
};


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

template <class DestValueType>
struct LinearTransform
{
  public:
        /* the functors argument type (actually, since 
           <tt>operator()</tt> is a template, much more types are possible)
        */
    typedef DestValueType argument_type;

        /* the functors result type
        */
    typedef DestValueType result_type;

        /* init scale and offset
        */
    LinearTransform(float scale, float offset)
    : scale_(scale), offset_(offset)
    {}
    template <class SrcValueType>
    result_type operator()(SrcValueType const & s) const
    {
        return NumericTraits<result_type>::fromRealPromote(scale_ * (NumericTraits<SrcValueType>::toRealPromote(s) + offset_));
    }
  private:

    float scale_;
    float offset_;
};

template <class SrcIterator, class SrcAccessor, class DestIterator, class DestAccessor, class T>
void applyMapping(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> img,
                  vigra::pair<DestIterator, DestAccessor> dest, T min, T max )
{
    int mapping = wxConfigBase::Get()->Read(wxT("/ImageCache/Mapping"), HUGIN_IMGCACHE_MAPPING);

    switch (mapping)
    {
        case 0:
        {
            // linear
            float offset_ = -float(min);
            float scale_ = 255/float(max)-float(min);
            vigra::transformImage(img, dest,
                                LinearTransform<typename DestAccessor::value_type>( scale_, offset_)
                                );
            break;
        }
        case 1:
        {
            // log
            ApplyLogFunctor logfunc(min, max);
            transformImage(img, dest,
                           logfunc);
            break;
        }
        case 2:
        {
            // gamma
            ApplyGammaFunctor<T> logfunc(min, max, 1/2.2f);
            transformImage(img, dest,
                        logfunc);
            break;
        }
        default:
            vigra_fail("Unknown image mapping mode");
    }
}


template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertImage(const ImageImportInfo & info,
                           vigra::pair<DestIterator, DestAccessor> dest,
                           wxString & type)
{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef typename SrcPixelType::value_type SrcComponentType;

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
        vigra::RGBToGrayAccessor<RGBValue<SrcPixelType> > ga;
        vigra::FindMinMax<SrcComponentType> minmax;   // init functor
        vigra::inspectImage(srcImageRange(tmp, ga),
                              minmax);
        min = minmax.min;
        max = minmax.max;
    } else{
        vigra_fail("Unknown image import range mode");
    }

    applyMapping(srcImageRange(tmp), dest, min, max);
}


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

    applyMapping(srcImageRange(tmp), dest, min, max);
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

    applyMapping(srcImageRange(tmp), dest, min, max);
}


template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void importAndConvertAlphaImage(const ImageImportInfo & info,
                                vigra::pair<DestIterator, DestAccessor> dest,
                                wxString type)
{
    typedef typename DestAccessor::value_type DestPixelType;
    typedef typename DestPixelType::value_type DestComponentType;
    typedef SrcPixelType SrcComponentType;

    // load image into temporary buffer.
    BasicImage<RGBValue<SrcPixelType> > tmp(info.width(), info.height());
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
        vigra::RGBToGrayAccessor<RGBValue<SrcPixelType> > ga;
        vigra::FindMinMax<SrcComponentType> minmax;   // init functor
        vigra::inspectImageIf(srcImageRange(tmp, ga),
                              maskImage(mask),
                              minmax);
        min = minmax.min;
        max = minmax.max;
    } else{
        vigra_fail("Unknown image import range mode");
    }

    applyMapping(srcImageRange(tmp), dest, min, max);
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
        // load images with VIGRA impex, and convert to 8 bit, if required.
        wxImage * image;
        std::string pixelTypeStr;
        bool linear=true;
        try {
            ImageImportInfo info(filename.c_str());

            image = new wxImage(info.width(), info.height());

            BasicImageView<RGBValue<unsigned char> > imgview((RGBValue<unsigned char> *)image->GetData(),
                image->GetWidth(),
                image->GetHeight());

            int bands = info.numBands();
            int extraBands = info.numExtraBands();
            const char * pixelType = info.getPixelType();
            pixelTypeStr = pixelType;

            DEBUG_DEBUG(filename << ": bands: " << bands << "  extra bands: " << extraBands << "  type: " << pixelType);

            wxString pixelTypeWX(pixelType, *wxConvCurrent);
            if ( bands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::importImage(info, destImage(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)));
                    copyImage(srcImageRange(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                              destImage(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(1)));
                    copyImage(srcImageRange(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                              destImage(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(2)));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertGrayImage<short> (info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertGrayImage<unsigned short >(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertGrayImage<unsigned int>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertGrayImage<int>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertGrayImage<float>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertGrayImage<double>(info, destImage(imgview), pixelTypeWX);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if (bands == 3 && extraBands == 0) {
                DEBUG_DEBUG( pixelType);
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::importImage(info, destImage(imgview));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertImage<RGBValue<short> > (info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertImage<RGBValue<unsigned short> >(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertImage<RGBValue<unsigned int> >(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertImage<RGBValue<int> >(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertImage<RGBValue<float> >(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertImage<RGBValue<double> >(info, destImage(imgview), pixelTypeWX);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if ( bands == 4 && extraBands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::BImage mask(imgview.size());
                    vigra::importImageAlpha(info, destImage(imgview), destImage(mask));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertAlphaImage<short> (info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertAlphaImage<unsigned short>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertAlphaImage<unsigned int>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertAlphaImage<int>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertAlphaImage<float>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertAlphaImage<double>(info, destImage(imgview), pixelTypeWX);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if ( bands == 2 && extraBands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::BImage mask(imgview.size());
                    vigra::importImageAlpha(info, destImage(imgview,  
                                                    VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                                            destImage(mask));
                    copyImage(srcImageRange(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                                destImage(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(1)));
                    copyImage(srcImageRange(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(0)),
                                destImage(imgview, VectorComponentAccessor<RGBValue<vigra::UInt8> >(2)));
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertGrayAlphaImage<short> (info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertGrayAlphaImage<unsigned short>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertGrayAlphaImage<unsigned int>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertGrayAlphaImage<int>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertGrayAlphaImage<float>(info, destImage(imgview), pixelTypeWX);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertGrayAlphaImage<double>(info, destImage(imgview), pixelTypeWX);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
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
        int mapping = wxConfigBase::Get()->Read(wxT("/ImageCache/Mapping"), HUGIN_IMGCACHE_MAPPING);
        int range = wxConfigBase::Get()->Read(wxT("/ImageCache/Range"), HUGIN_IMGCACHE_RANGE);

        linear = (mapping == 0 && range == 0);

        Entry * e = new Entry(image, pixelTypeStr, linear);
        images[filename] = e;
        return e;
    }
}

ImageCache::Entry * ImageCache::getSmallImage(const std::string & filename)
{
//    softFlush();
    std::map<std::string, Entry*>::iterator it;
    // "_small" is only used internally
    string name = filename + string("_small");
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
        wxImage * image = entry->image;
        if (image->Ok()) {
            wxImage small_image;
            const int w = 512;
            double ratio = (double)image->GetWidth() / image->GetHeight();
            small_image = image->Scale(w, (int) (w/ratio));

            wxImage * tmp = new wxImage( small_image );
            Entry * e = new Entry(tmp, entry->origType, entry->linear);
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
                    wxImage * srcImg = getImage(filename)->image;
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

SmallRemappedImageCache::MRemappedImage *
SmallRemappedImageCache::getRemapped(const PT::Panorama & pano,
                                    const PT::PanoramaOptions & opts,
                                    unsigned int imgNr,
                                    utils::MultiProgressDisplay & progress)
{
    // return old image, if already in cache and if it has changed since the last rendering
    if (set_contains(m_images, imgNr)) {
        // return cached image if the parameters of the image have not changed
        SrcPanoImage oldParam = m_imagesParam[imgNr];
        if (oldParam == pano.getSrcImage(imgNr)) {
            DEBUG_DEBUG("using cached remapped image " << imgNr);
            return m_images[imgNr];
        }
    }

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
    wxImage * src = e->image;
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
        ImageCache::Entry * e = ImageCache::getInstance().getSmallImage(iopts.m_flatfield.c_str());
        if (!e) {
            throw std::runtime_error("could not retrieve flatfield image for preview generation");
        }

        wxImage * flatsrc = e->image;
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
               opts,
               *remapped,
               progress);

    m_images[imgNr] = remapped;
    m_imagesParam[imgNr] = pano.getSrcImage(imgNr);
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


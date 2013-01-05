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

#include "ImageCache.h"

#include <iostream>
#include <boost/thread/thread.hpp>
#include <vigra/inspectimage.hxx>
#include <vigra/accessor.hxx>
#include <vigra/functorexpression.hxx>
#include <vigra/sized_int.hxx>
#include <vigra_ext/utils.h>
#include <vigra_ext/impexalpha.hxx>
#include <vigra_ext/Pyramid.h>
#include <vigra_ext/FunctorAccessor.h>



namespace HuginBase {
    
using namespace std;
using namespace vigra::functor;


template <class T1>
class GetRange
{
    public:
        static T1 min();
        static T1 max();
};

// ImageCache::GetRange implementation
#define VIGRA_EXT_GETRANGE(T1, MI,MA) \
template<> \
T1 GetRange<T1>::min() \
{ \
        return MI; \
} \
template<> \
T1 GetRange<T1>::max() \
{ \
        return MA; \
} \

VIGRA_EXT_GETRANGE(vigra::UInt8,  0, 255);
VIGRA_EXT_GETRANGE(vigra::Int16,  0, 32767);
VIGRA_EXT_GETRANGE(vigra::UInt16, 0, 65535);
VIGRA_EXT_GETRANGE(vigra::Int32,  0, 2147483647);
VIGRA_EXT_GETRANGE(vigra::UInt32, 0, 4294967295u);
VIGRA_EXT_GETRANGE(float,  0, 1.0f);
VIGRA_EXT_GETRANGE(double, 0, 1.0);

#undef VIGRA_EXT_GETRANGE


template <class SrcIMG>
void convertTo8Bit(SrcIMG & src, const std::string & origType, vigra::BRGBImage & dest)
{
    // code to apply the mapping to 8 bit
    // always scaled from 0..1 for integer images.
    
    dest.resize(src.size());
    
    double min=0;
    double max=vigra_ext::getMaxValForPixelType(origType);
    ;
    
    int mapping = HUGIN_IMGCACHE_MAPPING_INTEGER;
    
    // float needs to be from min ... max.
    if (origType == "FLOAT" || origType == "DOUBLE")
    {
        vigra::RGBToGrayAccessor<vigra::RGBValue<float> > ga;
        vigra::FindMinMax<float> minmax;   // init functor
        vigra::inspectImage(srcImageRange(src, ga),
                            minmax);
        min = minmax.min;
        max = minmax.max;
        mapping = HUGIN_IMGCACHE_MAPPING_FLOAT;
    }
    vigra_ext::applyMapping(srcImageRange(src), destImage(dest), min, max, mapping);
}



ImageCache::ImageCacheRGB8Ptr ImageCache::Entry::get8BitImage()
{
    if (image8->width() > 0) {
        return image8;
    } else if (image16->width() > 0) {
        convertTo8Bit(*image16,
                      origType,
                      *image8);
    } else if (imageFloat->width() > 0) {
        convertTo8Bit(*imageFloat,
                      origType,
                      *image8);
    }
    return image8;
}

ImageCache * ImageCache::instance = NULL;


void ImageCache::removeImage(const std::string & filename)
{
    map<string, EntryPtr>::iterator it = images.find(filename);
    if (it != images.end()) {
        images.erase(it);
    }

    string sfilename = filename + string("_small");
    it = images.find(sfilename);
    if (it != images.end()) {
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
    if(upperBound==0l)
        upperBound = 100 * 1024 * 1024l;
    long purgeToSize = long(0.75 * upperBound);

    // calculate used memory
    long imgMem = 0;

    std::map<std::string, EntryPtr>::iterator imgIt;
    for(imgIt=images.begin(); imgIt != images.end(); imgIt++) {
#ifdef DEBUG
        cout << "Image: " << imgIt->first << std::endl;
        cout << "CacheEntry: " << imgIt->second.use_count() << "last access: " << imgIt->second->lastAccess;
#endif
        if (imgIt->second->image8) {
            imgMem += imgIt->second->image8->width() * imgIt->second->image8->height() * 3;
#ifdef DEBUG
            cout << " 8bit: " << imgIt->second->image8.use_count();
#endif
        }
        if (imgIt->second->image16) {
            imgMem += imgIt->second->image16->width() * imgIt->second->image16->height() * 3*2;
#ifdef DEBUG
            cout << " 16bit: " << imgIt->second->image8.use_count();
#endif
        }
        if (imgIt->second->imageFloat) {
            imgMem += imgIt->second->imageFloat->width() * imgIt->second->imageFloat->height() * 3 * 4;
#ifdef DEBUG
            cout << " float: " << imgIt->second->imageFloat.use_count() ;
#endif
        }
        if (imgIt->second->mask) {
            imgMem += imgIt->second->mask->width() * imgIt->second->mask->height();
#ifdef DEBUG
            cout << " mask: " << imgIt->second->mask.use_count() << std:: endl;
#endif
        }
    }

    long pyrMem = 0;
    std::map<std::string, vigra::BImage*>::iterator pyrIt;
    for(pyrIt=pyrImages.begin(); pyrIt != pyrImages.end(); pyrIt++) {
        pyrMem += pyrIt->second->width() * pyrIt->second->height();
    }

    long usedMem = imgMem + pyrMem;

    DEBUG_DEBUG("total: " << (usedMem>>20) << " MB upper bound: " << (purgeToSize>>20) << " MB");
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
        for (map<string, EntryPtr>::iterator it = images.begin();
             it != images.end();
             ++it)
        {
            if (it->first.substr(it->first.size()-6) != ":small") {
                // only consider full images that are not used elsewhere
                if (it->second.unique()) {
                    DEBUG_DEBUG("Considering " << it->first << " for deletion");
                    accessMap.insert(make_pair(it->second->lastAccess, it->first));
                } else {
                    DEBUG_DEBUG(it->first << ", usecount: " << it->second.use_count());
                }
            }
        }
        while (purgeAmount > purgedMem) {
            bool deleted = false;
            if (pyrImages.size() > 0) {
                vigra::BImage * imgPtr = (*(pyrImages.begin())).second;
                purgedMem += imgPtr->width() * imgPtr->height();
                delete imgPtr;
                pyrImages.erase(pyrImages.begin());
                deleted = true;
            } else if (accessMap.size() > 0) {
                std::map<int,std::string>::iterator accIt = accessMap.begin();
                map<string, EntryPtr>::iterator it = images.find(accIt->second);
                // check for uniqueness.
                if (it != images.end()) {
                    DEBUG_DEBUG("soft flush: removing image: " << it->first);
                    if (it->second->image8) {
                        purgedMem += it->second->image8->width() * it->second->image8->height() * 3;
                    }
                    if (it->second->image16) {
                        purgedMem += it->second->image16->width() * it->second->image16->height() * 3 * 2;
                    }
                    if (it->second->imageFloat) {
                        purgedMem += it->second->imageFloat->width() * it->second->imageFloat->height()*3*4;
                    }
                    if (it->second->mask) {
                        purgedMem += it->second->mask->width() * it->second->mask->height();
                    }
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



ImageCache& ImageCache::getInstance()
{
    if (instance == NULL)
    {
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


//#if 0
///// add a scalar to all components, might break other stuff, therefore define just here
//template <class V1, class V2>
//inline
//vigra::RGBValue<V1>
//operator+(const vigra::RGBValue<V1> l, V2 const & r)
//{
//    return vigra::RGBValue<V1>(l.red() + r, l.green() + r, l.blue() + r);
//}
//
///// subtract a scalar to all components, might break other stuff, therefore define just here
//template <class V1, class V2>
//inline
//vigra::RGBValue<V1>
//operator-(const vigra::RGBValue<V1> l, V2 const & r)
//{
//    return vigra::RGBValue<V1>(l.red() - r, l.green() - r, l.blue() - r);
//}
//#endif




//struct MyMultFunc
//{
//    MyMultFunc(double f)
//    {
//        m = f;
//    }
//    double m;
//
//    template<class T>
//    T
//    operator()(T v) const
//    {
//        return vigra::NumericTraits<T>::fromRealPromote(v*m);
//    }
//};

template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void ImageCache::importAndConvertImage(const vigra::ImageImportInfo & info,
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
        vigra_ext::Multiply<double> f(scale);
        transformImage(dest.first, dest.first+ vigra::Diff2D(info.width(), info.height()), dest.second,
                       dest.first, dest.second,
                       Arg1()*Param(scale));
    }
}

#if 0

template <class SrcPixelType,
          class DestIterator, class DestAccessor>
void ImageCache::importAndConvertGrayImage(const ImageImportInfo & info,
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
void ImageCache::importAndConvertGrayAlphaImage(const ImageImportInfo & info,
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
void ImageCache::importAndConvertAlphaImage(const vigra::ImageImportInfo & info,
                                            vigra::pair<DestIterator, DestAccessor> dest,
                                            vigra::pair<MaskIterator, MaskAccessor> mask,
                                            const std::string & type)
{
    if (type == "FLOAT" || type == "DOUBLE" ) {
        // import image as it is
        vigra::importImageAlpha(info, dest, mask);
    } else {
        // integer image.. scale to 0 .. 1
        vigra::importImageAlpha(info, dest, mask);
        double scale = 1.0/vigra_ext::LUTTraits<SrcPixelType>::max();
        transformImage(dest.first, dest.first+ vigra::Diff2D(info.width(), info.height()),  dest.second, 
                       dest.first, dest.second,
                       Arg1()*Param(scale));
    }
}

ImageCache::EntryPtr ImageCache::getImage(const std::string & filename)
{
//    softFlush();
    m_accessCounter++;
    std::map<std::string, EntryPtr>::iterator it;
    it = images.find(filename);
    if (it != images.end()) {
        it->second->lastAccess = m_accessCounter;
        return it->second;
    } else {
        if (m_progress) {
            m_progress->pushTask(AppBase::ProgressTask("Loading image: "+hugin_utils::stripPath(filename), "", 0));
        }
        
        EntryPtr e = loadImageSafely(filename);
        
        if (m_progress) {
            m_progress->popTask();
        }
        
        if (!e.get())
        {
            // could not access image.
            throw std::exception();
        }
        
        images[filename] = e;
        e->lastAccess = m_accessCounter;
        return e;
    }
}

ImageCache::EntryPtr ImageCache::loadImageSafely(const std::string & filename)
{
    // load images with VIGRA impex, and store either 8 bit or float images
    std::string pixelTypeStr;
    ImageCacheRGB8Ptr img8(new vigra::BRGBImage);
    ImageCacheRGB16Ptr img16(new vigra::UInt16RGBImage);
    ImageCacheRGBFloatPtr imgFloat(new vigra::FRGBImage);
    ImageCache8Ptr mask(new vigra::BImage);

    try {
        vigra::ImageImportInfo info(filename.c_str());

        int bands = info.numBands();
        int extraBands = info.numExtraBands();
        const char * pixelType = info.getPixelType();
        pixelTypeStr = pixelType;

        DEBUG_DEBUG(filename << ": bands: " << bands << "  extra bands: " << extraBands << "  type: " << pixelType);

        if (pixelTypeStr == "UINT8") {
            img8->resize(info.size());
        } else if (pixelTypeStr == "UINT16" ) {
            img16->resize(info.size());
        } else {
            imgFloat->resize(info.size());
        }

        if ( bands == 1) {
            // load and convert image to 8 bit, if needed
            if (strcmp(pixelType, "UINT8") == 0 ) {
                vigra::importImage(info, destImage(*img8,
                                                   vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)));
                copyImage(srcImageRange(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)),
                          destImage(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(1)));
                copyImage(srcImageRange(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)),
                          destImage(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(2)));
            } else if (strcmp(pixelType, "UINT16") == 0 ) {
                vigra::importImage(info, destImage(*img16,
                                                   vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(0)));
                copyImage(srcImageRange(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(0)),
                          destImage(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(1)));
                copyImage(srcImageRange(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(0)),
                          destImage(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(2)));
            } else {
                if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertImage<vigra::Int16> (info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), pixelType);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertImage<vigra::UInt32>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), pixelType);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertImage<vigra::Int32>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), pixelType);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertImage<float>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), pixelType);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertImage<double>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), pixelType);
                } else {
                    DEBUG_ERROR("Unsupported pixel type: " << pixelType);
                    return EntryPtr();
                }
                copyImage(srcImageRange(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)),
                          destImage(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(1)));
                copyImage(srcImageRange(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)),
                          destImage(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(2)));
            }
        } else if ( bands == 2 && extraBands==1) {
            mask->resize(info.size());
            // load and convert image to 8 bit, if needed
            if (strcmp(pixelType, "UINT8") == 0 ) {
                vigra::importImageAlpha(info, destImage(*img8,
                                                   vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)),
                                                   destImage(*mask));
                copyImage(srcImageRange(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)),
                          destImage(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(1)));
                copyImage(srcImageRange(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)),
                          destImage(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(2)));
            } else if (strcmp(pixelType, "UINT16") == 0 ) {
                vigra::importImageAlpha(info, destImage(*img16,
                                                   vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(0)),
                                                   destImage(*mask));
                copyImage(srcImageRange(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(0)),
                          destImage(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(1)));
                copyImage(srcImageRange(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(0)),
                          destImage(*img16, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt16> >(2)));
            } else {
                if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertAlphaImage<vigra::Int16> (info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertAlphaImage<vigra::UInt32>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertAlphaImage<vigra::Int32>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertAlphaImage<float>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), destImage(*mask), pixelType);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertAlphaImage<double>(info, destImage(*imgFloat,
                            vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), destImage(*mask), pixelType);
                } else {
                    DEBUG_ERROR("Unsupported pixel type: " << pixelType);
                    return EntryPtr();
                }
                copyImage(srcImageRange(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)),
                          destImage(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(1)));
                copyImage(srcImageRange(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)),
                          destImage(*imgFloat, vigra::VectorComponentAccessor<vigra::RGBValue<float> >(2)));
            }
        } else if (bands == 3 && extraBands == 0) {
            DEBUG_DEBUG( pixelType);
            // load and convert image to 8 bit, if needed
            if (strcmp(pixelType, "UINT8") == 0 ) {
                vigra::importImage(info, destImage(*img8));
            } else if (strcmp(pixelType, "UINT16") == 0 ) {
                vigra::importImage(info, destImage(*img16));
            } else if (strcmp(pixelType, "INT16") == 0 ) {
                importAndConvertImage<vigra::RGBValue<vigra::Int16> > (info, destImage(*imgFloat), pixelType);
            } else if (strcmp(pixelType, "UINT32") == 0 ) {
                importAndConvertImage<vigra::RGBValue<vigra::UInt32> >(info, destImage(*imgFloat), pixelType);
            } else if (strcmp(pixelType, "INT32") == 0 ) {
                importAndConvertImage<vigra::RGBValue<vigra::Int32> >(info, destImage(*imgFloat), pixelType);
            } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                vigra::importImage(info, destImage(*imgFloat));
//                    importAndConvertImage<vigra::RGBValue<float> >(info, destImage(*imgFloat), pixelType);
            } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                vigra::importImage(info, destImage(*imgFloat));
//                    importAndConvertImage<vigra::RGBValue<double> >(info, destImage(*imgFloat), pixelType);
            } else {
                DEBUG_ERROR("Unsupported pixel type: " << pixelType);
                return EntryPtr();
            }
        } else if ( bands == 4 && extraBands == 1) {
            mask->resize(info.size());
            // load and convert image to 8 bit, if needed
            if (strcmp(pixelType, "UINT8") == 0 ) {
                vigra::importImageAlpha(info, destImage(*img8), destImage(*mask));
            } else if (strcmp(pixelType, "UINT16") == 0 ) {
                vigra::importImageAlpha(info, destImage(*img16), destImage(*mask));
            } else if (strcmp(pixelType, "INT16") == 0 ) {
                importAndConvertAlphaImage<short> (info, destImage(*imgFloat), destImage(*mask), pixelType);
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
                return EntryPtr();
            }
        } else {
            DEBUG_ERROR("unsupported depth, only images with 1 or 3 channel images are supported.");
            return EntryPtr();
        }
    } catch (std::exception & e) {
        // could not load image..
       DEBUG_ERROR("Error during image reading: " << e.what());
       return EntryPtr();
    }

    return EntryPtr(new Entry(img8, img16, imgFloat, mask, pixelTypeStr));
}

ImageCache::EntryPtr ImageCache::getImageIfAvailable(const std::string & filename)
{
    std::map<std::string, EntryPtr>::iterator it;
    it = images.find(filename);
    if (it != images.end()) {
        m_accessCounter++;
        it->second->lastAccess = m_accessCounter;
        return it->second;
    } else {
        // not found, return 0 pointer.
        return EntryPtr();
    }
}

ImageCache::EntryPtr ImageCache::getSmallImage(const std::string & filename)
{
    m_accessCounter++;
    softFlush();
    std::map<std::string, EntryPtr>::iterator it;
    // "_small" is only used internally
    string name = filename + string(":small");
    it = images.find(name);
    if (it != images.end()) {
        return it->second;
    } else {
        if (m_progress)
        {
            m_progress->pushTask(AppBase::ProgressTask("Scaling image: "+hugin_utils::stripPath(filename), "", 0));
        }
        DEBUG_DEBUG("creating small image " << name );
        EntryPtr entry = getImage(filename);
        
        EntryPtr small_entry = loadSmallImageSafely(entry);
        small_entry->lastAccess = m_accessCounter;
        images[name] = small_entry;
        DEBUG_INFO ( "created small image: " << name);
        if (m_progress) {
            m_progress->popTask();
        }
        return small_entry;
    }
}

ImageCache::EntryPtr ImageCache::loadSmallImageSafely(EntryPtr entry)
{
    // && entry->image8
    size_t w=0;
    size_t h=0;
    if (entry->image8->width() > 0) {
        w = entry->image8->width();
        h = entry->image8->height();
    } else if (entry->image16->width() > 0) {
        w = entry->image16->width();
        h = entry->image16->height();
    } else if (entry->imageFloat->width() > 0) {
        w = entry->imageFloat->width();
        h = entry->imageFloat->height();
    } else {
        vigra_fail("Could not load image");
    }

    size_t sz = w*h;
    size_t smallImageSize = 800 * 800l;

    int nLevel=0;
    while(sz > smallImageSize) {
        sz /=4;
        nLevel++;
    }
    EntryPtr e(new Entry);
    e->origType = entry->origType;
    // TODO: fix bug with mask reduction
    vigra::BImage fullsizeMask = *(entry->mask);
    if (entry->imageFloat->width() != 0 ) {
        e->imageFloat = ImageCacheRGBFloatPtr(new vigra::FRGBImage);
        if (entry->mask->width() != 0) {
            vigra_ext::reduceNTimes(*(entry->imageFloat), fullsizeMask, *(e->imageFloat), *(e->mask), nLevel);
        } else {
            vigra_ext::reduceNTimes(*(entry->imageFloat), *(e->imageFloat), nLevel);
        }
    }
    if (entry->image16->width() != 0 ) {
        e->image16 = ImageCacheRGB16Ptr(new vigra::UInt16RGBImage);
        if (entry->mask->width() != 0) {
            vigra_ext::reduceNTimes(*(entry->image16), fullsizeMask, *(e->image16), *(e->mask), nLevel);
        } else {
            vigra_ext::reduceNTimes(*(entry->image16), *(e->image16), nLevel);
        }
    }
    if (entry->image8->width() != 0) {
        e->image8 = ImageCacheRGB8Ptr(new vigra::BRGBImage);
        if (entry->mask->width() != 0) {
            vigra_ext::reduceNTimes(*(entry->image8), fullsizeMask, *(e->image8), *(e->mask), nLevel);
        } else {
            vigra_ext::reduceNTimes(*(entry->image8), *(e->image8), nLevel);
        }
    }
    return e;
}

ImageCache::EntryPtr ImageCache::getSmallImageIfAvailable(const std::string & filename)
{
    m_accessCounter++;
    softFlush();
    std::map<std::string, EntryPtr>::iterator it;
    // "_small" is only used internally
    string name = filename + string(":small");
    it = images.find(name);
    if (it != images.end()) {
        return it->second;
    } else {
        // not found, return 0 pointer.
        return EntryPtr();
    }
}

ImageCache::RequestPtr ImageCache::requestAsyncImage(const std::string & filename)
{
    // see if we have a request already
    std::map<std::string, RequestPtr>::iterator it = m_requests.find(filename);
    if (it != m_requests.end()) {
        // return a copy of the existing request.
        return it->second;
    } else {
        bool need_thread = m_requests.empty() && m_smallRequests.empty();
        // Make a new request.
        RequestPtr request = RequestPtr(new Request(filename, false));
        m_requests[filename] = request;
        if (need_thread) {
            spawnAsyncThread();
        }
        return request;
    }
}

ImageCache::RequestPtr ImageCache::requestAsyncSmallImage(const std::string & filename)
{
    // see if we have a request already
    std::map<std::string, RequestPtr>::iterator it = m_smallRequests.find(filename);
    if (it != m_smallRequests.end()) {
        // return a copy of the existing request.
        return it->second;
    } else {
        // Make a new request.
        bool need_thread = m_requests.empty() && m_smallRequests.empty();
        RequestPtr request = RequestPtr(new Request(filename, true));
        m_smallRequests[filename] = request;
        if (need_thread) {
            spawnAsyncThread();
        }
        return request;
    }
}

void ImageCache::postEvent(RequestPtr request, EntryPtr entry)
{
    // This is called in the main thread, but the request and entry came from
    // the background loading thread, which will close itself now.
    bool is_small_request = request->getIsSmall();
    const std::string & filename = request->getFilename();
    // Put the loaded image in the cache.
    if (is_small_request) {
        std::string name = filename+std::string(":small");
        images[name] = entry;
    } else {
        images[filename] = entry;
    }
    entry->lastAccess = m_accessCounter;
    // Remove all the completed and no longer wanted requests from the queues.
    // We need to check everything, as images can be loaded synchronously after
    // an asynchronous request for it was made, and also something could have
    // given up waiting (e.g. when the user switches images faster than they
    // load).
    // Take this opportunity to give out the signals, for the image just loaded
    // and anything else we spot.
    for (std::map<std::string, RequestPtr>::iterator it = m_smallRequests.begin();
         it != m_smallRequests.end();)
    {
        std::map<std::string, RequestPtr>::iterator next_it = it;
        next_it++;
        if (it->second.unique()) {
            // Last copy of the request is in the list.
            // Anything requesting it must have given up waiting.
            m_smallRequests.erase(it);
            
        } else if (getSmallImageIfAvailable(it->first).get()) {
            // already loaded.
            // signal to anything waiting and remove from the list.
            it->second->ready(getSmallImage(it->first), it->first, true);
            m_smallRequests.erase(it);
        }
        it = next_it;
    }
    for (std::map<std::string, RequestPtr>::iterator it = m_requests.begin();
         it != m_requests.end();)
    {
        std::map<std::string, RequestPtr>::iterator next_it = it;
        next_it++;
        if (it->second.unique()) {
            // The last copy of the request is in the list of requests.
            // Anything that requested it must have given up waiting.
            // Forget about it without loading.
            m_requests.erase(it);
        } else if (getImageIfAvailable(it->first).get()) {
            // already loaded.
            // Signal to anything waiting.
            it->second->ready(getImage(it->first), it->first, false);
            m_requests.erase(it);
        }
        it = next_it;
    }
    // If there are more images to load, start the thread again.
    if (!(m_requests.empty() && m_smallRequests.empty())) {
        // Start a background thread to load another image.
        spawnAsyncThread();
    }
}

void ImageCache::spawnAsyncThread()
{
    // Pick an image to load.
    // Try the small images first.
    std::map<std::string, RequestPtr>::iterator it = m_smallRequests.begin();
    if (it == m_smallRequests.end()) {
        it = m_requests.begin();
        if (it == m_requests.end())
        {
            DEBUG_DEBUG("Not staring a thread to load an image, since no images are wanted.");
        } else {
            boost::thread(loadSafely, it->second, EntryPtr());
        }
    } else {
        // got a small image request, check if its larger version has loaded.
        const std::string & filename = it->second->getFilename();
        EntryPtr large = getImageIfAvailable(filename);
        if (large.get() == 0)
        {
            // the larger one is needed to generate it.
            RequestPtr request(new Request(filename, false));
            boost::thread(loadSafely, request, EntryPtr());
        } else {
            // we have the large image.
            boost::thread(loadSafely, it->second, large);
        }
    }
    // thread should be processing the image asynchronously now.
    // Only one image is done at a time, so very little can go wrong between the
    // threads. The loading thread does not need to alter the ImageCache, and
    // there is a time when we can safely pick Requests which haven't started
    // loading, without giving images and lists mutexes.
}

void ImageCache::loadSafely(ImageCache::RequestPtr request, EntryPtr large)
{
    // load the image
    EntryPtr new_entry;
    if (large.get())
    {
        new_entry = loadSmallImageSafely(large);
    } else {
        new_entry = loadImageSafely(request->getFilename());
    }
    // pass an event with the load image and request, which can get picked up by
    // the main thread later. This could be a wxEvent for example.
    // Check if it exists, to avoid crashing in odd cases.
    if (getInstance().asyncLoadCompleteSignal)
    {
        (*getInstance().asyncLoadCompleteSignal)(request, new_entry);
    } else {
        DEBUG_ERROR("Please set HuginBase::ImageCache::getInstance().asyncLoadCompleteSignal to handle asynchronous image loads.");
    }
}

} //namespace

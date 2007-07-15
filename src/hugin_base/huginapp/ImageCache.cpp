// -*- c-basic-offset: 4 -*-

/** @file ImageCache.cpp
 *
 *  @brief implementation of ImageCache Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: ImageCache.cpp 1997 2007-05-09 21:14:15Z dangelo $
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

// ImageCache::GetRange implementation
#define VIGRA_EXT_GETRANGE(T1, MI,MA) \
template<> \
T1 ImageCache::GetRange<T1>::min() \
{ \
        return MI; \
} \
template<> \
T1 ImageCache::GetRange<T1>::max() \
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



ImageCache * ImageCache::instance = NULL;

ImageCache& ImageCache::getInstance()
{
    if (instance == NULL)
    {
        instance = new ImageCache();
    }
    return *instance;
}



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
    long upperBound = 100 * 1024 * 1024l;
    long purgeToSize = long(0.75 * upperBound);

    // calculate used memory
    long imgMem = 0;

    std::map<std::string, EntryPtr>::iterator imgIt;
    for(imgIt=images.begin(); imgIt != images.end(); imgIt++) {
        cout << "Image: " << imgIt->first << std::endl;
        cout << "CacheEntry: " << imgIt->second.use_count() << "last access: " << imgIt->second->lastAccess;
        if (imgIt->second->image8) {
            imgMem += imgIt->second->image8->width() * imgIt->second->image8->height() * 3;
            cout << " 8bit: " << imgIt->second->image8.use_count();
        }
        if (imgIt->second->imageFloat) {
            imgMem += imgIt->second->imageFloat->width() * imgIt->second->imageFloat->height() * 3 * 4;
            cout << " float: " << imgIt->second->imageFloat.use_count() ;
        }
        if (imgIt->second->mask) {
            imgMem += imgIt->second->mask->width() * imgIt->second->mask->height();
            cout << " mask: " << imgIt->second->mask.use_count() << std:: endl;
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
                if (it != images.end()) {
                    DEBUG_DEBUG("soft flush: removing image: " << it->first);
                    if (it->second->image8) {
                        purgedMem += it->second->image8->width() * it->second->image8->height() * 3;
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
        cout << "purged: " << (purgedMem>>20) << " MB, memory used for images: " << ((usedMem - purgedMem)>>20) << " MB" << endl;

    }
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


void ImageCache::convertTo8Bit(vigra::FRGBImage & src, std::string origType, vigra::BRGBImage & dest)
{
    // code to apply the mapping to 8 bit
    // always scaled from 0..1 for integer images.

    dest.resize(src.size());

    double min=0;
    double max=1;

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
        transformImage(dest.first, dest.first+ vigra::Diff2D(info.width(), info.height()),  dest.second, 
                       dest.first, dest.second,
                       Arg1()*Param(scale));
    }
}

ImageCache::EntryPtr ImageCache::getImage(const std::string & filename,
                                        bool integer)
{
//    softFlush();
    m_accessCounter++;
    std::map<std::string, EntryPtr>::iterator it;
    it = images.find(filename);
    if (it != images.end()) {
        it->second->lastAccess = m_accessCounter;
        if (integer && it->second->image8->width() == 0) {
            // convert to 8 bit
            it->second->image8->resize(it->second->imageFloat->size());
            convertTo8Bit(*(it->second->imageFloat),
                          it->second->origType,
                          *(it->second->image8));
        }
        return it->second;
    } else {
        if (m_progress) {
            char* message;
            sprintf(message, "Loading image %s",hugin_utils::stripPath(filename).c_str());
            m_progress->pushTask(AppBase::ProgressTask(std::string(message), "", 0));
        }
#if 1
        // load images with VIGRA impex, and store either 8 bit or float images
        std::string pixelTypeStr;
        ImageCacheRGB8Ptr img8(new vigra::BRGBImage);
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
            } else {
                imgFloat->resize(info.size());
            }

//wxString pixelTypeWX(pixelType, *wxConvCurrent);
            if ( bands == 1) {
                // load and convert image to 8 bit, if needed
                if (strcmp(pixelType, "UINT8") == 0 ) {
                    vigra::importImage(info, destImage(*img8,
                                                       vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)));
                    copyImage(srcImageRange(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)),
                              destImage(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(1)));
                    copyImage(srcImageRange(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(0)),
                              destImage(*img8, vigra::VectorComponentAccessor<vigra::RGBValue<vigra::UInt8> >(2)));
                } else {
                    if (strcmp(pixelType, "INT16") == 0 ) {
                        importAndConvertImage<vigra::Int16> (info, destImage(*imgFloat,
                                vigra::VectorComponentAccessor<vigra::RGBValue<float> >(0)), pixelType);
                    } else if (strcmp(pixelType, "UINT16") == 0 ) {
                        importAndConvertImage<vigra::UInt16>(info, destImage(*imgFloat,
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
                        DEBUG_FATAL("Unsupported pixel type: " << pixelType);
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
                } else if (strcmp(pixelType, "INT16") == 0 ) {
                    importAndConvertImage<vigra::RGBValue<vigra::Int16> > (info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "UINT16") == 0 ) {
                    importAndConvertImage<vigra::RGBValue<vigra::UInt16> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "UINT32") == 0 ) {
                    importAndConvertImage<vigra::RGBValue<vigra::UInt32> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "INT32") == 0 ) {
                    importAndConvertImage<vigra::RGBValue<vigra::Int32> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "FLOAT") == 0 ) {
                    importAndConvertImage<vigra::RGBValue<float> >(info, destImage(*imgFloat), pixelType);
                } else if (strcmp(pixelType, "DOUBLE") == 0 ) {
                    importAndConvertImage<vigra::RGBValue<double> >(info, destImage(*imgFloat), pixelType);
                } else {
                    DEBUG_FATAL("Unsupported pixel type: " << pixelType);
                }
            } else if ( bands == 4 && extraBands == 1) {
                // load and convert image to 8 bit, if needed
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
           DEBUG_FATAL("Error during image reading: " << e.what());
            throw;
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

        if (integer && img8->width() == 0) {
            // convert to 8 bit
            convertTo8Bit(*imgFloat, pixelTypeStr, *img8);
        }
        EntryPtr e(new Entry(img8, imgFloat, mask, pixelTypeStr));
        images[filename] = e;
        e->lastAccess = m_accessCounter;
        return e;
    }
}

ImageCache::EntryPtr ImageCache::getSmallImage(const std::string & filename,
                                               bool integer)
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
            char* message;
            sprintf(message, "Scaling image %s",hugin_utils::stripPath(filename).c_str());
            m_progress->pushTask(AppBase::ProgressTask(std::string(message), "", 0));
        }
        DEBUG_DEBUG("creating small image " << name );
        EntryPtr entry = getImage(filename);
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
        e->lastAccess = m_accessCounter;
        if (entry->imageFloat->width() != 0 ) {
            e->imageFloat = ImageCacheRGBFloatPtr(new vigra::FRGBImage);
            if (entry->mask->width() != 0) {
                vigra_ext::reduceNTimes(*(entry->imageFloat), *(entry->mask), *(e->imageFloat), *(e->mask), nLevel);
            } else {
                vigra_ext::reduceNTimes(*(entry->imageFloat), *(e->imageFloat), nLevel);
            }
        }
        if (entry->image8->width() != 0) {
            e->image8 = ImageCacheRGB8Ptr(new vigra::BRGBImage);
            if ((entry->mask->width() != 0) && (e->mask->width() == 0)  ) {
                vigra_ext::reduceNTimes(*(entry->image8), *(entry->mask), *(e->image8), *(e->mask), nLevel);
            } else {
                vigra_ext::reduceNTimes(*(entry->image8), *(e->image8), nLevel);
            }
        }
        if (integer && e->image8->width() == 0) {
            // convert to 8 bit
            convertTo8Bit(*(e->imageFloat), e->origType, *(e->image8));
        }
        images[name] = e;
        DEBUG_INFO ( "created small image: " << name);
        if (m_progress) {
            m_progress->popTask();
        }
        return e;
    }
}

#if 0
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
                    EntryPtr e = getImageWX(filename, srcImg);
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

#endif


} //namespace
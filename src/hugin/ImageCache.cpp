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

#include "panoinc_WX.h"

#include "panoinc.h"

#include <vigra/basicimage.hxx>
#include <vigra/basicimageview.hxx>
#include <vigra/rgbvalue.hxx>
#include "vigra_ext/Pyramid.h"

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
    images.clear();
}

void ImageCache::softFlush()
{
    long upperBound = wxConfigBase::Get()->Read("/ImageCache/UpperBound", 75 * 1024 * 1024l);
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



ImagePtr ImageCache::getImage(const std::string & filename)
{
//    softFlush();

    std::map<std::string, wxImage *>::iterator it;
    it = images.find(filename);
    if (it != images.end()) {
        return it->second;
    } else {
        if (m_progress) {
            char *str = wxT("Loading image");
            m_progress->pushTask(ProgressTask(wxString::Format("%s %s",str,utils::stripPath(filename).c_str()).c_str(), "", 0));
        }
#if 0
        // load images with VIGRA impex, and scale to 8 bit
        ImageImportInfo info(filename.c_str());
        if (info.numBands() < 3) {
            // greyscale image
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
            m_progress->pushTask(ProgressTask(_("Scaling image"), utils::stripPath(filename).c_str(), 0));
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
                      m_progress->pushTask(ProgressTask("creating grayscale",filename.c_str(), 0));
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
                        m_progress->pushTask(ProgressTask(wxString::Format("Creating pyramid image for %s, level %d",filename.c_str(), key.level).c_str(),"",0));
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
    // return old image, if already in cache
        if (set_contains(m_images, imgNr)) {
            DEBUG_DEBUG("using cached remapped image " << imgNr);
            return m_images[imgNr];
        }

        // remap image
        DEBUG_DEBUG("remapping image " << imgNr);

        // load image
        const PanoImage & img = pano.getImage(imgNr);
        wxImage * src = ImageCache::getInstance().getSmallImage(img.getFilename().c_str());
        if (!src->Ok()) {
            throw std::runtime_error("could not retrieve small source image for preview generation");
        }
        // image view
        BasicImageView<RGBValue<unsigned char> > srcImg((RGBValue<unsigned char> *)src->GetData(),
                                                        src->GetWidth(),
                                                        src->GetHeight());
        // mask image
        BImage srcAlpha(src->GetWidth(), src->GetHeight(), 255);

        MRemappedImage *remapped = new MRemappedImage;
        remapped->remapImage(pano, opts,
                             srcImageRange(srcImg),
                             srcImage(srcAlpha),
                             imgNr, progress);
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


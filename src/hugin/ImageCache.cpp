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

#include <string>

#include "common/utils.h"
#include "hugin/ImageProcessing.h"
#include "hugin/ImageCache.h"
#include "hugin/huginApp.h"

using namespace std;
using namespace vigra;


ImageCache * ImageCache::instance = 0;

ImageCache::ImageCache()
{
}

ImageCache::~ImageCache()
{
    images.clear();
//    delete instance;
    instance = 0;
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

ImageCache & ImageCache::getInstance()
{
    if (!instance) {
        instance = new ImageCache();
    }
    return *instance;

}


ImagePtr ImageCache::getImage(const std::string & filename)
{
    std::map<std::string, wxImage *>::iterator it;
    it = images.find(filename);
    if (it != images.end()) {
        return it->second;
    } else {
        g_MainFrame->SetStatusText(wxString::Format("Loading image %s",filename.c_str()),0);
        wxImage * image = new wxImage(filename.c_str());
        if (!image->Ok()){
            DEBUG_ERROR("Can't load image: " << filename);
        }
        // FIXME where is RefCountNotifier deleted?
//        ImagePtr ptr(image, new RefCountNotifier<wxImage>(*this));
        images[filename] = image;
        return image;
    }
}

ImagePtr ImageCache::getImageSmall(const std::string & filename)
{
    std::map<std::string, wxImage *>::iterator it;
    // "_small" is only used internally
    string name = filename + string("_small");
    it = images.find(name);
    if (it != images.end()) {
        return it->second;
    } else {
        g_MainFrame->SetStatusText(wxString::Format("Scaling image %s",filename.c_str()),0);
        DEBUG_DEBUG("creating small image " << name );
        ImagePtr image = getImage(filename);
        if (image->Ok()) {
            wxImage small_image;
            if ( image->GetHeight() > image->GetWidth() ) {
                small_image = image->Scale(
                    (int)((float)image->GetWidth()/
                          (float)image->GetHeight()*512.0), 512);
            } else {
                small_image = image->Scale(
                    512, (int)((float)image->GetHeight()/
                               (float)image->GetWidth()*512.0));
            }
            wxImage * tmp = new wxImage( &small_image );
            images[name] = tmp;
            DEBUG_INFO ( "created small image: " << name);
            return tmp;
        } else {
            return image;
        }
    }
}

const vigra::BImage & ImageCache::getPyramidImage(const std::string & filename,
                                                  int level)
{
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
                    g_MainFrame->SetStatusText(wxString::Format("Creating grayscale version of image %s",filename.c_str()),0);

                    vigra::copyImage(wxImageUpperLeft(*srcImg),
                                     wxImageLowerRight(*srcImg),
                                     RGBToGrayAccessor<RGBValue<unsigned char> >(),
                                     img->upperLeft(),
                                     BImage::Accessor());
                } else {
                    // reduce previous level to current level
                    DEBUG_DEBUG("reducing level " << key.level-1 << " to level " << key.level);
                    assert(img);
                    g_MainFrame->SetStatusText(wxString::Format("Creating pyramid image for %s, level %d",filename.c_str(), key.level),0);
                    BImage *smallImg = new BImage();
                    reduceToNextLevel(*img, *smallImg);
                    img = smallImg;
                }
                pyrImages[key.toString()]=img;
            }
        }
        // we have found our image
        return *img;
    }
}


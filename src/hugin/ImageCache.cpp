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
#include "include/common/utils.h"
#include "hugin/ImageCache.h"

ImageCache * ImageCache::instance = 0;


ImageCache::ImageCache()
{
}

ImageCache::~ImageCache()
{
    delete instance;
    instance = 0;
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
        wxImage * image = new wxImage(filename.c_str());
        if (!image->Ok()){
            DEBUG_ERROR("Can't load image: " << filename)
        }
        // FIXME where is RefCountNotifier deleted?
//        ImagePtr ptr(image, new RefCountNotifier<wxImage>(*this));
        images[filename] = image;

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
        images[filename + "_small"] = tmp;
        DEBUG_INFO ( "nu gibts 'n kleenes Bildi" )

        return image;
    }
}

ImagePtr ImageCache::getImageSmall(const std::string & filename)
{
    std::map<std::string, wxImage *>::iterator it;
    // "_small" is only used internally
    it = images.find(filename + "_small");
    if (it != images.end()) {
        return it->second;
    } else {
        DEBUG_ERROR("Can't load small image: " << filename);
    }
    wxImage * nix = new wxImage(10,10);
    return nix;
}

void ImageCache::notify(wxImage & img)
{
    DEBUG_DEBUG("ImageCache::notify for image 0x" << std::hex << &img);
}

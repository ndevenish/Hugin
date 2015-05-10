// -*- c-basic-offset: 4 -*-

/** @file wxImageCache.cpp
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "wxImageCache.h"

wxImage imageCacheEntry2wxImage(ImageCache::EntryPtr e)
{
    ImageCacheRGB8Ptr img = e->get8BitImage();
    if (img) {
        return wxImage(img->width(),
                       img->height(),
                       (unsigned char *) img->data(),
                       true);
    } else {
        // invalid wxImage
        return wxImage();
    }

}


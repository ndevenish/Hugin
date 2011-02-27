// -*- c-basic-offset: 4 -*-
/** @file wxImageCache.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _WXIMAGECACHE_H
#define _WXIMAGECACHE_H

#include <hugin_shared.h>
#include <map>
#include <panoinc_WX.h>
#include <PT/RemappedPanoImage.h>

#include <boost/shared_ptr.hpp>

#include <huginapp/ImageCache.h>
#include <huginapp/CachedImageRemapper.h>

typedef HuginBase::ImageCache::ImageCacheRGB8Ptr    ImageCacheRGB8Ptr;
typedef HuginBase::ImageCache::ImageCacheRGB16Ptr   ImageCacheRGB16Ptr;
typedef HuginBase::ImageCache::ImageCacheRGBFloatPtr ImageCacheRGBFloatPtr;
typedef HuginBase::ImageCache::ImageCache8Ptr       ImageCache8Ptr;

using HuginBase::ImageCache;
using HuginBase::SmallRemappedImageCache;

WXIMPEX wxImage imageCacheEntry2wxImage(ImageCache::EntryPtr e);

#endif // _IMAGECACHE_H

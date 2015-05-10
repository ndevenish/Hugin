// -*- c-basic-offset: 4 -*-
/** @file ImageCache.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _HUGINAPP_CACHEDIMAGEREMAPPER_H
#define _HUGINAPP_CACHEDIMAGEREMAPPER_H

#include <hugin_shared.h>
#include <nona/ImageRemapper.h>

#include <map>
#include <huginapp/ImageCache.h>

namespace HuginBase {

/** class to cache remapped images, loaded from the hugin small
 *  image cache.
 *
 *  This is meant to be used by the preview stitcher.
 */
class IMPEX SmallRemappedImageCache : public Nona::SingleImageRemapper<vigra::FRGBImage, vigra::BImage>
{
    
    typedef Nona::RemappedPanoImage<vigra::FRGBImage, vigra::BImage> MRemappedImage;
    
    
public:
    ///
    virtual ~SmallRemappedImageCache();
    
    
public:
    ///
    virtual MRemappedImage* getRemapped(const PanoramaData & pano,
                                        const PanoramaOptions & opts,
                                        unsigned int imgNr,
                                        vigra::Rect2D outputROI,
                                        AppBase::ProgressDisplay* progress);

    
    virtual	void release(MRemappedImage* d)
	{
		// NOP, will be done by invalidate..
	}
    
    /** invalidates all images */
    void invalidate();

    /** invalidate a specific image */
    void invalidate(unsigned int imgNr);

    
protected:
    std::map<unsigned, MRemappedImage*> m_images;
    
    // descriptions of the remapped image. useful to determine
    // if it has to be updated or not
    std::map<unsigned, SrcPanoImage> m_imagesParam;
    std::map<unsigned, PanoramaOptions> m_panoOpts;
    
};


} //namespace
#endif // _IMAGECACHE_H

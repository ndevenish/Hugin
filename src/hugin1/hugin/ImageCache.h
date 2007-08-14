// -*- c-basic-offset: 4 -*-
/** @file ImageCache.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: ImageCache.h 1988 2007-05-08 22:55:04Z dangelo $
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

#ifndef _IMAGECACHE_H
#define _IMAGECACHE_H

#include <map>
#include <panoinc_WX.h>
#include <PT/RemappedPanoImage.h>

#include <common/utils.h>
#include <boost/shared_ptr.hpp>

#ifdef _Hgn1_REMAPPEDPANOIMAGE_H
#include <huginapp/ImageCache.h>
#include <huginapp/CachedImageRemapper.h>

typedef HuginBase::ImageCache::ImageCacheRGB8Ptr    ImageCacheRGB8Ptr;
typedef HuginBase::ImageCache::ImageCacheRGB16Ptr   ImageCacheRGB16Ptr;
typedef HuginBase::ImageCache::ImageCacheRGBFloatPtr ImageCacheRGBFloatPtr;
typedef HuginBase::ImageCache::ImageCache8Ptr       ImageCache8Ptr;

using HuginBase::ImageCache;
using HuginBase::SmallRemappedImageCache;

wxImage imageCacheEntry2wxImage(ImageCache::EntryPtr e);

#else

// use reference counted pointers
typedef boost::shared_ptr<vigra::BRGBImage> ImageCacheRGB8Ptr;
typedef boost::shared_ptr<vigra::UInt16RGBImage> ImageCacheRGB16Ptr;
typedef boost::shared_ptr<vigra::FRGBImage> ImageCacheRGBFloatPtr;
typedef boost::shared_ptr<vigra::BImage> ImageCache8Ptr;


/** key for an image. used to find images, and to store access information.
 *
 *  Key is misnamed, because its more than just a key.
 */
struct ImageKey
{
    /// name of the image
    std::string name;
    /// producer (for special images)
    std::string producer;
    /// number of accesses
    int accesses;

    bool operator==(const ImageKey& o) const
        { return name == o.name && producer == o.producer; }
};


/** This is a cache for all the images we use.
 *
 *  is a singleton for easy access from everywhere.
 *  The cache is used as an image source, that needs
 *  to know how to reproduce the requested images, in case
 *  that they have been deleted.
 *
 *  @todo: implement a strategy for smart deletion of images
 *  @todo: add more advanced key, that stores access statistics
 *         and so on.
 */
class ImageCache
{
public:

    /** information about an image inside the cache */
    class Entry
    {
        public:

            ImageCacheRGB8Ptr image8;
            ImageCacheRGB16Ptr image16;
            ImageCacheRGBFloatPtr imageFloat;
            ImageCache8Ptr mask;

            std::string origType;
            int lastAccess;

            Entry()
            : image8(ImageCacheRGB8Ptr(new vigra::BRGBImage)),
              image16(ImageCacheRGB16Ptr(new vigra::UInt16RGBImage)),
              imageFloat(ImageCacheRGBFloatPtr(new vigra::FRGBImage)),
              mask(ImageCache8Ptr(new vigra::BImage))
            {
                DEBUG_TRACE("Constructing an empty ImageCache::Entry");
            };

            Entry(ImageCacheRGB8Ptr & img, 
                  ImageCacheRGB16Ptr & img16,
                  ImageCacheRGBFloatPtr & imgFloat,
                  ImageCache8Ptr & imgMask, const std::string & typ)
            : image8(img), image16(img16), imageFloat(imgFloat), mask(imgMask), origType(typ), lastAccess(0)
            { 
                DEBUG_TRACE("Constructing ImageCache::Entry");
            };

            ~Entry()
            {
                DEBUG_TRACE("Deleting ImageCacheEntry");
            }

            /** returns an 8 bit image, suitable for display on screen.
             *
             *  If only a 16 bit or float image is available, the 8 bit
             *  image is derived from it.
             */
            ImageCacheRGB8Ptr get8BitImage();
    };

    /** a shared pointer to the entry */
    typedef boost::shared_ptr<Entry> EntryPtr;

    /** dtor.
     */
    virtual ~ImageCache();

    /** get the global ImageCache object */
    static ImageCache & getInstance();

    /** get a image.
     *
     *  it will be loaded if its not already in the cache
     *
     *  Hold the EntryPtr as long as the image data is needed!
     */
    EntryPtr getImage(const std::string & filename);

    /** get an small image.
     *
     *  This image is 512x512 pixel maximum and can be used for icons
     *  and different previews. It is directly derived from the original.
     */
    EntryPtr getSmallImage(const std::string & filename);


    /** remove a specific image (and dependant images)
     * from the cache 
     */
    void removeImage(const std::string & filename);

    /** release all images in the cache.
     *
     *  useful on project load, or maybe before stitching really
     *  big pictures
     */
    void flush();

    /** a soft version of flush.
     *
     *  Releases some images if they go over a certain threshold
     */
    void softFlush();

    void setProgressDisplay(utils::MultiProgressDisplay * disp)
        {
            m_progress = disp;
        }

    /** get a pyramid image.
     *
     *  A image pyramid is a image in multiple resolutions.
     *  Usually it is used to accelerate image processing, by using
     *  lower resolutions first. they are properly low pass filtered,
     *  so no undersampling occurs (it would if one just takes
     *  every 2^level pixel instead).
     *
     *  @param filename of source image
     *  @param level of pyramid. height and width are calculated as
     *         follows: height/(level^2), width/(level^1)
     *
     */
//    const vigra::BImage & getPyramidImage(const std::string & filename,
//                                          int level);

private:
    /** ctor. private, nobody execpt us can create an instance.
     */
    ImageCache();

    static ImageCache * instance;

    std::map<std::string, EntryPtr> images;

    // key for your pyramid map.
    struct PyramidKey{
        PyramidKey(const std::string & str, int lv)
            : filename(str), level(lv) { }
        std::string toString()
            { return filename + utils::lexical_cast<std::string>(level); }
        std::string filename;
        int level;
    };
    std::map<std::string, vigra::BImage *> pyrImages;

    // our progress display
    utils::MultiProgressDisplay * m_progress;

    int m_accessCounter;
};


/** class to cache remapped images, loaded from the hugin small
 *  image cache.
 *
 *  This is meant to be used by the preview stitcher.
 */
class SmallRemappedImageCache : public PT::SingleImageRemapper<vigra::FRGBImage,
                                vigra::BImage>
{
    typedef PT::RemappedPanoImage<vigra::FRGBImage, vigra::BImage> MRemappedImage;
public:
    virtual ~SmallRemappedImageCache();

#if 0
    virtual
    MRemappedImage *
    getRemapped(const std::string & filename,
                const vigra::Diff2D & origSrcSize,
                const vigra::Diff2D & srcSize,
                PT::VariableMap srcVars,
                PT::Lens::LensProjectionFormat srcProj,
                PT::ImageOptions imgOpts,
                const vigra::Diff2D &destSize,
                PT::PanoramaOptions::ProjectionFormat destProj,
                double destHFOV,
                utils::MultiProgressDisplay & progress);
#endif

#ifdef _Hgn1_REMAPPEDPANOIMAGE_H
    virtual
    MRemappedImage *
    getRemapped(const HuginBase::PanoramaData & pano, const HuginBase::PanoramaOptions & opts,
                unsigned int imgNr, AppBase::MultiProgressDisplay& progress);
#else
    virtual
    MRemappedImage *
    getRemapped(const PT::Panorama & pano, const PT::PanoramaOptions & opts,
                unsigned int imgNr, utils::MultiProgressDisplay & progress);
#endif
    
    virtual	void
	release(MRemappedImage * d)
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
    std::map<unsigned, PT::SrcPanoImage> m_imagesParam;
    std::map<unsigned, PT::PanoramaOptions> m_panoOpts;
};

/** shallow copy of the 8 bit image contained in \p e
 *  \p e needs to be kept as long as this is 
 */
wxImage imageCacheEntry2wxImage(ImageCache::EntryPtr e);

#endif // _IMAGECACHE_H


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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _IMAGECACHE_H
#define _IMAGECACHE_H

#include <map>

#include <common/utils.h>
#include <vigra/stdimage.hxx>

#include <PT/ImageTransforms.h>

typedef wxImage * ImagePtr;


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
    /** dtor.
     */
    virtual ~ImageCache();

    /** get the global ImageCache object */
    static ImageCache & getInstance();

    /** get a image.
     *
     *  it will be loaded if its not already in the cache
     *
     *  Do not modify this image. Use a copy if it is really needed
     */
    ImagePtr getImage(const std::string & filename);

    /** get an small image version.
     *
     *  This image is 512x512 pixel maximum and can be used for icons
     *  and different previews. It is directly derived from the original.
     *
     *  @todo let selfdefined images been added belonging to the original one.
     *  @todo create substitute, remove commands
     *  @todo avoid smaller images as original
     */
    ImagePtr getSmallImage(const std::string & filename);

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
    const vigra::BImage & getPyramidImage(const std::string & filename,
                                          int level);

private:
    /** ctor. private, nobody execpt us can create an instance.
     */
    ImageCache();

    static ImageCache * instance;

    std::map<std::string, ImagePtr> images;

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
};


/** class to cache remapped images, loaded from the hugin small
 *  image cache.
 *
 *  This is meant to be used by the preview stitcher.
 */
class SmallRemappedImageCache : public PT::SingleImageRemapper<vigra::BRGBImage,
                                vigra::BImage>
{
    typedef PT::RemappedPanoImage<vigra::BRGBImage, vigra::BImage> MRemappedImage;
public:
    virtual ~SmallRemappedImageCache();

    virtual
    MRemappedImage &
    operator()(const PT::Panorama & pano, const PT::PanoramaOptions & opts,
               unsigned int imgNr, utils::MultiProgressDisplay & progress);

    /** nop
     */
    virtual void release() {};

    /** invalidates all images */
    void invalidate();

    /** invalidate a specific image */
    void invalidate(unsigned int imgNr);

protected:
    std::map<unsigned int, MRemappedImage*> m_images;
};



#endif // _IMAGECACHE_H

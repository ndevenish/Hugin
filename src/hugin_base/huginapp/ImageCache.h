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

#ifndef _HUGINAPP_IMAGECACHE_H
#define _HUGINAPP_IMAGECACHE_H

#include <hugin_shared.h>
#include "hugin_config.h"
#include <map>
#include <vector>
#include "hugin_utils/shared_ptr.h"
#ifdef HAVE_CXX11
#include <functional>
#else
#include <boost/function.hpp>
#endif
#include <vigra/stdimage.hxx>
#include <vigra/imageinfo.hxx>
#include <hugin_utils/utils.h>
#include <appbase/ProgressDisplay.h>

#define HUGIN_IMGCACHE_MAPPING_INTEGER        0l
#define HUGIN_IMGCACHE_MAPPING_FLOAT          1l


namespace HuginBase {
    
/** This is a cache for all the images we use.
 *
 *  is a singleton for easy access from everywhere.
 *  The cache is used as an image source, that needs
 *  to know how to reproduce the requested images, in case
 *  that they have been deleted.
 *
 */
class IMPEX ImageCache
{

    public:
        /// use reference counted pointers
        typedef sharedPtrNamespace::shared_ptr<vigra::BRGBImage> ImageCacheRGB8Ptr;
        typedef sharedPtrNamespace::shared_ptr<vigra::UInt16RGBImage> ImageCacheRGB16Ptr;
        typedef sharedPtrNamespace::shared_ptr<vigra::FRGBImage> ImageCacheRGBFloatPtr;
        typedef sharedPtrNamespace::shared_ptr<vigra::BImage> ImageCache8Ptr;
        typedef sharedPtrNamespace::shared_ptr<vigra::ImageImportInfo::ICCProfile> ImageCacheICCProfile;

        /** information about an image inside the cache */
        struct IMPEX Entry
        {
            ImageCacheRGB8Ptr image8;
            ImageCacheRGB16Ptr image16;
            ImageCacheRGBFloatPtr imageFloat;
            ImageCache8Ptr mask;
            ImageCacheICCProfile iccProfile;

            std::string origType;
            int lastAccess;

            public:
                ///
                Entry()
                  : image8(ImageCacheRGB8Ptr(new vigra::BRGBImage)),
                    image16(ImageCacheRGB16Ptr(new vigra::UInt16RGBImage)),
                    imageFloat(ImageCacheRGBFloatPtr(new vigra::FRGBImage)),
                    mask(ImageCache8Ptr(new vigra::BImage)),
                    iccProfile(ImageCacheICCProfile(new vigra::ImageImportInfo::ICCProfile)),
                    lastAccess(0)
                {
                      DEBUG_TRACE("Constructing an empty ImageCache::Entry");
                };

                ///
                Entry(ImageCacheRGB8Ptr & img, 
                      ImageCacheRGB16Ptr & img16,
                      ImageCacheRGBFloatPtr & imgFloat,
                      ImageCache8Ptr & imgMask,
                      ImageCacheICCProfile & ICCProfile,
                      const std::string & typ)
                  : image8(img), image16(img16), imageFloat(imgFloat), mask(imgMask),
                    iccProfile(ICCProfile), origType(typ), lastAccess(0)
                { 
                        DEBUG_TRACE("Constructing ImageCache::Entry");
                };

                ///
                ~Entry()
                {
                    DEBUG_TRACE("Deleting ImageCacheEntry");
                };

                ///
                ImageCacheRGB8Ptr get8BitImage();
        };

        /** a shared pointer to the entry */
        typedef sharedPtrNamespace::shared_ptr<Entry> EntryPtr;
        
        /** Request for an image to load
         *  Connect to the ready signal so when the image loads you can respond.
         */
        class Request
        {
            public:
                Request(std::string filename, bool request_small)
                    :m_filename(filename), m_isSmall(request_small)
                    {};
                /** Signal that fires when the image is loaded.
                 *  Function must return void and have three arguments: EntryPtr
                 *  for the requested image, std::string for the filename, and a
                 *  bool that is true iff this is a small image.
                 *
                 *  The image could be freed after the signal fires, but keeping
                 *  the EntryPtr prevents this.
                 */
#ifdef HAVE_CXX11
                std::vector <std::function<void(EntryPtr, std::string, bool)>> ready;
#else
                std::vector <boost::function<void(EntryPtr, std::string, bool)> > ready;
#endif
                bool getIsSmall() const
                    {return m_isSmall;};
                const std::string & getFilename() const
                    {return m_filename;};
            protected:
                std::string m_filename;
                bool m_isSmall;
        };
        
        /** Reference counted request for an image to load.
         *  Hold on to this when you want an image to load. If you no longer
         * want the image, just delete it. Deleting it before the image loads
         * lets other images load next.
         *
         * Connect to the ready signal to respond to the image loading. To keep
         * the image loaded, keep the EntryPtr given to the signal handler.
         *
         * It is reference counted, so you can freely copy and delete it.
         */
        typedef sharedPtrNamespace::shared_ptr<Request> RequestPtr;

    private:
        // ctor. private, nobody execpt us can create an instance.
        ImageCache()
            : asyncLoadCompleteSignal(0), upperBound(100*1024*1024l),
              m_progress(NULL), m_accessCounter(0)
        {};
        
    public:
        /** dtor.
         */
        virtual ~ImageCache()
        {
                images.clear();
                instance = NULL;
        }

        /** get the global ImageCache object */
        static ImageCache & getInstance();
        
    private:
        static ImageCache* instance;

        
    public:
        /** get a image.
         *
         *  it will be loaded if its not already in the cache
         *
         *  Hold the EntryPtr as long as the image data is needed!
         *
         *  If it isn't vital that the real image is obtained immediately, use
         *  getImageIfAvailable instead. This means you can keep the UI
         *  responsive while the real image is fetched from a disk or network
         *  and decoded.
         */
        EntryPtr getImage(const std::string & filename);
        
        /** Get an image if already loaded.
         *  If not already in the cache, the pointer returned is 0.
         *
         *  Hold a non-zero EntryPtr as long as the image data is needed.
         *
         *  If you really need the image immediately, use getImage() instead.
         */
        EntryPtr getImageIfAvailable(const std::string & filename);

        /** get an small image.
         *
         *  This image is 512x512 pixel maximum and can be used for icons
         *  and different previews. It is directly derived from the original.
         *
         *  If it isn't vital that the real image is obtained immediately, use
         *  getSmallImageIfAvailable instead. This means you can keep the UI
         *  responsive while the real image is fetched, decoded, and scaled.
         */
        EntryPtr getSmallImage(const std::string & filename);
        
        /** Get a small image if already loaded.
         *  The EntryPtr returned is 0 if the image isn't loaded yet.
         *
         *  This image is 512x512 pixels maximum and can be used for icons
         *  and different previews. It is directly derived from the original.
         *
         *  If you really need the image immediately, use getSmallImage()
         *  instead.
         */
        EntryPtr getSmallImageIfAvailable(const std::string & filename);
         
        /** Request an image be loaded.
         * This function returns quickly even when the image is not cached.
         *
         * @return Object to keep while you want the image. Connect to its
         * ready signal to be notified when the image is ready.
         */
        RequestPtr requestAsyncImage(const std::string & filename);
        
        /** Request a small image be loaded.
         * This function returns quickly even when the image is not cached.
         *
         * @return Object to keep while you want the image. Connect to its
         * ready signal to be notified when it is ready.
         */
        RequestPtr requestAsyncSmallImage(const std::string & filename);

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
		/** sets the upper limit, which is used by softFlush() 
		 */
		void SetUpperLimit(long newUpperLimit) { upperBound=newUpperLimit; };
        
        /** Signal for when a asynchronous load completes.
         *  If you use the requestAsync functions, ensure there is something
         *  connected to this signal. The signal is raised in another thread,
         *  so the handler must be thread safe.
         *
         *  The signal handler must pass the request and entry to postEvent from
         *  the main thread when it is safe. For example, it you can wrap the
         *  request and entry in some wxEvent and the main thread can handle it
         *  later.
         */
        void (*asyncLoadCompleteSignal)(RequestPtr, EntryPtr);
        
        /** Pass on a loaded event for any images loaded asynchronously.
         *  Call from the main GUI thread when an ImageLoadedEvent occurs.
         *  The ImageLoadedEvent originates from async_load_thread.
         *
         *  @param request The RequestPtr from the ImageLoadedEvent.
         *  @param entry the EntryPtr from the ImageLoadedEvent.
         */
        void postEvent(RequestPtr request, EntryPtr entry);

    private:
        long upperBound;

        template <class SrcPixelType,
                  class DestIterator, class DestAccessor>
        static void importAndConvertImage(const vigra::ImageImportInfo& info,
                                   vigra::pair<DestIterator, DestAccessor> dest,
                                   const std::string& type);
        
    //    template <class SrcPixelType,
    //              class DestIterator, class DestAccessor>
    //    void importAndConvertGrayImage(const ImageImportInfo& info,
    //                                   vigra::pair<DestIterator, DestAccessor> dest,
    //                                   wxString type);
        
    //    template <class SrcPixelType,
    //              class DestIterator, class DestAccessor>
    //    void importAndConvertGrayAlphaImage(const ImageImportInfo & info,
    //                                        vigra::pair<DestIterator, DestAccessor> dest,
    //                                        wxString type);
        
        template <class SrcPixelType,
                  class DestIterator, class DestAccessor,
                  class MaskIterator, class MaskAccessor>
        static void importAndConvertAlphaImage(const vigra::ImageImportInfo & info,
                                        vigra::pair<DestIterator, DestAccessor> dest,
                                        vigra::pair<MaskIterator, MaskAccessor> mask,
                                        const std::string & type);
        
        
    public:
        ///
        void setProgressDisplay(AppBase::ProgressDisplay* disp)
            { m_progress = disp; }
        
        ///
        void clearProgressDisplay(AppBase::ProgressDisplay* disp)
            { m_progress = NULL; }
        
        
    private:
        std::map<std::string, EntryPtr> images;

        // our progress display
        AppBase::ProgressDisplay* m_progress;

        int m_accessCounter;
        
        // Requests for full size images that need loading
        std::map<std::string, RequestPtr> m_requests;
        
        // Requests for small images that need generating.
        std::map<std::string, RequestPtr> m_smallRequests;
        
        /// Start a background thread to load an image.
        void spawnAsyncThread();
        
        /** Load a requested image in a way that will work in parallel.
         *  When done, it sends an event with the newly created EntryPtr and
         *  request.
         *  @param RequestPtr request for the image to load.
         *  @param large EntryPtr for the large image when a small image is to
         *               be generated from it. Use a 0 pointer (the default) to
         *               generate a full size image.
         */
        static void loadSafely(RequestPtr request, EntryPtr large = EntryPtr());
        
        /** Load a full size image, in a way that will work in parallel.
         *  If the image cannot be loaded, the pointer returned is 0.
         */
        static EntryPtr loadImageSafely(const std::string & filename);
        
        /** Load a small image, in a way that will work in parallel.
         *  If the image cannot be loaded, the pointer returned is 0.
         * @param entry Large image to scale down.
         */
        static EntryPtr loadSmallImageSafely(EntryPtr entry);
        
    public:
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
    //    const vigra::BImage & getPyramidImage(const std::string& filename,
    //                                          int level);

    private:
        // key for your pyramid map.
        struct PyramidKey
        {
            std::string filename;
            int level;
            
            public:
                PyramidKey(const std::string& str, int lv)
                  : filename(str), level(lv)
                {};
            
                std::string toString();
        };
        
        std::map<std::string, vigra::BImage *> pyrImages;
};


} //namespace
#endif // _IMAGECACHE_H

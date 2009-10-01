/*
 *  Copyright (C) 2004 Andrew Mihal
 *
 *  This software is an extension of the VIGRA computer vision library.
 *  ( Version 1.2.0, Aug 07 2003 )
 *  You may use, modify, and distribute this software according
 *  to the terms stated in the LICENSE file included in
 *  the VIGRA distribution.
 *
 *  VIGRA is Copyright 1998-2002 by Ullrich Koethe
 *  Cognitive Systems Group, University of Hamburg, Germany
 *
 *  The VIGRA Website is
 *      http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/
 *  Please direct questions, bug reports, and contributions to
 *      koethe@informatik.uni-hamburg.de
 *
 *  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR
 *  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */
#ifndef VIGRA_EXT_CACHEDFILEIMAGE_HXX
#define VIGRA_EXT_CACHEDFILEIMAGE_HXX

#include <errno.h>
#include <map>
#include <iostream>
#include <list>
#include <stdio.h>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#endif

#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#endif

#ifdef __GW32C__
#define fseeko fseeko64
#undef malloc
#endif

#include <boost/static_assert.hpp>
#include <boost/pool/pool.hpp>

#include <vigra/error.hxx>
#include <vigra/metaprogramming.hxx>
#include <vigra/utilities.hxx>

using std::cout;
using std::cerr;
using std::endl;
using std::list;
using std::map;
using std::min;

#ifdef _WIN32
#define mktemp _mktemp
#endif

namespace vigra {

/** Abstract base class for CachedFileImages. */
class CachedFileImageBase {
public:
    virtual ~CachedFileImageBase() { }
    virtual int numBlocksAllocated() const = 0;
    virtual int numBlocksNeeded() const = 0;
    virtual void swapOutBlock() const = 0;
    virtual void printBlockStats() const = 0;
};

// Forward declaration.
template <class PIXELTYPE> class CachedFileImage;

/** A singleton that manages memory for several CachedFileImages.
 */
class CachedFileImageDirector {
public:

    ~CachedFileImageDirector() {
        // This is no longer necessary. temp files get unlinked on creation.
        //// Make sure all image caches get destroyed and
        //// temp backing files get deleted.
        //if (!imageList.empty()) {
        //    cout << endl << "Cleaning up temporary files." << endl;
        //    while (!imageList.empty()) {
        //        CachedFileImageBase const *image = imageList.front();
        //        //cout << "deleting image " << image << endl;
        //        // Remember that this delete call modifies imageList.
        //        delete image;
        //    }
        //}
        //delete pool;
    }

    // Obtain a reference to the singleton.
    static CachedFileImageDirector &v() {
        static CachedFileImageDirector instance;
        return instance;
    }

    // Obtain a pointer to a blocksize chunk of memory.
    void* allocateBlock() {
        void *block = pool->malloc();
        // Prevent the pool from allocating more than one block
        // at a time from the system. Otherwise the pool will grab
        // more blocks than managedBlocks.
        pool->set_next_size(1);
        if (block == NULL) throw std::bad_alloc();
        return block;
    }

    void deallocateBlock(void* block) {
        if (block != NULL) pool->free(block);
    }

    // Set the number of bytes this director manages.
    void setAllocation(long long bytes) {
        // This may not be changed after images have been created.
        vigra_precondition(imageList.empty(), "CachedFileImageDirector: "
                "attempt to change allocation after images have already "
                "been created.");
        managedBytes = bytes;
        // Recalculate the number of blocks available.
        managedBlocks = (int)ceil(managedBytes / (double)blocksize);
        blocksAvailable = managedBlocks;
        //cout << "director.setAllocation:  managedBlocks=" << managedBlocks
        //     << " blocksAvailable=" << blocksAvailable << endl;
    }

    // Set the cache block size. This is the minimum amount that is
    // moved between the caches and the backing files.
    void setBlockSize(int bytes) {
        // This may not be changed after images have been created.
        vigra_precondition(imageList.empty(), "CachedFileImageDirector: "
                "attempt to change block size after images have already "
                "been created.");
        blocksize = bytes;
        delete pool;
        pool = new boost::pool<>(blocksize);
        // Recalculate the number of blocks available.
        managedBlocks = (int)ceil(managedBytes / (double)blocksize);
        blocksAvailable = managedBlocks;
        //cout << "director.setBlockSize:  managedBlocks=" << managedBlocks
        //     << " blocksAvailable=" << blocksAvailable << endl;
    }

    long long getAllocation() const {
        return managedBytes;
    }

    // Get the cache block size.
    int getBlockSize() const {
        return blocksize;
    }

    int getManagedBlocks() const {
        return managedBlocks;
    }

    int getBlocksAvailable() const {
        return blocksAvailable;
    }

    // Request a certain number of blocks for a new image.
    // Returns the number of blocks the new image may use.
    int requestBlocksForNewImage(int blocks, CachedFileImageBase const * image) {
        //cout << "director.requestBlocksForNewImage(blocks=" << blocks
        //     << " image=" << image << endl;
        int blocksAllocated = 0;
        if (blocksAvailable > 0) {
            blocksAllocated = 1;
            blocksAvailable--;
        } else {
            // Zero blocks available.
            // Try to free a block by forcing another image to swap.
            blocksAvailable += freeBlock(image);
            if (blocksAvailable == 0) {
                // Attempt to free a block was a failure.
                vigra_fail("CachedFileImageDirector::requestBlocksForNewImage(): "
                        "no blocks available and attempt to free blocks failed.");
            }
            blocksAllocated = blocksAvailable;
            blocksAvailable = 0;
        }

        // Register the new image.
        // By placing it at the back, it is marked as the
        // most-recently-swapped image.
        imageList.push_back(image);

        // Initialize the cache miss counter for this image.
        imageToMissMap[image] = 0LL;

        //cout << "director.requestBlocksForNewImage: blocksAllocated="
        //     << blocksAllocated << " blocksAvailable=" << blocksAvailable << endl;
        //cout << "director.imageList=";
        //std::copy(imageList.begin(), imageList.end(),
        //        std::ostream_iterator<CachedFileImageBase const *>(cout, " "));
        //cout << endl;

        return blocksAllocated;
    }

    // Unregister a CachedFileImage with the director.
    void returnBlocksUnregisterImage(int blocks, CachedFileImageBase const * image) {
        blocksAvailable += blocks;
        imageList.remove(image);
        //cout << "director.returnBlocks: blocks=" << blocks
        //     << " image=" << image << endl;
        //cout << "director.returnBlocks: blocksAvailable=" << blocksAvailable
        //     << endl;
        //cout << "director.returnBlocks: imageList=";
        //std::copy(imageList.begin(), imageList.end(),
        //        std::ostream_iterator<CachedFileImageBase const *>(cout, " "));
        //cout << endl;
    }

    // Tell the director that a cache miss has occured for this image.
    // The director may decide to allocate more blocks to the image.
    // If so it returns a nonzero number.
    int registerCacheMiss(CachedFileImageBase const * image) {
        cacheMisses++;
        imageToMissMap[image]++;

        // Remove image from list.
        imageList.remove(image);

        if (blocksAvailable == 0) {
            // Try to free a block from an image that has not
            // missed recently. If this fails then we just won't
            // give the calling image more blocks.
            blocksAvailable += freeBlock(image);
        }

        // Add image to back of list.
        // This marks it most-recently-swapped
        imageList.push_back(image);

        if (blocksAvailable > 0) {
            // There are more blocks available to give out.
            // Give the image one more block.
            blocksAvailable--;
            //cout << "director.registerCacheMiss(image=" << image << ")"
            //     << " blocksAvailable=" << blocksAvailable << " return 1" << endl;
            return 1;
        } else {
            //cout << "director.registerCacheMiss(image=" << image << ")"
            //     << " blocksAvailable=" << blocksAvailable << " return 0" << endl;
            return 0;
        }
    }

    // How many cache misses have occured in total.
    long long getCacheMisses() {
        return cacheMisses;
    }

    // How many cache misses have occured for the given image.
    long long getCacheMisses(CachedFileImageBase const * image) {
        return imageToMissMap[image];
    }

    // Reset all cache miss counters.
    void resetCacheMisses() {
        cacheMisses = 0LL;
        map<CachedFileImageBase const *, long long>::iterator i;
        for (i = imageToMissMap.begin(); i != imageToMissMap.end(); i++) {
            (*i).second = 0LL;
        }
    }

    // Print general stats about allocated blocks and cache misses.
    void printStats() {
        cout << "Summary: cache misses="
             << cacheMisses
             << "  blocks managed="
             << managedBlocks
             << "  allocated="
             << (managedBlocks - blocksAvailable)
             << "  free="
             << blocksAvailable
             << endl;
    }

    // Print stats about a particular image.
    void printStats(const char * imageName, const CachedFileImageBase * image) {
        cout << imageName << " " << image << ":"
             << " cache misses=" << imageToMissMap[image]
             << "  blocks allocated=" << image->numBlocksAllocated()
             << "  blocks required=" << image->numBlocksNeeded()
             << endl;
    }

    // Print stats about a particular image.
    // Add an integer suffix to the image's name.
    void printStats(const char * imageName, const int imageNumber,
            const CachedFileImageBase * image) {
        cout << imageName << imageNumber << " " << image << ":"
             << " cache misses=" << imageToMissMap[image]
             << "  blocks allocated=" << image->numBlocksAllocated()
             << "  blocks required=" << image->numBlocksNeeded()
             << endl;
    }

    // Print stats on all images.
    void printAllBlockStats() {
        list<CachedFileImageBase const *>::iterator i;
        for (i = imageList.begin(); i != imageList.end(); i++) {
            (*i)->printBlockStats();
        }
    }

protected:

    // Singleton constructor.
    CachedFileImageDirector()
    : blocksize(2<<20),
      managedBytes(1LL<<30),
      cacheMisses(0),
      imageList(),
      imageToMissMap()
    {
        // Recalculate the number of blocks available.
        managedBlocks = (int)ceil(managedBytes / (double)blocksize);
        blocksAvailable = managedBlocks;
        pool = new boost::pool<>(blocksize);
    }

    // Free a block for the given image.
    // Find another image with proportionally more blocks than image.
    int freeBlock(CachedFileImageBase const * image) {
        // Try to free a block from an image.
        // Check least-recently-missed images first.
        list<CachedFileImageBase const *>::iterator i;
        for (i = imageList.begin(); i != imageList.end(); i++) {
            CachedFileImageBase const * candidate = *i;
            //cout << "director.freeBlock candidate=" << candidate
            //     << " candidate_blocks=" << candidate->numBlocksAllocated()
            //     << " image_blocks=" << image->numBlocksAllocated() << endl;

            //// Don't try to break close ties - else if blocks cannot be divided up
            //// evenly the remainder will just get rotated around.
            //if (candidate->numBlocksAllocated() > (1+image->numBlocksAllocated())) {
            //    //cout << "director.freeBlock(image=" << image << ")"
            //    //     << " candidate=" << candidate << " return 1" << endl;
            //    candidate->swapOutBlock();
            //    // mark candidate as most-recently-missed.
            //    imageList.erase(i);
            //    imageList.push_back(candidate);
            //    return 1;
            //}

            // 20080221: If an image is at the front of the list, it has not missed recently.
            // Take a block from it. Don't move it to the end of the list. This only happens
            // if there is a miss on that image.
            if (candidate->numBlocksAllocated() > 0) {
                candidate->swapOutBlock();
                return 1;
            }
        }

        // Try harder to get another block for image?
        // Otherwise the thrash exception will be thrown.
        //if (image->numBlocksAllocated() == 1 && image->numBlocksNeeded() > 1) {
        //    // Look for images that have one block allocated and only need one
        //    // block. Make them swap. Put donor to rear of list.
        //}

        // Try even harder?
        // do the above and also turn off rotation constraint?

        //cout << "director.freeBlock(image=" << image << ") return 0" << endl;

        // No blocks could be freed from other images.
        return 0;
    }

    int blocksize;
    long long managedBytes;
    int managedBlocks;
    int blocksAvailable;
    long long cacheMisses;

    // Pool for blocks
    boost::pool<> *pool;

    // List of images.
    // Front is least-recently-missed image
    // Back is most-recently-missed image
    list<CachedFileImageBase const *> imageList;

    // Map of images to cache miss counters.
    map<CachedFileImageBase const *, long long> imageToMissMap;

};

/** A policy for iterating over all pixels in a CachedFileImage sequentially. */
template <class Iterator>
class CachedFileSequentialAccessIteratorPolicy
{
public:
    typedef Iterator BaseType;
    typedef typename Iterator::value_type value_type;
    typedef typename Iterator::difference_type::MoveX difference_type;
    typedef typename Iterator::reference reference;
    typedef typename Iterator::index_reference index_reference;
    typedef typename Iterator::pointer pointer;
    typedef typename Iterator::iterator_category iterator_category;

    static void initialize(BaseType & d) { }

    static reference dereference(BaseType const & d) {
        return *d;
    }

    static index_reference dereference(BaseType const & d, difference_type n) {
        int width = d.i->width();
        int dy = n / width;
        int dx = n % width;
        if (d.x() + dx >= width) {dy++; dx -= width;}
        else if (d.x() + dx < 0) {dy--; dx += width;}
        return d(dx, dy);
    }

    static bool equal(BaseType const & d1, BaseType const & d2) {
        int width1 = d1.i->width();
        int width2 = d2.i->width();
        return (d1.y()*width1 + d1.x()) == (d2.y()*width2 + d2.x());
    }

    static bool less(BaseType const & d1, BaseType const & d2) {
        int width1 = d1.i->width();
        int width2 = d2.i->width();
        return (d1.y()*width1 + d1.x()) < (d2.y()*width2 + d2.x());
    }

    static difference_type difference(BaseType const & d1, BaseType const & d2) {
        int width1 = d1.i->width();
        int width2 = d2.i->width();
        return (d1.y()*width1 + d1.x()) - (d2.y()*width2 + d2.x());
    }

    static void increment(BaseType & d) {
        ++d.x;
        if (d.x() == d.i->width()) {
            d.x = 0;
            ++d.y;
        }
    }

    static void decrement(BaseType & d) {
        --d.x;
        if (d.x() < 0) {
            d.x = d.i->width() - 1;
            --d.y;
        }
    }

    static void advance(BaseType & d, difference_type n) {
        int width = d.i->width();
        int dy = n / width;
        int dx = n % width;
        d.x += dx;
        d.y += dy;
        if (d.x() >= width) {++d.y; d.x -= width;}
        if (d.x() < 0) {--d.y; d.x += width;}
    }

};

namespace cfi_detail {

template <class StridedOrUnstrided, class T>
class DirectionSelector;

template <class T>
class DirectionSelector<UnstridedArrayTag, T>
{
public:
    DirectionSelector(T base=0) : current_(base) {}
    DirectionSelector(DirectionSelector const & rhs) : current_(rhs.current_) {}

    DirectionSelector & operator=(DirectionSelector const & rhs) {
        current_ = rhs.current_;
        return *this;
    }

    void operator++() { ++current_; }
    void operator++(int) { ++current_; }
    void operator--() { --current_; }
    void operator--(int) { --current_; }
    void operator+=(int dx) { current_ += dx; }
    void operator-=(int dx) { current_ -= dx; }

    bool operator==(DirectionSelector const & rhs) const { return (current_ == rhs.current_); }
    bool operator!=(DirectionSelector const & rhs) const { return (current_ != rhs.current_); }
    bool operator<(DirectionSelector const & rhs) const { return (current_ < rhs.current_); }
    bool operator<=(DirectionSelector const & rhs) const { return (current_ <= rhs.current_); }
    bool operator>(DirectionSelector const & rhs) const { return (current_ > rhs.current_); }
    bool operator>=(DirectionSelector const & rhs) const { return (current_ >= rhs.current_); }

    int operator-(DirectionSelector const & rhs) const { return (current_ - rhs.current_); }

    T operator()() const { return current_; }
    T operator()(int d) const { return (current_ + d); }

    T current_;
};

template <class T>
class DirectionSelector<StridedArrayTag, T> {
public:
    DirectionSelector(int stride=1, T base=0) : stride_(stride), current_(base) {}
    DirectionSelector(DirectionSelector const & rhs) : stride_(rhs.stride_), current_(rhs.current_) {}

    DirectionSelector & operator=(DirectionSelector const & rhs) {
        stride_ = rhs.stride_;
        current_ = rhs.current_;
        return *this;
    }

    void operator++() { current_ += stride_; }
    void operator++(int) { current_ += stride_; }
    void operator--() { current_ -= stride_; }
    void operator--(int) { current_ -= stride_; }
    void operator+=(int dy) { current_ += dy*stride_; }
    void operator-=(int dy) { current_ -= dy*stride_; }

    bool operator==(DirectionSelector const & rhs) const { return (current_ == rhs.current_); }
    bool operator!=(DirectionSelector const & rhs) const { return (current_ != rhs.current_); }
    bool operator<(DirectionSelector const & rhs) const { return (current_ < rhs.current_); }
    bool operator<=(DirectionSelector const & rhs) const { return (current_ <= rhs.current_); }
    bool operator>(DirectionSelector const & rhs) const { return (current_ > rhs.current_); }
    bool operator>=(DirectionSelector const & rhs) const { return (current_ >= rhs.current_); }

    int operator-(DirectionSelector const & rhs) const { return (current_ - rhs.current_) / stride_; }

    T operator()() const { return current_; }
    T operator()(int d) const { return current_ + d*stride_; }

    int stride_;
    T current_;
};

template <class StridedOrUnstrided, class T, class Notify>
class NotifyingDirectionSelector;

template <class T, class Notify>
class NotifyingDirectionSelector<UnstridedArrayTag, T, Notify>
{
#ifdef __GNUC__
friend class Notify::self_type;
#else
friend typename Notify::self_type;
#endif
protected:
    NotifyingDirectionSelector(T base = 0) : current_(base), notify_(NULL) {}
    NotifyingDirectionSelector(NotifyingDirectionSelector const & rhs) : current_(rhs.current_), notify_(NULL) {}

    void setNotify(Notify *n) { notify_ = n; }

public:
    NotifyingDirectionSelector & operator=(NotifyingDirectionSelector const & rhs) {
        notify_->_notify(current_, rhs.current_);
        current_ = rhs.current_;
        return *this;
    }

    void operator++() { notify_->_notify(current_, current_+1); ++current_; }
    void operator++(int) { notify_->_notify(current_, current_+1); ++current_; }
    void operator--() { notify_->_notify(current_, current_-1); --current_; }
    void operator--(int) { notify_->_notify(current_, current_-1); --current_; }
    void operator+=(int dx) { notify_->_notify(current_, current_+dx); current_ += dx; }
    void operator-=(int dx) { notify_->_notify(current_, current_-dx); current_ -= dx; }

    bool operator==(NotifyingDirectionSelector const & rhs) const { return (current_ == rhs.current_); }
    bool operator!=(NotifyingDirectionSelector const & rhs) const { return (current_ != rhs.current_); }
    bool operator<(NotifyingDirectionSelector const & rhs) const { return (current_ < rhs.current_); }
    bool operator<=(NotifyingDirectionSelector const & rhs) const { return (current_ <= rhs.current_); }
    bool operator>(NotifyingDirectionSelector const & rhs) const { return (current_ > rhs.current_); }
    bool operator>=(NotifyingDirectionSelector const & rhs) const { return (current_ >= rhs.current_); }

    int operator-(NotifyingDirectionSelector const & rhs) const { return (current_ - rhs.current_); }

    T operator()() const { return current_; }
    T operator()(int d) const { return (current_ + d); }

    T current_;

private:
    Notify *notify_;

};

template <class T, class Notify>
class NotifyingDirectionSelector<StridedArrayTag, T, Notify> {
#ifdef __GNUC__
friend class Notify::self_type;
#else
friend typename Notify::self_type;
#endif
protected:
    NotifyingDirectionSelector(int stride = 1, T base = 0) : stride_(stride), current_(base), notify_(NULL) {}
    NotifyingDirectionSelector(NotifyingDirectionSelector const & rhs) : stride_(rhs.stride_), current_(rhs.current_), notify_(NULL) {}

    void setNotify(Notify *n) { notify_ = n; }

public:
    NotifyingDirectionSelector & operator=(NotifyingDirectionSelector const & rhs) {
        notify_->_notify(current_, rhs.current_);
        stride_ = rhs.stride_;
        current_ = rhs.current_;
        return *this;
    }

    void operator++() { notify_->_notify(current_, current_+stride_); current_ += stride_; }
    void operator++(int) { notify_->_notify(current_, current_+stride_); current_ += stride_; }
    void operator--() { notify_->_notify(current_, current_-stride_); current_ -= stride_; }
    void operator--(int) { notify_->_notify(current_, current_-stride_); current_ -= stride_; }
    void operator+=(int dy) { notify_->_notify(current_, current_+dy*stride_); current_ += dy*stride_; }
    void operator-=(int dy) { notify_->_notify(current_, current_-dy*stride_); current_ -= dy*stride_; }

    bool operator==(NotifyingDirectionSelector const & rhs) const { return (current_ == rhs.current_); }
    bool operator!=(NotifyingDirectionSelector const & rhs) const { return (current_ != rhs.current_); }
    bool operator<(NotifyingDirectionSelector const & rhs) const { return (current_ < rhs.current_); }
    bool operator<=(NotifyingDirectionSelector const & rhs) const { return (current_ <= rhs.current_); }
    bool operator>(NotifyingDirectionSelector const & rhs) const { return (current_ > rhs.current_); }
    bool operator>=(NotifyingDirectionSelector const & rhs) const { return (current_ >= rhs.current_); }

    int operator-(NotifyingDirectionSelector const & rhs) const { return (current_ - rhs.current_) / stride_; }

    T operator()() const { return current_; }
    T operator()(int d) const { return current_ + d*stride_; }

    int stride_;
    T current_;

private:
    Notify *notify_;

};

} // namespace cfi_detail

/** Base class for CachedFileImage traversers. */
template <class IMAGEITERATOR, class IMAGETYPE, class PIXELTYPE, class REFERENCE, class POINTER,
          class StridedOrUnstrided=UnstridedArrayTag>
class CachedFileImageIteratorBase
{
public:
    typedef CachedFileImageIteratorBase<IMAGEITERATOR,
            IMAGETYPE, PIXELTYPE, REFERENCE, POINTER, StridedOrUnstrided> self_type;
    typedef IMAGETYPE image_type;
    typedef PIXELTYPE value_type;
    typedef PIXELTYPE PixelType;
    typedef REFERENCE reference;
    typedef REFERENCE index_reference;
    typedef POINTER pointer;
    typedef Diff2D difference_type;
    typedef image_traverser_tag iterator_category;
    typedef RowIterator<IMAGEITERATOR> row_iterator;
    typedef ColumnIterator<IMAGEITERATOR> column_iterator;
    typedef typename cfi_detail::DirectionSelector<StridedOrUnstrided, int> MoveX;
    friend class cfi_detail::DirectionSelector<StridedOrUnstrided, int>;
    typedef typename cfi_detail::NotifyingDirectionSelector<StridedOrUnstrided, int, self_type> MoveY;
    friend class cfi_detail::NotifyingDirectionSelector<StridedOrUnstrided, int, self_type>;
    friend class CachedFileSequentialAccessIteratorPolicy<IMAGEITERATOR>;

    MoveX x;
    MoveY y;

    IMAGEITERATOR & operator+=(difference_type const & s) {
        x += s.x;
        y += s.y;
        return static_cast<IMAGEITERATOR &>(*this);
    }

    IMAGEITERATOR & operator-=(difference_type const & s) {
        x -= s.x;
        y -= s.y;
        return static_cast<IMAGEITERATOR &>(*this);
    }

    IMAGEITERATOR operator+(difference_type const & s) const {
        IMAGEITERATOR ret(static_cast<IMAGEITERATOR const &>(*this));
        ret += s;
        return ret;
    }

    IMAGEITERATOR operator-(difference_type const & s) const {
        IMAGEITERATOR ret(static_cast<IMAGEITERATOR const &>(*this));
        ret -= s;
        return ret;
    }

    difference_type operator-(CachedFileImageIteratorBase const & rhs) const {
        return difference_type(x-rhs.x, y-rhs.y);
    }

    bool operator==(CachedFileImageIteratorBase const & rhs) const {
        return (x == rhs.x) && (y == rhs.y);
    }

    bool operator!=(CachedFileImageIteratorBase const & rhs) const {
        return (x != rhs.x) || (y != rhs.y);
    }

    reference operator*() const {
        //std::cout << "iterator=" << this << " currentRow=" << (void*)currentRow << " modifying pixel at (" << x << "," << y._y << ")  = " << (void*)(&currentRow[x]) << std::endl;
        return currentRow[x()];
        //return (*i)(x, y);
    }

    // FIXME pointer is supposed to be a weak_ptr
    pointer operator->() const {
        //BOOST_STATIC_ASSERT(false);
        return (*i)[y()] + x();
    }

    index_reference operator[](difference_type const & d) const {
        if (d.y == 0) {
            return currentRow[x(d.x)];
        } else {
            return (*i)(x(d.x), y(d.y));
        }
    }

    index_reference operator()(int dx, int dy) const {
        if (dy == 0) {
            return currentRow[x(dx)];
        } else {
            return (*i)(x(dx), y(dy));
        }
    }

    // FIXME pointer is supposed to be a weak_ptr
    pointer operator[](int dy) const {
        //BOOST_STATIC_ASSERT(false);
        return (*i)[y(dy)] + x();
    }

    row_iterator rowIterator() const {
        return row_iterator(static_cast<IMAGEITERATOR const &>(*this));
    }

    column_iterator columnIterator() const {
        return column_iterator(static_cast<IMAGEITERATOR const &>(*this));
    }

protected:

    CachedFileImageIteratorBase(const int X, const int Y, image_type * const I) : x(X), y(Y), i(I), currentRow(NULL) {
        y.setNotify(this);
        _notify(y());
    }

    // Constructor only for strided iterators
    CachedFileImageIteratorBase(const int X, const int Y, image_type * const I, int xstride, int ystride) : x(xstride, X), y(ystride, Y), i(I), currentRow(NULL) {
        y.setNotify(this);
        _notify(y());
    }

    CachedFileImageIteratorBase(const CachedFileImageIteratorBase &r) : x(r.x), y(r.y), i(r.i), currentRow(NULL) {
        y.setNotify(this);
        _notify(y());
    }

    CachedFileImageIteratorBase& operator=(const CachedFileImageIteratorBase &r) {
        // Note that we change i first so that the assignment to y causes us to get notified
        // about the new y coordinate in the new image.
        currentRow = NULL;
        i = r.i;
        x = r.x;
        y = r.y;
        return *this;
    }

    void _notify(int initialY) {
        // Y has been initialized to initialY.
        if (i) currentRow = (*i)[initialY];
        //std::cout << "iterator " << this << " _notify(" << initialY << ") currentRow=" << (void*)currentRow << std::endl;
    }

    void _notify(int oldY, int newY) {
        // Y has changed from oldY to newY
        if (i) currentRow = (*i)[newY];
        //std::cout << "iterator " << this << " _notify(" << oldY << ", " << newY << ") currentRow=" << (void*)currentRow << std::endl;
    }

    image_type *i;
    pointer currentRow;

};
 
/** Forward declarations */
template <class PIXELTYPE>
class StridedCachedFileImageIterator;

template <class PIXELTYPE>
class ConstStridedCachedFileImageIterator;

/** Regular CachedFileImage traverser. */
template <class PIXELTYPE>
class CachedFileImageIterator
: public CachedFileImageIteratorBase<CachedFileImageIterator<PIXELTYPE>,
                CachedFileImage<PIXELTYPE>,
                PIXELTYPE, PIXELTYPE &, PIXELTYPE *>
// FIXME this needs to be a weak_ptr    ^^^^^^^^^^^
// in case someone uses the iterator to get a pointer to cached data.
{
public:

    typedef CachedFileImageIteratorBase<CachedFileImageIterator,
            CachedFileImage<PIXELTYPE>,
            PIXELTYPE, PIXELTYPE &, PIXELTYPE *> Base;

    CachedFileImageIterator(const int x = 0,
                            const int y = 0,
                            CachedFileImage<PIXELTYPE> * const i = NULL)
    : Base(x, y, i)
    {}

    friend class StridedCachedFileImageIterator<PIXELTYPE>;
    friend class ConstStridedCachedFileImageIterator<PIXELTYPE>;
};

/** Traverser over const CachedFileImage. */
template <class PIXELTYPE>
class ConstCachedFileImageIterator
: public CachedFileImageIteratorBase<ConstCachedFileImageIterator<PIXELTYPE>,
                const CachedFileImage<PIXELTYPE>,
                PIXELTYPE, PIXELTYPE const &, PIXELTYPE const *>
// FIXME this needs to be a weak_ptr          ^^^^^^^^^^^^^^^^^
// in case someone uses the iterator to get a pointer to cached data.
{
public:

    typedef CachedFileImageIteratorBase<ConstCachedFileImageIterator,
            const CachedFileImage<PIXELTYPE>,
            PIXELTYPE, PIXELTYPE const &, PIXELTYPE const *> Base;
    // FIXME this needs to be a weak_ptr  ^^^^^^^^^^^^^^^^^

    ConstCachedFileImageIterator(const int x = 0,
                                 const int y = 0,
                                 const CachedFileImage<PIXELTYPE> * const i = NULL)
    : Base(x, y, i)
    {}

    // FIXME functions to copy CachedFileImageIterator to ConstCachedFileImageIterator are broken

    //ConstCachedFileImageIterator(CachedFileImageIterator<PIXELTYPE> const & rhs)
    //: Base(0, 0, NULL)
    //{
    //    Base::x = rhs.x;
    //    Base::y = rhs.y;
    //    Base::i = rhs.i;
    //    Base::currentRow = NULL;
    //    Base::y.setNotify(this);
    //    Base::_notify(Base::y());
    //}

    //ConstCachedFileImageIterator &
    //operator=(CachedFileImageIterator<PIXELTYPE> const & rhs)
    //{
    //    Base::x = rhs.x;
    //    Base::y = rhs.y;
    //    Base::i = rhs.i;
    //    Base::currentRow = NULL;
    //    Base::y.setNotify(this);
    //    Base::_notify(Base::y());
    //    return *this;
    //}

    friend class ConstStridedCachedFileImageIterator<PIXELTYPE>;
};

/** Regular CachedFileImage traverser. */
template <class PIXELTYPE>
class StridedCachedFileImageIterator
: public CachedFileImageIteratorBase<StridedCachedFileImageIterator<PIXELTYPE>,
                CachedFileImage<PIXELTYPE>,
                PIXELTYPE, PIXELTYPE &, PIXELTYPE *, StridedArrayTag>
// FIXME this needs to be a weak_ptr    ^^^^^^^^^^^
// in case someone uses the iterator to get a pointer to cached data.
{
public:

    typedef CachedFileImageIteratorBase<StridedCachedFileImageIterator,
            CachedFileImage<PIXELTYPE>,
            PIXELTYPE, PIXELTYPE &, PIXELTYPE *, StridedArrayTag> Base;

    StridedCachedFileImageIterator(const int x = 0,
                                   const int y = 0,
                                   CachedFileImage<PIXELTYPE> * const i = NULL,
                                   const int xstride = 1,
                                   const int ystride = 1)
    : Base(x, y, i, xstride, ystride)
    {}

    StridedCachedFileImageIterator(CachedFileImageIterator<PIXELTYPE> const & r,
                                   const int xstride,
                                   const int ystride)
    : Base(r.x(), r.y(), r.i, xstride, ystride)
    {}

};

/** Traverser over const CachedFileImage. */
template <class PIXELTYPE>
class ConstStridedCachedFileImageIterator
: public CachedFileImageIteratorBase<ConstStridedCachedFileImageIterator<PIXELTYPE>,
                const CachedFileImage<PIXELTYPE>,
                PIXELTYPE, PIXELTYPE const &, PIXELTYPE const *, StridedArrayTag>
// FIXME this needs to be a weak_ptr          ^^^^^^^^^^^^^^^^^
// in case someone uses the iterator to get a pointer to cached data.
{
public:

    typedef CachedFileImageIteratorBase<ConstStridedCachedFileImageIterator,
            const CachedFileImage<PIXELTYPE>,
            PIXELTYPE, PIXELTYPE const &, PIXELTYPE const *, StridedArrayTag> Base;
    // FIXME this needs to be a weak_ptr  ^^^^^^^^^^^^^^^^^

    ConstStridedCachedFileImageIterator(const int x = 0,
                                        const int y = 0,
                                        const CachedFileImage<PIXELTYPE> * const i = NULL,
                                        const int xstride = 1,
                                        const int ystride = 1)
    : Base(x, y, i, xstride, ystride)
    {}

    ConstStridedCachedFileImageIterator(ConstCachedFileImageIterator<PIXELTYPE> const & r,
                                        const int xstride,
                                        const int ystride)
    : Base(r.x(), r.y(), r.i, xstride, ystride)
    {}

    ConstStridedCachedFileImageIterator(CachedFileImageIterator<PIXELTYPE> const & r,
                                        const int xstride,
                                        const int ystride)
    : Base(r.x(), r.y(), r.i, xstride, ystride)
    {}

    // FIXME functions to copy StridedCachedFileImageIterator to ConstStridedCachedFileImageIterator are broken

    //ConstStridedCachedFileImageIterator(StridedCachedFileImageIterator<PIXELTYPE> const & rhs)
    //: Base(0, 0, NULL, 1, 1)
    //{
    //    Base::x = rhs.x;
    //    Base::y = rhs.y;
    //    Base::i = rhs.i;
    //    Base::currentRow = NULL;
    //    Base::y.setNotify(this);
    //    Base::_notify(Base::y());
    //}

    //ConstStridedCachedFileImageIterator &
    //operator=(StridedCachedFileImageIterator<PIXELTYPE> const & rhs)
    //{
    //    Base::x = rhs.x;
    //    Base::y = rhs.y;
    //    Base::i = rhs.i;
    //    Base::currentRow = NULL;
    //    Base::y.setNotify(this);
    //    Base::_notify(Base::y());
    //    return *this;
    //}

};

// Forward declaration.
template <class T> struct IteratorTraits;

/** Basic CachedFileImage */
template <class PIXELTYPE>
class CachedFileImage : public CachedFileImageBase {
public:

    typedef PIXELTYPE value_type;
    typedef PIXELTYPE PixelType;
    typedef PIXELTYPE & reference;
    typedef PIXELTYPE const & const_reference;
    // FIXME these need to be weak_ptrs
    typedef PIXELTYPE * pointer;
    typedef PIXELTYPE const * const_pointer;
    typedef CachedFileImageIterator<PIXELTYPE> traverser;
    typedef ConstCachedFileImageIterator<PIXELTYPE> const_traverser;
    typedef IteratorAdaptor<CachedFileSequentialAccessIteratorPolicy<traverser> > iterator;
    typedef IteratorAdaptor<CachedFileSequentialAccessIteratorPolicy<const_traverser> > const_iterator;
    typedef Diff2D difference_type;
    typedef Size2D size_type;
    typedef typename IteratorTraits<traverser>::DefaultAccessor Accessor;
    typedef typename IteratorTraits<const_traverser>::DefaultAccessor ConstAccessor;

    CachedFileImage() {
        initMembers();
    }

    CachedFileImage(int width, int height) {
        vigra_precondition((width >= 0) && (height >= 0),
                "CachedFileImage::CachedFileImage(int width, int height): "
                "width and height must be >= 0.\n");
        initMembers();
        resize(width, height, value_type());
    }

    explicit CachedFileImage(difference_type const & size, value_type const & d = value_type()) {
        vigra_precondition((size.x >= 0) && (size.y >= 0),
                "CachedFileImage::CachedFileImage(Diff2D size): "
                "size.x and size.y must be >= 0.\n");
        initMembers();
        resize(size.x, size.y, d);
    }

    CachedFileImage(int width, int height, value_type const & d) {
        vigra_precondition((width >= 0) && (height >= 0),
                "CachedFileImage::CachedFileImage(int width, int height, value_type const & ): "
                "width and height must be >= 0.\n");
        initMembers();
        resize(width, height, d);
    }

    CachedFileImage(const CachedFileImage & rhs) {
        initMembers();
        resizeCopy(rhs);
    }

    virtual ~CachedFileImage() {
        deallocate();
    }

    CachedFileImage & operator=(const CachedFileImage &rhs);
    CachedFileImage & init(value_type const & pixel);

    void resize(int width, int height) {
        resize(width, height, value_type());
    }

    void resize(difference_type const & size) {
        resize(size.x, size.y, value_type());
    }

    void resize(int width, int height, value_type const & d);
    void resizeCopy(const CachedFileImage & rhs);
    void swap( CachedFileImage<PIXELTYPE>& rhs );

    int width() const {
        return width_;
    }

    int height() const {
        return height_;
    }

    size_type size() const {
        return size_type(width(), height());
    }

    bool isInside(difference_type const & d) const {
        return d.x >= 0 && d.y >= 0 &&
               d.x < width() && d.y < height();
    }

    reference operator[](difference_type const & d) {
        return (getLinePointerDirty(d.y))[d.x];
    }

    const_reference operator[](difference_type const & d) const {
        return (getLinePointer(d.y))[d.x];
    }

    reference operator()(int dx, int dy) {
        return (getLinePointerDirty(dy))[dx];
    }

    const_reference operator()(int dx, int dy) const {
        return (getLinePointer(dy))[dx];
    }

    // FIXME - needs to return a weak_ptr
    pointer operator[](int dy) {
        //BOOST_STATIC_ASSERT(false);
        //cout << "fetching line pointer for row " << dy << endl;
        if (dy < 0 || dy >= height_) return NULL;
        else return getLinePointerDirty(dy);
    }

    // FIXME - needs to return a weak_ptr
    const_pointer operator[](int dy) const {
        //BOOST_STATIC_ASSERT(false);
        //cout << "fetching line pointer for row " << dy << endl;
        if (dy < 0 || dy >= height_) return NULL;
        else return getLinePointer(dy);
    }

    traverser upperLeft() {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::upperLeft(): image must have non-zero size.");
        return traverser(0, 0, this);
    }

    traverser lowerRight() {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::lowerRight(): image must have non-zero size.");
        return traverser(width_, height_, this);
    }

    const_traverser upperLeft() const {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::upperLeft(): image must have non-zero size.");
        return const_traverser(0, 0, this);
    }

    const_traverser lowerRight() const {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::lowerRight(): image must have non-zero size.");
        return const_traverser(width_, height_, this);
    }

    iterator begin() {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::begin(): image must have non-zero size.");
        return iterator(traverser(0, 0, this));
    }

    iterator end() {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::end(): image must have non-zero size.");
        return iterator(traverser(0, height_, this));
    }

    const_iterator begin() const {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::begin(): image must have non-zero size.");
        return const_iterator(const_traverser(0, 0, this));
    }

    const_iterator end() const {
        //vigra_precondition(width_ > 0 && height_ > 0,
        vigra_assert(width_ > 0 && height_ > 0,
                "CachedFileImage::end(): image must have non-zero size.");
        return const_iterator(const_traverser(0, height_, this));
    }

    Accessor accessor() {
        return Accessor();
    }

    ConstAccessor accessor() const {
        return ConstAccessor();
    }

    int numBlocksAllocated() const {
        return blocksAllocated_;
    }

    int numBlocksNeeded() const {
        return blocksNeeded_;
    }

    void printBlockStats() const {
        cout << "image " << this
             << " blocksAllocated=" << blocksAllocated_
             << "/" << blocksNeeded_
             << " mrl=" << *mostRecentlyLoadedBlockIterator_
             << " blocksInMemory={";

        std::copy(blocksInMemory_->begin(), blocksInMemory_->end(),
                std::ostream_iterator<int>(cout, " "));

        cout << "}" << endl;
    }

private:

    // Data value used to initialize previously unused pixels.
    PIXELTYPE initPixel;

    void deallocate();

    // Init the cache data structures once we know the size of the image.
    void initLineStartArray();
    
    // obtain a pointer to the beginning of a line.
    // split into two functions for efficiency.
    // getLinePointer can then be inlined, and we only incur the function
    // call overhead on cache misses.
    PIXELTYPE * getLinePointer(const int dy) const;
    PIXELTYPE * getLinePointerDirty(const int dy);
    PIXELTYPE * getLinePointerCacheMiss(const int dy) const;

    // Free space by swapping out a block of lines to the file.
    void swapOutBlock() const;

    // Lazy creation of tmp file for swapping image data to disk
    void initTmpfile() const;

    void initMembers() {
        initPixel = value_type();
        linesPerBlocksize_ = 0;
        linesPerBlocksizeLog2_ = 0;
        blocksAllocated_ = 0;
        blocksNeeded_ = 0;
        blocksInMemory_ = NULL;
        lines_ = NULL;
        blockIsClean_ = NULL;
        blockInFile_ = NULL;
        width_ = 0;
        height_ = 0;
#ifdef _WIN32
        hTempFile_ = INVALID_HANDLE_VALUE;
#else
        tmpFile_ = NULL;
#endif
        tmpFilename_ = NULL;
    }

    // how many image lines are loaded in one fread
    int linesPerBlocksize_;
    int linesPerBlocksizeLog2_;
    // How many blocks are currently cached in memory.
    mutable int blocksAllocated_;
    // How many blocks are needed for the entire image.
    int blocksNeeded_;

    // List of blocks in memory. 
    // Blocks are in sequential order.
    mutable list<int> *blocksInMemory_;

    // An iterator that points to the most recently loaded block
    // in the blocksInMemory_ list.
    mutable list<int>::iterator mostRecentlyLoadedBlockIterator_;

    // Array of pointers to first pixel in each line.
    mutable PIXELTYPE ** lines_;

    // For each block, indicate if that block is dirty.
    // Dirty blocks get swapped to disk when they are deallocated.
    // Clean blocks are simply deallocated.
    mutable bool * blockIsClean_;

    // For each block, indicate if that block is in the file.
    // Blocks in the file get read from disk when the block is swapped in.
    // Blocks not in the file get initialized with initPixel instead.
    mutable bool * blockInFile_;

    int width_, height_;

#ifdef _WIN32
    mutable HANDLE hTempFile_;
#else
    mutable FILE *tmpFile_;
#endif

    mutable char *tmpFilename_;
};

template <class PIXELTYPE>
void CachedFileImage<PIXELTYPE>::deallocate() {
    // Unregister the image with the director.
    //cout << "image " << this << ".deallocate" << endl;
    CachedFileImageDirector::v().returnBlocksUnregisterImage(blocksAllocated_, this);
    delete blocksInMemory_;

    // Deallocate all the blocks in use.
    if (lines_ != NULL) {
        int line = 0;
        for (int block = 0; block < blocksNeeded_; block++) {
            int firstLineInBlock = line;
            for (int subblock = 0; subblock < linesPerBlocksize_; subblock++, line++) {
                if (line >= height_) break;
                PIXELTYPE *p = lines_[line];
                if (p != NULL) {
                    for (int column = 0; column < width_; column++) {
                        //FIXME if pixel type is not a simple data type
                        // and this destructor actually does
                        // something, then we are in big trouble.
                        (p[column]).~PIXELTYPE();
                    }
                }
            }
            CachedFileImageDirector::v().deallocateBlock(lines_[firstLineInBlock]);
            if (line >= height_) break;
        }
        delete[] lines_;
    }

    delete[] blockIsClean_;
    delete[] blockInFile_;

    // Close and delete the tmp file if it exists.
#ifdef _WIN32
    if (hTempFile_ != INVALID_HANDLE_VALUE) {
        CloseHandle(hTempFile_);
    }
#else
    if (tmpFile_ != NULL) {
        fclose(tmpFile_);
    }
#endif
    //    unlink(tmpFilename_);
    //}
    delete[] tmpFilename_;
};

/** Perform initialization of internal data structures once image size is known. */
template <class PIXELTYPE>
void CachedFileImage<PIXELTYPE>::initLineStartArray() {

    // Number of lines to load in one block.
    linesPerBlocksize_ = (int)floor(
            ((double)CachedFileImageDirector::v().getBlockSize())
            / (width_ * sizeof(PIXELTYPE)));
    // Round this down to the nearest power of two.
    // This will let us divide using right-shift to calculate block number from line number.
    linesPerBlocksizeLog2_ = (int)floor(log((double)linesPerBlocksize_)/log(2.0));
    linesPerBlocksize_ = 1 << linesPerBlocksizeLog2_;
    //cout << "linesPerBlocksizeLog2=" << linesPerBlocksizeLog2_ << endl;
    //cout << "linesPerBlocksize=" << linesPerBlocksize_ << endl;

    // This is no longer necessary with SKIPSM pyramid operations.
    //// Need a minimum of 3 lines per block to support 5-line sliding windows.
    //if (linesPerBlocksize_ < 3) {
    //    vigra_fail("Image cache block size is too small. Use the -b flag to "
    //            "increase the cache block size.");
    //}

    blocksNeeded_ = (int)ceil(((double)height_) / linesPerBlocksize_);

    //cout << "image.initLineStartArray this=" << this
    //     << " linesPerBlocksize=" << linesPerBlocksize_
    //     << " blocksNeeded=" << blocksNeeded_ << endl;

    // Request blocks from the director.
    int blocksAllowed = CachedFileImageDirector::v().requestBlocksForNewImage(
            blocksNeeded_, this);

    //cout << "image.initLineStartArray this=" << this
    //     << " blocksAllowed=" << blocksAllowed << endl;

    // Create the blockLRU list.
    blocksInMemory_ = new list<int>();

    lines_ = new PIXELTYPE*[height_];
    blockIsClean_ = new bool[blocksNeeded_];
    blockInFile_ = new bool[blocksNeeded_];

    // Initialize blockIsClean / blockInFile vectors.
    for (int block = 0; block < blocksNeeded_; block++) {
        blockIsClean_[block] = true;
        blockInFile_[block] = false;
    }

    // Allocate mem for the first linesPerBlocksize_*blocksAllowed lines.
    int line = 0;
    for (int block = 0; block < blocksAllowed; block++) {
        blocksInMemory_->push_back(block);
        blocksAllocated_++;

        // Get a block from the director.
        PIXELTYPE* blockStart = (PIXELTYPE*)CachedFileImageDirector::v().allocateBlock();

        // Divide the block up amongst the lines in the block.
        for (int subblock = 0; subblock < linesPerBlocksize_; subblock++, line++, blockStart+=width_) {
            if (line >= height_) break;
            lines_[line] = blockStart;
            // Fill the line with initPixel.
            std::uninitialized_fill_n(lines_[line], width_, initPixel);
        }
        if (line >= height_) break;
    }

    // All remaining lines (if any) are null (swapped out)
    for (; line < height_; line++) {
        lines_[line] = NULL;
    }

    //cout << "image.initLineStartArray this=" << this << " blocksInMemory=";
    //std::copy(blocksInMemory_->begin(), blocksInMemory_->end(),
    //        std::ostream_iterator<int>(cout, " "));
    //cout << endl;

    mostRecentlyLoadedBlockIterator_ = blocksInMemory_->begin();

    return;
};

/** Obtain a pointer to the beginning of a row.
 *  Marks the block that owns that row dirty.
 */
template <class PIXELTYPE>
inline PIXELTYPE * CachedFileImage<PIXELTYPE>::getLinePointerDirty(const int dy) {
    PIXELTYPE *line = lines_[dy];

    // Check if line dy is swapped out.
    if (line == NULL) line = getLinePointerCacheMiss(dy);

    // Mark this block as dirty.
    //blockIsClean_[dy / linesPerBlocksize_] = false;
    blockIsClean_[dy >> linesPerBlocksizeLog2_] = false;

    return line;
};

/** Obtain a pointer to the beginning of a row.
 *  This is for clean dereferences.
 */
template <class PIXELTYPE>
inline PIXELTYPE * CachedFileImage<PIXELTYPE>::getLinePointer(const int dy) const {
    PIXELTYPE *line = lines_[dy];

    // Check if line dy is swapped out.
    if (line == NULL) line = getLinePointerCacheMiss(dy);

    return line;
};

/** Swap in the requested line. */
template <class PIXELTYPE>
PIXELTYPE * CachedFileImage<PIXELTYPE>::getLinePointerCacheMiss(const int dy) const {
    //int blockNumber = dy / linesPerBlocksize_;
    int blockNumber = dy >> linesPerBlocksizeLog2_;
    //int firstLineInBlock = blockNumber * linesPerBlocksize_;
    int firstLineInBlock = blockNumber << linesPerBlocksizeLog2_;

    //cout << "-------------------------------------------------------" << endl;
    //cout << "image " << this << " cache miss:"
    //     << " line=" << dy
    //     << " block=" << blockNumber
    //     << " firstLineInBlock=" << firstLineInBlock
    //     << " blocksAllocated=" << blocksAllocated_
    //     << " blocksInMemory={";
    //std::copy(blocksInMemory_->begin(), blocksInMemory_->end(),
    //        std::ostream_iterator<int>(cout, " "));
    //cout << "}" << endl;

    int moreBlocks = CachedFileImageDirector::v().registerCacheMiss(this);
    if (moreBlocks == 0 && blocksAllocated_ == 0) {
        vigra_fail("enblend: Out of memory blocks. Try using the -m flag to "
                "increase the amount of memory to use, or use the -b flag to "
                "decrease the block size\n"
                "reason: cache miss, 0 blocks allocated, 0 blocks available.");
    } else if (moreBlocks == 0) {
        // Make space for new block.
        swapOutBlock();
    }

    // Allocate a block.
    PIXELTYPE* blockStart = (PIXELTYPE*)CachedFileImageDirector::v().allocateBlock();

    // The number of lines in the block and the number of pixels in the block.
    int numLinesInBlock = min(height_, firstLineInBlock + linesPerBlocksize_)
            - firstLineInBlock;
    int pixelsToRead = numLinesInBlock * width_;

    if (blockInFile_[blockNumber]) {
        // Find the right spot in the file.
#ifdef _WIN32
        DWORD dwError = NO_ERROR;
        LONGLONG offset = (LONGLONG)width_
                          * (LONGLONG)firstLineInBlock
                          * (LONGLONG)sizeof(PIXELTYPE);
        LARGE_INTEGER liOffset;
        liOffset.QuadPart = offset;
        liOffset.LowPart = SetFilePointer(hTempFile_, liOffset.LowPart, &liOffset.HighPart, FILE_BEGIN);
        if (liOffset.LowPart == INVALID_SET_FILE_POINTER && (dwError = GetLastError()) != NO_ERROR) {
            LPVOID lpMsgBuf;
            FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);
            cerr << endl << lpMsgBuf << endl;
            LocalFree(lpMsgBuf);
            vigra_fail("Unable to seek within temporary file.\n"
                    "This error is specific to the Windows version of Enblend.");
        }
#else
        off_t offset = (off_t)width_
                * (off_t)firstLineInBlock
                * (off_t)sizeof(PIXELTYPE); 
        if (fseeko(tmpFile_, offset, SEEK_SET) != 0) {
            vigra_fail(strerror(errno));
        }
#endif
        // Fill the block with data from the file.
        // FIXME: should use compression
#ifdef _WIN32
        DWORD bytesRead;
        if (0 == ReadFile(hTempFile_, 
                          blockStart, 
                          sizeof(PIXELTYPE) * pixelsToRead, 
                          &bytesRead, 
                          NULL)) {
            DWORD dwError = GetLastError();
            LPVOID lpMsgBuf;
            FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);
            cerr << endl << lpMsgBuf << endl;
            LocalFree(lpMsgBuf);
            vigra_fail("enblend: error reading from image swap file.\n");
        }
#else
        clearerr(tmpFile_);
        int itemsRead = fread(blockStart, sizeof(PIXELTYPE), pixelsToRead, tmpFile_);
        if (itemsRead < pixelsToRead) {
            perror("enblend");
            vigra_fail("enblend: error reading from image swap file.\n");
        }
#endif
    }
    else {
        // File does not have data for this block.
        // Fill lines with initPixel.
        std::uninitialized_fill_n(blockStart, pixelsToRead, initPixel);
    }

    // Divide the block up amongst the lines in the block.
    for (int l = 0; l < linesPerBlocksize_; l++, blockStart+=width_) {
        int absoluteLineNumber = l + firstLineInBlock;
        if (absoluteLineNumber >= height_) break;
        lines_[absoluteLineNumber] = blockStart;
    }

    // Mark this block as clean.
    blockIsClean_[blockNumber] = true;

    // Add this block to the list in the right spot.
    list<int>::iterator i = blocksInMemory_->begin();
    for (; i != blocksInMemory_->end(); ++i) {
        if (*i > blockNumber) break;
    }
    blocksInMemory_->insert(i, blockNumber);

    // Push mostRecentlyLoadedBlockIterator to the end of a sequential sequence
    // of blocks in memory.
    mostRecentlyLoadedBlockIterator_ = --i;
    for (++i; i != blocksInMemory_->end(); ++i) {
        if (*i == ++blockNumber) mostRecentlyLoadedBlockIterator_ = i;
        else break;
    }

    blocksAllocated_++;

    //cout << "image.after-swap-in this=" << this
    //     << " blocksAllocated=" << blocksAllocated_
    //     << " mostRecentlyLoadedBlock=" << *mostRecentlyLoadedBlockIterator_
    //     << " blocksInMemory=";
    //std::copy(blocksInMemory_->begin(), blocksInMemory_->end(),
    //        std::ostream_iterator<int>(cout, " "));
    //cout << endl;
    //CachedFileImageDirector::v().printAllBlockStats();

    return lines_[dy];
};

/** Swap a block out to disk. */
template <class PIXELTYPE>
void CachedFileImage<PIXELTYPE>::swapOutBlock() const {
    if (blocksAllocated_ == 0) {
        vigra_fail("Attempt to free a block from an image that has no blocks.");
    }

    //cout << "swapOutBlock image=" << this
    //     << " blocksAllocated=" << blocksAllocated_
    //     << " block list before:";
    //std::copy(blocksInMemory_->begin(), blocksInMemory_->end(),
    //        std::ostream_iterator<int>(cout, " "));
    //cout << endl;

    // Choose a block to swap out.
    int blockNumber = 0;
    // This is no longer necessary with SKIPSM-based reduce.
    //if (blocksAllocated_ == 1 && blocksNeeded_ > 1) {
    //    // Never swap out our only block (if we need more than one).
    //    // Enblend iterates over images in 5-line sliding windows (in reduce).
    //    // This is to avoid thrashing.
    //    vigra_fail("enblend: Out of memory blocks. Try using the -m flag to "
    //            "increase the amount of memory to use, or use the -b flag to "
    //            "decrease the block size\n"
    //            "reason: probable thrash, 1 blocks allocated, 0 blocks available.");
    //} else {
        if (mostRecentlyLoadedBlockIterator_ == blocksInMemory_->begin()) {
            // We're at the beginning of the image.
            // The last block in memory is the one least likely to be used next,
            // assuming forward iteration through the image.
            blockNumber = blocksInMemory_->back();
            blocksInMemory_->pop_back();
        } else {
            // Try the block before mostRecentlyLoadedBlockIterator.
            // Keep the m-r-l block for the 5-line sliding window.
            // The block just before that is least likely to be used next,
            // assuming forward iteration through the image.
            list<int>::iterator candidate = mostRecentlyLoadedBlockIterator_;
            --candidate;
            blockNumber = *candidate;
            blocksInMemory_->erase(candidate);
        }
    //}

    //cout << "swapOutBlock image=" << this << " mRLBI=" << *mostRecentlyLoadedBlockIterator_
    //     << " after list remove block=" << blockNumber << ": ";
    //std::copy(blocksInMemory_->begin(), blocksInMemory_->end(),
    //        std::ostream_iterator<int>(cout, " "));
    //cout << endl;

    //int firstLineInBlock = blockNumber * linesPerBlocksize_;
    int firstLineInBlock = blockNumber << linesPerBlocksizeLog2_;
    PIXELTYPE *blockStart = lines_[firstLineInBlock];

    // If block is dirty, swap it to the file.
    if (!blockIsClean_[blockNumber]) {
        blockInFile_[blockNumber] = true;

        // Lazy init the temp file.
#ifdef _WIN32
        if (hTempFile_ == INVALID_HANDLE_VALUE)
#else
        if (tmpFile_ == NULL) 
#endif
            initTmpfile();

        // Find the right spot in the file.
#ifdef _WIN32
        DWORD dwError = NO_ERROR;
        LONGLONG offset = (LONGLONG)width_
                          * (LONGLONG)firstLineInBlock
                          * (LONGLONG)sizeof(PIXELTYPE);
        LARGE_INTEGER liOffset;
        liOffset.QuadPart = offset;
        liOffset.LowPart = SetFilePointer(hTempFile_, liOffset.LowPart, &liOffset.HighPart, FILE_BEGIN);
        if (liOffset.LowPart == INVALID_SET_FILE_POINTER && (dwError = GetLastError()) != NO_ERROR) {
            LPVOID lpMsgBuf;
            FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);
            cerr << endl << lpMsgBuf << endl;
            LocalFree(lpMsgBuf);
            vigra_fail("Unable to seek within temporary file.\n"
                    "This error is specific to the Windows version of Enblend.");
        }
#else
        off_t offset = (off_t)width_
                * (off_t)firstLineInBlock
                * (off_t)sizeof(PIXELTYPE);
        if (fseeko(tmpFile_, offset, SEEK_SET) != 0) {
            vigra_fail(strerror(errno));
        }
#endif

        int numLinesInBlock = min(height_, firstLineInBlock + linesPerBlocksize_)
                - firstLineInBlock;
        int pixelsToWrite = numLinesInBlock * width_;
        //FIXME this should use compression.
#ifdef _WIN32
        DWORD bytesWritten;
        if (0 == WriteFile(hTempFile_, 
                           blockStart, 
                           sizeof(PIXELTYPE) * pixelsToWrite, 
                           &bytesWritten, 
                           NULL)) {
            DWORD dwError = GetLastError();
            LPVOID lpMsgBuf;
            FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    dwError,
                    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0,
                    NULL);
            cerr << endl << lpMsgBuf << endl;
            LocalFree(lpMsgBuf);
            vigra_fail("enblend: error writing to image swap file.\nMost likely cause: No space for temporary files.\nMake sure that there is enough space in the temporary directory\n");
        }
#else
        clearerr(tmpFile_);
        int itemsWritten = fwrite(blockStart, sizeof(PIXELTYPE), pixelsToWrite, tmpFile_);
        if (itemsWritten < pixelsToWrite) {
            perror("enblend");
            vigra_fail("enblend: error writing to image swap file.\nMost likely cause: No space for temporary files.\nMake sure that there is enough space in the temporary directory\n");
        }
#endif
    }

    // Deallocate lines in block.
    for (int l = 0; l < linesPerBlocksize_; l++) {
        int absoluteLineNumber = l + firstLineInBlock;
        if (absoluteLineNumber >= height_) break;
        PIXELTYPE *p = lines_[absoluteLineNumber];
        for (int column = 0; column <= width_; column++) {
            //FIXME if pixel type is not a simple data type and
            // this destructor actually does
            // something, then we are in big trouble.
            (p[column]).~PIXELTYPE();
        }
        lines_[absoluteLineNumber] = NULL;
    }

    // Return block to the director.
    CachedFileImageDirector::v().deallocateBlock(blockStart);

    blocksAllocated_--;

    //cout << "swapOutBlock image=" << this << " after swapout:"
    //     << " blocksAllocated=" << blocksAllocated_
    //     << " blocksInMemory=";
    //std::copy(blocksInMemory_->begin(), blocksInMemory_->end(),
    //        std::ostream_iterator<int>(cout, " "));
    //cout << endl;
};

/** Create the tmp file to store swapped-out blocks. */
template <class PIXELTYPE>
void CachedFileImage<PIXELTYPE>::initTmpfile() const {


#ifdef _WIN32
    const DWORD BUFSIZE=MAX_PATH;
    char filenameTemplate[BUFSIZE];
    char lpPathBuffer[BUFSIZE];
    DWORD dwRetVal;

     // Get the temp path.
    dwRetVal = GetTempPath(BUFSIZE,     // length of the buffer
                           lpPathBuffer); // buffer for path 
    if (dwRetVal > BUFSIZE || (dwRetVal == 0))
    {
        vigra_fail ("GetTempPath failed.");
    }

    if (! GetTempFileNameA(lpPathBuffer, // directory for temp files
                          ".en", // temp file prefix
                          0, // create unique name
                          filenameTemplate))  // buffer for name
    {
        vigra_fail("enblend: unable to create image swap file name.\n");
    }
    
    hTempFile_ = CreateFileA(filenameTemplate,
                             GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ, // share mode
                             NULL, // security attributes
                             CREATE_ALWAYS, // create mode
                             FILE_FLAG_DELETE_ON_CLOSE|FILE_ATTRIBUTE_HIDDEN|FILE_ATTRIBUTE_TEMPORARY, // file attributes
                             NULL);
    if (hTempFile_ == INVALID_HANDLE_VALUE) {
        vigra_fail(strerror(errno));
    }

#else
    sigset_t oldsigmask;
    sigset_t SigintMask;
    sigprocmask(SIG_BLOCK, &SigintMask, &oldsigmask);

    char * filenameTemplate;
    char * tmpdir = getenv("TMPDIR");
    if (tmpdir) {
        int len = strlen(tmpdir) + 22;
        if (len > 0) {
            filenameTemplate = (char *) malloc(strlen(tmpdir)+ 22);
            strncpy(filenameTemplate, tmpdir, len);
            if (filenameTemplate[len-1] != '/') {
                strncat(filenameTemplate, "/", len);
            }
            strncat(filenameTemplate, ".enblend_tmpXXXXXX", len);
        } else {
            filenameTemplate = strdup("/tmp/.enblend_tmpXXXXXX");
        }
    } else {
       filenameTemplate = strdup("/tmp/.enblend_tmpXXXXXX");
    }
    

#if defined(HAVE_MKSTEMP)
    int tmpFD = mkstemp(filenameTemplate);
    if (tmpFD < 0) {
        vigra_fail("enblend: unable to create image swap file.\n");
    }
    tmpFile_ = fdopen(tmpFD, "wb+");
#else // HAVE_MKSTEMP
    char *tmpReturn = mktemp(filenameTemplate);
    if (tmpReturn == NULL) {
        vigra_fail("enblend: unable to create image swap file.\n");
    }
    tmpFile_ = fopen(filenameTemplate, "wb+");
#endif // !HAVE_MKSTEMP

    if (tmpFile_ == NULL) {
        vigra_fail(strerror(errno));
    }

#endif // ! _WIN32

    unsigned int filenameTemplateLength = (unsigned int)strlen(filenameTemplate) + 1;
    tmpFilename_ = new char[filenameTemplateLength];
    strncpy(tmpFilename_, filenameTemplate, filenameTemplateLength);

#ifndef _WIN32
    unlink(tmpFilename_);
    sigprocmask(SIG_SETMASK, &oldsigmask, NULL);
    free(filenameTemplate);
#endif

    // This doesn't seem to help.
    //if (setvbuf(tmpFile_, NULL, _IONBF, 0) != 0) {
    //    vigra_fail(strerror(errno));
    //}
};

template <class PIXELTYPE>
CachedFileImage<PIXELTYPE> &
CachedFileImage<PIXELTYPE>::operator=(const CachedFileImage<PIXELTYPE> & rhs) {
    if (this != &rhs) {
        if ((width() != rhs.width()) ||
                (height() != rhs.height())) {
            resizeCopy(rhs);
        }
        else {
            const_iterator is = rhs.begin();
            const_iterator iend = rhs.end();
            iterator id = begin();
            for(; is != iend; ++is, ++id) *id = *is;
        }
    }
    return *this;
};

template <class PIXELTYPE>
CachedFileImage<PIXELTYPE> &
CachedFileImage<PIXELTYPE>::init(value_type const & pixel) {
    initPixel = pixel;

    // Mark all blocks as clean
    // Mark all blocks as absent from the backing file.
    // This will effectively discard all current pixel data in the file.
    for (int block = 0; block < blocksNeeded_; block++) {
        blockIsClean_[block] = true;
        blockInFile_[block] = false;
    }

    // For any lines that are currently allocated, fill them with initPixel.
    // This will discard all current pixel data in memory.
    for (int line = 0; line < height_; line++) {
        PIXELTYPE *p = lines_[line];
        if (lines_[line] != NULL) {
            for (int column = 0; column <= width_; column++) {
                //FIXME if pixel type is not a simple data type and this
                // destructor actually does
                // something, then we are in big trouble.
                (p[column]).~PIXELTYPE();
            }
            // Fill the line with initPixel.
            std::uninitialized_fill_n(lines_[line], width_, initPixel);
        }
    }

    return *this;
};

template <class PIXELTYPE>
void CachedFileImage<PIXELTYPE>::resize(int width, int height, value_type const & d) {
    vigra_precondition((width >= 0) && (height >= 0),
            "CachedFileImage::resize(int width, int height, value_type const &): "
            "width and height must be >= 0.\n");
    deallocate();
    initMembers();
    initPixel = d;
    width_ = width;
    height_ = height;
    initLineStartArray();
};

template <class PIXELTYPE>
void CachedFileImage<PIXELTYPE>::resizeCopy(const CachedFileImage & rhs) {
    deallocate();
    initMembers();
    if (rhs.width() * rhs.height() > 0) {
        width_ = rhs.width();
        height_ = rhs.height();
        initLineStartArray();

        const_iterator is = rhs.begin();
        const_iterator iend = rhs.end();
        iterator id = begin();
        for(; is != iend; ++is, ++id) *id = *is;
    }
};

template <class PIXELTYPE>
void CachedFileImage<PIXELTYPE>::swap( CachedFileImage<PIXELTYPE>& rhs ) {
    if (&rhs != this) {
        std::swap(initPixel, rhs.initPixel);
        std::swap(linesPerBlocksize_, rhs.linesPerBlocksize_);
        std::swap(linesPerBlocksizeLog2_, rhs.linesPerBlocksizeLog2_);
        std::swap(blocksAllocated_, rhs.blocksAllocated_);
        std::swap(blocksNeeded_, rhs.blocksNeeded_);
        std::swap(blocksInMemory_, rhs.blocksInMemory_);
        std::swap(mostRecentlyLoadedBlockIterator_, rhs.mostRecentlyLoadedBlockIterator_);
        std::swap(lines_, rhs.lines_);
        std::swap(blockIsClean_, rhs.blockIsClean_);
        std::swap(blockInFile_, rhs.blockInFile_);
        std::swap(width_, rhs.width_);
        std::swap(height_, rhs.height_);
#ifdef _WIN32
        std::swap(hTempFile_, rhs.hTempFile_);
#else
        std::swap(tmpFile_, rhs.tmpFile_);
#endif
        std::swap(tmpFilename_, rhs.tmpFilename_);
    }
};

/********************************************************/
/*                                                      */
/*              argument object factories               */
/*                                                      */
/********************************************************/

template <class PixelType, class Accessor>
inline triple<typename CachedFileImage<PixelType>::const_traverser, 
              typename CachedFileImage<PixelType>::const_traverser, Accessor>
srcImageRange(CachedFileImage<PixelType> const & img, Accessor a)
{
    return triple<typename CachedFileImage<PixelType>::const_traverser, 
                  typename CachedFileImage<PixelType>::const_traverser, 
          Accessor>(img.upperLeft(),
                    img.lowerRight(),
                    a);
}

template <class PixelType, class Accessor>
inline pair<typename CachedFileImage<PixelType>::const_traverser, Accessor>
srcImage(CachedFileImage<PixelType> const & img, Accessor a)
{
    return pair<typename CachedFileImage<PixelType>::const_traverser, 
                Accessor>(img.upperLeft(), a);
}

template <class PixelType, class Accessor>
inline triple<typename CachedFileImage<PixelType>::traverser, 
              typename CachedFileImage<PixelType>::traverser, Accessor>
destImageRange(CachedFileImage<PixelType> & img, Accessor a)
{
    return triple<typename CachedFileImage<PixelType>::traverser, 
                  typename CachedFileImage<PixelType>::traverser, 
          Accessor>(img.upperLeft(),
                    img.lowerRight(),
                    a);
}

template <class PixelType, class Accessor>
inline pair<typename CachedFileImage<PixelType>::traverser, Accessor>
destImage(CachedFileImage<PixelType> & img, Accessor a)
{
    return pair<typename CachedFileImage<PixelType>::traverser, 
                Accessor>(img.upperLeft(), a);
}

template <class PixelType, class Accessor>
inline pair<typename CachedFileImage<PixelType>::const_traverser, Accessor>
maskImage(CachedFileImage<PixelType> const & img, Accessor a)
{
    return pair<typename CachedFileImage<PixelType>::const_traverser, 
                Accessor>(img.upperLeft(), a);
}

/****************************************************************/

template <class PixelType>
inline triple<typename CachedFileImage<PixelType>::const_traverser, 
              typename CachedFileImage<PixelType>::const_traverser, 
              typename CachedFileImage<PixelType>::ConstAccessor>
srcImageRange(CachedFileImage<PixelType> const & img)
{
    return triple<typename CachedFileImage<PixelType>::const_traverser, 
                  typename CachedFileImage<PixelType>::const_traverser, 
                  typename CachedFileImage<PixelType>::ConstAccessor>(img.upperLeft(),
                                                                 img.lowerRight(),
                                                                 img.accessor());
}

template <class PixelType>
inline pair< typename CachedFileImage<PixelType>::const_traverser, 
             typename CachedFileImage<PixelType>::ConstAccessor>
srcImage(CachedFileImage<PixelType> const & img)
{
    return pair<typename CachedFileImage<PixelType>::const_traverser, 
                typename CachedFileImage<PixelType>::ConstAccessor>(img.upperLeft(), 
                                                               img.accessor());
}

template <class PixelType>
inline triple< typename CachedFileImage<PixelType>::traverser, 
               typename CachedFileImage<PixelType>::traverser, 
               typename CachedFileImage<PixelType>::Accessor>
destImageRange(CachedFileImage<PixelType> & img)
{
    return triple<typename CachedFileImage<PixelType>::traverser, 
                  typename CachedFileImage<PixelType>::traverser, 
                  typename CachedFileImage<PixelType>::Accessor>(img.upperLeft(),
                                                            img.lowerRight(),
                                                            img.accessor());
}

template <class PixelType>
inline pair< typename CachedFileImage<PixelType>::traverser, 
             typename CachedFileImage<PixelType>::Accessor>
destImage(CachedFileImage<PixelType> & img)
{
    return pair<typename CachedFileImage<PixelType>::traverser, 
                typename CachedFileImage<PixelType>::Accessor>(img.upperLeft(), 
                                                          img.accessor());
}

template <class PixelType>
inline pair< typename CachedFileImage<PixelType>::const_traverser, 
             typename CachedFileImage<PixelType>::ConstAccessor>
maskImage(CachedFileImage<PixelType> const & img)
{
    return pair<typename CachedFileImage<PixelType>::const_traverser, 
                typename CachedFileImage<PixelType>::ConstAccessor>(img.upperLeft(), 
                                                               img.accessor());
}

template <class PixelType>
inline pair< ConstStridedCachedFileImageIterator<PixelType>,
             typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor>
maskStrideIter(CachedFileImageIterator<PixelType> const & upperLeft, int xstride, int ystride)
{
    return pair< ConstStridedCachedFileImageIterator<PixelType>,
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor >
                (ConstStridedCachedFileImageIterator<PixelType>(upperLeft, xstride, ystride),
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor());
}

template <class PixelType>
inline pair< ConstStridedCachedFileImageIterator<PixelType>,
             typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor>
maskStrideIter(ConstCachedFileImageIterator<PixelType> const & upperLeft, int xstride, int ystride)
{
    return pair< ConstStridedCachedFileImageIterator<PixelType>,
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor >
                (ConstStridedCachedFileImageIterator<PixelType>(upperLeft, xstride, ystride),
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor());
}

template <class PixelType>
inline triple< ConstStridedCachedFileImageIterator<PixelType>,
             ConstStridedCachedFileImageIterator<PixelType>,
             typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor>
maskStrideIterRange(CachedFileImageIterator<PixelType> const & upperLeft,
                    CachedFileImageIterator<PixelType> const & lowerRight,
                    int xstride, int ystride)
{
    return triple< ConstStridedCachedFileImageIterator<PixelType>,
                ConstStridedCachedFileImageIterator<PixelType>,
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor >
                (ConstStridedCachedFileImageIterator<PixelType>(upperLeft, xstride, ystride),
                ConstStridedCachedFileImageIterator<PixelType>(lowerRight, xstride, ystride),
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor());
}

template <class PixelType>
inline triple< ConstStridedCachedFileImageIterator<PixelType>,
             ConstStridedCachedFileImageIterator<PixelType>,
             typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor>
maskStrideIterRange(ConstCachedFileImageIterator<PixelType> const & upperLeft,
                    ConstCachedFileImageIterator<PixelType> const & lowerRight,
                    int xstride, int ystride)
{
    return triple< ConstStridedCachedFileImageIterator<PixelType>,
                ConstStridedCachedFileImageIterator<PixelType>,
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor >
                (ConstStridedCachedFileImageIterator<PixelType>(upperLeft, xstride, ystride),
                ConstStridedCachedFileImageIterator<PixelType>(lowerRight, xstride, ystride),
                typename IteratorTraits<ConstStridedCachedFileImageIterator<PixelType> >::DefaultAccessor());
}

template <typename PixelType, typename ImgAccessor>
vigra::triple<StridedCachedFileImageIterator<PixelType>, StridedCachedFileImageIterator<PixelType>, ImgAccessor>
stride(int xstride, int ystride, vigra::triple<CachedFileImageIterator<PixelType>, CachedFileImageIterator<PixelType>, ImgAccessor> image) {
    Diff2D diff = image.second - image.first;
    if (diff.x % xstride) diff.x += (xstride - (diff.x % xstride));
    if (diff.y % ystride) diff.y += (ystride - (diff.y % ystride));
    //cout << "stride(" << xstride << ", " << ystride << ", " << (image.second - image.first) << ", " << diff << ")" << endl;
    return vigra::make_triple(StridedCachedFileImageIterator<PixelType>(image.first, xstride, ystride),
                       StridedCachedFileImageIterator<PixelType>(image.first + diff, xstride, ystride),
                       image.third);
};

template <typename PixelType, typename ImgAccessor>
vigra::triple<ConstStridedCachedFileImageIterator<PixelType>, ConstStridedCachedFileImageIterator<PixelType>, ImgAccessor>
stride(int xstride, int ystride, vigra::triple<ConstCachedFileImageIterator<PixelType>, ConstCachedFileImageIterator<PixelType>, ImgAccessor> image) {
    Diff2D diff = image.second - image.first;
    if (diff.x % xstride) diff.x += (xstride - (diff.x % xstride));
    if (diff.y % ystride) diff.y += (ystride - (diff.y % ystride));
    //cout << "stride(" << xstride << ", " << ystride << ", " << (image.second - image.first) << ", " << diff << ")" << endl;
    return vigra::make_triple(ConstStridedCachedFileImageIterator<PixelType>(image.first, xstride, ystride),
                       ConstStridedCachedFileImageIterator<PixelType>(image.first + diff, xstride, ystride),
                       image.third);
};

template <typename PixelType, typename ImgAccessor>
std::pair<StridedCachedFileImageIterator<PixelType>, ImgAccessor>
stride(int xstride, int ystride, std::pair<CachedFileImageIterator<PixelType>, ImgAccessor> image) {
    return std::make_pair(StridedCachedFileImageIterator<PixelType>(image.first, xstride, ystride), image.second);
};

template <typename PixelType, typename ImgAccessor>
std::pair<ConstStridedCachedFileImageIterator<PixelType>, ImgAccessor>
stride(int xstride, int ystride, std::pair<ConstCachedFileImageIterator<PixelType>, ImgAccessor> image) {
    return std::make_pair(ConstStridedCachedFileImageIterator<PixelType>(image.first, xstride, ystride), image.second);
};

} // namespace vigra

#endif /* VIGRA_EXT_CACHEDFILEIMAGE_HXX */

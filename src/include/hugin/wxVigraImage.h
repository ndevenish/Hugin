// -*- c-basic-offset: 4 -*-
/** @file wxVigraImage.h
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

#ifndef _WXVIGRAIMAGE_H
#define _WXVIGRAIMAGE_H

#include "wx/image.h"

/** a wrapper to make a wxImage look like a vigra image
 *
 *  it does not support resize and some other functions of
 *  vigra::BasicImage
 *
 *  Most code is taken from vigra::BasicImage
 */
//class wxVigraImage : public vigra::BasicImage<vigra::RGBValue<unsigned char> >
class wxVigraImage
{
public:
    // typedefs required for a vigra image
    typedef vigra::RGBValue<unsigned char> value_type;
    typedef vigra::RGBValue<unsigned char> PixelType;
    typedef PixelType &             reference;
    typedef PixelType const &       const_reference;
    typedef PixelType *             pointer;
    typedef PixelType * const       const_pointer;
    typedef PixelType * iterator;
    typedef PixelType * ScanOrderIterator;
    typedef PixelType const * const_iterator;
    typedef PixelType const * ConstScanOrderIterator;
    typedef vigra::ConstBasicImageIterator<PixelType, PixelType **> const_traverser;
    typedef vigra::BasicImageIterator<PixelType, PixelType **> Iterator;
    typedef vigra::ConstBasicImageIterator<PixelType, PixelType **> ConstIterator;
    typedef vigra::Diff2D difference_type;
    typedef vigra::Diff2D size_type;
    typedef vigra::IteratorTraits<Iterator>::DefaultAccessor Accessor;
    typedef vigra::IteratorTraits<ConstIterator>::DefaultAccessor ConstAccessor;
    /** create a new image with the data from img.
     *
     *  It does not copy the image data. @p img must live at least
     *  as long as wxVigraImage.
     */
    wxVigraImage(wxImage & img)
        : wxImg(img)
        {
            data_ = (vigra::RGBValue<unsigned char>*) wxImg.GetData();
            width_ = wxImg.GetWidth();
            height_ = wxImg.GetHeight();
            lines_ = initLineStartArray(data_, width_, height_);
        };

    ~wxVigraImage()
        {
            delete[] lines_;
        }


    /** width of Image
     */
    int width() const
    {
        return width_;
    }

        /** height of Image
        */
    int height() const
    {
        return height_;
    }

        /** size of Image
        */
    vigra::Diff2D size() const
    {
        return vigra::Diff2D(width(), height());
    }

        /** test whether a given coordinate is inside the image
        */
    bool isInside(vigra::Diff2D const & d) const
    {
        return d.x >= 0 && d.y >= 0 &&
               d.x < width() && d.y < height();
    }

        /** access pixel at given location. <br>
	    usage: <TT> PixelType value = image[Diff2D(1,2)] </TT>
        */
    PixelType & operator[](vigra::Diff2D const & d)
    {
        return lines_[d.y][d.x];
    }

        /** read pixel at given location. <br>
	    usage: <TT> PixelType value = image[Diff2D(1,2)] </TT>
        */
    PixelType const & operator[](vigra::Diff2D const & d) const
    {
        return lines_[d.y][d.x];
    }

        /** access pixel at given location. <br>
	    usage: <TT> PixelType value = image(1,2) </TT>
        */
    PixelType & operator()(int dx, int dy)
    {
        return lines_[dy][dx];
    }

        /** read pixel at given location. <br>
	    usage: <TT> PixelType value = image(1,2) </TT>
        */
    PixelType const & operator()(int dx, int dy) const
    {
        return lines_[dy][dx];
    }

        /** access pixel at given location.
	        Note that the 'x' index is the trailing index. <br>
	    usage: <TT> PixelType value = image[2][1] </TT>
        */
    PixelType * operator[](int dy)
    {
        return lines_[dy];
    }

        /** read pixel at given location.
	        Note that the 'x' index is the trailing index. <br>
	    usage: <TT> PixelType value = image[2][1] </TT>
        */
    PixelType const * operator[](int dy) const
    {
        return lines_[dy];
    }

        /** init 2D random access iterator poining to upper left pixel
        */
    Iterator upperLeft()
    {
        return Iterator(lines_);
    }

        /** init 2D random access iterator poining to
         pixel(width, height), i.e. one pixel right and below lower right
         corner of the image as is common in C/C++.
        */
    Iterator lowerRight()
    {
        return upperLeft() + size();
    }

        /** init 2D random access const iterator poining to upper left pixel
        */
    ConstIterator upperLeft() const
    {
        return ConstIterator(lines_);
    }

        /** init 2D random access const iterator poining to
         pixel(width, height), i.e. one pixel right and below lower right
         corner of the image as is common in C/C++.
        */
    ConstIterator lowerRight() const
    {
        return upperLeft() + size();
    }

        /** init 1D random access iterator pointing to first pixel
        */
    ScanOrderIterator begin()
    {
        return data_;
    }

        /** init 1D random access iterator pointing past the end
        */
    ScanOrderIterator end()
    {
        return data_ + width() * height();
    }

        /** init 1D random access const iterator pointing to first pixel
        */
    ConstScanOrderIterator begin() const
    {
        return data_;
    }

        /** init 1D random access const iterator pointing past the end
        */
    ConstScanOrderIterator end() const
    {
        return data_ + width() * height();
    }

        /** return default accessor
        */
    Accessor accessor()
    {
        return Accessor();
    }

        /** return default const accessor
        */
    ConstAccessor accessor() const
    {
        return ConstAccessor();
    }

private:

    static PixelType ** initLineStartArray(PixelType * data, int width, int height)
    {
        PixelType ** lines = new PixelType*[height];
        for(int y=0; y<height; ++y)
            lines[y] = data + y*width;
        return lines;
    }

    wxImage &wxImg;
    PixelType * data_;
    PixelType ** lines_;
    int width_, height_;
};


inline vigra::triple<wxVigraImage::ConstIterator,
              wxVigraImage::ConstIterator,
              wxVigraImage::ConstAccessor>
srcImageRange(wxVigraImage const & img);

inline vigra::pair< wxVigraImage::ConstIterator,
             wxVigraImage::ConstAccessor>
srcImage(wxVigraImage const & img);

inline vigra::triple< wxVigraImage::Iterator,
               wxVigraImage::Iterator,
           wxVigraImage::Accessor>
destImageRange(wxVigraImage const& img);

inline vigra::pair< wxVigraImage::Iterator,
             wxVigraImage::Accessor>
destImage(wxVigraImage & img)
{
    return vigra::pair<wxVigraImage::Iterator,
        wxVigraImage::Accessor>(img.upperLeft(),
                                img.accessor());
}



inline vigra::pair< wxVigraImage::ConstIterator,
             wxVigraImage::ConstAccessor>
maskImage(wxVigraImage const & img);

template <class Accessor>
inline vigra::triple<typename wxVigraImage::ConstIterator,
              typename wxVigraImage::ConstIterator, Accessor>
srcImageRange(wxVigraImage const & img, Accessor a)
{
    return vigra::triple<typename wxVigraImage::ConstIterator,
        typename wxVigraImage::ConstIterator, Accessor>
        (img.upperLeft(),
         img.lowerRight(),
         a);
}

template <class Accessor>
inline vigra::pair<typename wxVigraImage::ConstIterator, Accessor>
srcImage(wxVigraImage const & img, Accessor a)
{
    return vigra::pair<typename wxVigraImage::ConstIterator,
                Accessor>(img.upperLeft(), a);
}

template <class Accessor>
inline vigra::triple<typename wxVigraImage::Iterator,
              typename wxVigraImage::Iterator, Accessor>
destImageRange(wxVigraImage & img, Accessor a)
{
    return vigra::triple<typename wxVigraImage::Iterator,
                  typename wxVigraImage::Iterator,
          Accessor>(img.upperLeft(),
                    img.lowerRight(),
                a);
}

template <class Accessor>
inline vigra::pair<typename wxVigraImage::Iterator, Accessor>
destImage(wxVigraImage & img, Accessor a)
{
    return vigra::pair<typename wxVigraImage::Iterator,
                Accessor>(img.upperLeft(), a);
}


#endif // _WXVIGRAIMAGE_H

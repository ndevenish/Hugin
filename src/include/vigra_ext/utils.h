// -*- c-basic-offset: 4 -*-
/** @file ROI.h
 *
 *  functions to manage ROI's
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

#ifndef VIGRA_EXT_UTILS_H
#define VIGRA_EXT_UTILS_H

#include <common/math.h>
#include <vigra/rgbvalue.hxx>
#include <cmath>

namespace vigra {

template <class T1, class T2>
struct PromoteTraits<RGBValue<T1>, T2 >
{
    typedef RGBValue<typename PromoteTraits<T1, T2>::Promote> Promote;
};

}

namespace vigra_ext {

using VIGRA_CSTD::pow;

inline float pow(float a, double b)
{
    return(a,(float) b);
}

/// component-wise absolute value
template <class T>
inline
vigra::RGBValue<T> pow(vigra::RGBValue<T> const & v, double e) {
    return vigra::RGBValue<T>(pow(v.red(),e), pow(v.green(),e),  pow(v.blue(),e));
}

/// add a scalar to all components
template <class V1, class V2>
inline
vigra::RGBValue<V1> &
operator+=(vigra::RGBValue<V1> & l, V2 const & r)
{
    l.red() += r;
    l.green() += r;
    l.blue() += r;
    return l;
}

/// add a scalar to all components
template <class V1, class V2>
inline
// WARNING: This is a hack.. 
//vigra::RGBValue<V1>
typename vigra::PromoteTraits<vigra::RGBValue<V1>, V2 >::Promote
operator+(vigra::RGBValue<V1> const & r1, V2 const & r2)
{
    typename vigra::PromoteTraits<vigra::RGBValue<V1>, V2 >::Promote res(r1);

    res += r2;

    return res;
}

/** Apply pow() function to each vector component.
 */
template <class V, int SIZE, class D1, class D2>
inline
vigra::TinyVector<V, SIZE>
pow(vigra::TinyVector<V, SIZE> const & v, double e)
{
    vigra::TinyVector<V, SIZE> res;
    for (int i=0; i<SIZE; i++)
        res[i] = std::pow(v[i], e);
    return res;
} 

/** count pixels that are > 0 in both images */
struct OverlapSizeCounter
{
    OverlapSizeCounter()
	: size(0)
    { }

    template<typename PIXEL>
    void operator()(PIXEL const & img1, PIXEL const & img2)
    {
	if (img1 > 0 && img2 > 0) {
	    size++;
	}
    }

    unsigned int getSize()
    {
	return size;
    }

    unsigned int size;
};


/** functor to combine two functors: result = f1( f2(v) )
 */
template <class F1, class F2>
struct NestFunctor
{
    F1 & f1;
    F2 & f2;
    NestFunctor(F1 & fu1, F2 & fu2)
    : f1(fu1), f2(fu2)
    { }

    /** the functor's second argument type
     */
    typedef typename F1::result_type result_type;

    template <class T1>
            result_type operator()(T1 const & v) const
    {
        return f1(f2(v));
    }
};


/** count pixels that are > 0 in a single image */
struct MaskPixelCounter
{
    MaskPixelCounter()
	: count(0)
    { }

    template<typename PIXEL>
    void operator()(PIXEL const & img1)
    {
	if (img1 > 0) {
	    count++;
	}
    }

    int getCount()
    {
	return count;
    }

    int count;
};

/** Apply a circular crop to \p img
 *
 *  Sets all pixels that are outside of
 *  the circle specified by \p middle and \p radius
 *  to zero.
 */
template <class SrcImageIterator, class SrcAccessor>
void circularCrop(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> img,
                  FDiff2D middle, double radius)
{
    vigra::Diff2D imgSize = img.second - img.first;
    double r2 = radius*radius;

    // create dest y iterator
    SrcImageIterator yd(img.first);
    // loop over the image and transform
    for(int y=0; y < imgSize.y; ++y, ++yd.y)
    {
        // create x iterators
        SrcImageIterator xd(yd);
        for(int x=0; x < imgSize.x; ++x, ++xd.x)
        {
            double dx = x-middle.x;
            double dy = y-middle.y;
            if (dx*dx+dy*dy > r2) {
                *xd = 0;
            }
        }
    }
}

} // namespace

#endif // VIGRA_EXT_UTILS_H


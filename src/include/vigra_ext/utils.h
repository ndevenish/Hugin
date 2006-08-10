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

template <class T1, unsigned int R, unsigned int G, unsigned int B, class T2>
struct PromoteTraits<RGBValue<T1, R, G, B>, T2 >
{
    typedef RGBValue<typename PromoteTraits<T1, T2>::Promote> Promote;
};

}

namespace vigra_ext {

using VIGRA_CSTD::pow;
using VIGRA_CSTD::log;

inline float pow(float a, double b)
{
    return std::pow(a,(float) b);
}

/// component-wise absolute value
template <class T, unsigned int R, unsigned int G, unsigned int B>
inline
vigra::RGBValue<T, R, G, B> pow(vigra::RGBValue<T, R, G, B> const & v, double e) {
    return vigra::RGBValue<T, R, G, B>(pow(v.red(),e), pow(v.green(),e),  pow(v.blue(),e));
}

/// add a scalar to all components
template <class V1, unsigned int R, unsigned int G, unsigned int B, class V2>
inline
vigra::RGBValue<V1, R, G, B> &
operator+=(vigra::RGBValue<V1, R, G, B> & l, V2 const & r)
{
    l.red() += r;
    l.green() += r;
    l.blue() += r;
    return l;
}

    /// component-wise logarithm
template <class T, unsigned int RIDX, unsigned int GIDX, unsigned int BIDX>
inline
vigra::RGBValue<T, RIDX, GIDX, BIDX>
log(vigra::RGBValue<T, RIDX, GIDX, BIDX> const & v)
{
    return vigra::RGBValue<T, RIDX, GIDX, BIDX>(std::log(v.red()), std::log(v.green()), std::log(v.blue()));
}

/// component-wise logarithm
template <class T, unsigned int RIDX, unsigned int GIDX, unsigned int BIDX>
inline
vigra::RGBValue<T, RIDX, GIDX, BIDX>
log10(vigra::RGBValue<T, RIDX, GIDX, BIDX> const & v)
{
    return vigra::RGBValue<T, RIDX, GIDX, BIDX>(std::log10(v.red()), std::log10(v.green()), std::log10(v.blue()));
}

/// add a scalar to all components
template <class V1, unsigned int R, unsigned int G, unsigned int B, class V2>
inline
// WARNING: This is a hack.. 
//vigra::RGBValue<V1>
typename vigra::PromoteTraits<vigra::RGBValue<V1, R, G, B>, V2 >::Promote
operator+(vigra::RGBValue<V1, R, G, B> const & r1, V2 const & r2)
{
    typename vigra::PromoteTraits<vigra::RGBValue<V1, R, G, B>, V2 >::Promote res(r1);

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

/** Apply log() function to each vector component.
 */
template <class V, int SIZE, class D1, class D2>
inline
vigra::TinyVector<V, SIZE>
log(vigra::TinyVector<V, SIZE> const & v, double e)
{
    vigra::TinyVector<V, SIZE> res;
    for (int i=0; i<SIZE; i++)
        res[i] = std::log(v[i], e);
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
 *
 * The functors are copied, so there is no way to get
 * their internal state after they have been applied.
 * 
 * This is quite useful for multithreaded processing.
 */
template <class F1, class F2>
struct NestFunctor
{
    F1 f1;
    F2 f2;
    NestFunctor(const F1 & fu1, const F2 & fu2)
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

    /** if F2 takes 2 arguments */
    template <class T1, class T2>
    result_type operator()(T1 const & v1, T2 const & v2) const
    {
        return f1(f2(v1,v2));
    }

    /** if F2 takes 3 arguments */
    template <class T1, class T2, class T3>
    result_type operator()(T1 const & v1, T2 const & v2, T3 const & v3) const
    {
        return f1(f2(v1,v2,v3));
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


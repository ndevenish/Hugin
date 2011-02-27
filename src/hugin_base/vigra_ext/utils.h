// -*- c-basic-offset: 4 -*-
/** @file vigra_ext/utils.h
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

#include <hugin_math/hugin_math.h>
#include <vigra/rgbvalue.hxx>
#include <vigra/transformimage.hxx>
#include <cmath>

namespace vigra {

template <class T1, unsigned int R, unsigned int G, unsigned int B, class T2>
struct PromoteTraits<RGBValue<T1, R, G, B>, T2 >
{
    typedef RGBValue<typename PromoteTraits<T1, T2>::Promote> Promote;
};

}

namespace vigra_ext {

/** Traits to define the maximum value for all types.
 *  The case of float and double differs from vigra::NumericTraits::max() */
#define LUT_TRAITS(T1,S) \
template<> \
struct LUTTraits<T1> \
{ \
    static T1 max() \
{ \
    return S; \
} \
}; \
template<> \
struct LUTTraits<vigra::RGBValue<T1> > \
{ \
    static T1 max() \
{ \
    return S; \
} \
};

    template <class T1>
    struct LUTTraits;

    LUT_TRAITS(unsigned char, UCHAR_MAX);
    LUT_TRAITS(signed char, SCHAR_MAX);
    LUT_TRAITS(unsigned short, USHRT_MAX);
    LUT_TRAITS(signed short, SHRT_MAX);
    LUT_TRAITS(unsigned int, UINT_MAX);
    LUT_TRAITS(signed int, INT_MAX);
    LUT_TRAITS(float, 1.0);
    LUT_TRAITS(double, 1.0);

#undef LUT_TRAITS

inline double getMaxValForPixelType(const std::string & v)
{
    if (v == "UINT8") {
        return 255;
    } else if (v == "INT8") {
        return 127;
    } else if (v == "UINT16") {
        return 65535;
    } else if (v == "INT16") {
        return 32767;
    } else if (v == "UINT32") {
        return 4294967295u;
    } else if (v == "INT32") {
        return 2147483647;
    }
    return 1.0;
}

template <class VALUE>
struct PointPairT
{
    PointPairT()
    {
    }

    PointPairT(short img1, VALUE val1, const hugin_utils::FDiff2D & p1, float r1,
              short img2, VALUE val2, const hugin_utils::FDiff2D & p2, float r2)
    : imgNr1(img1), i1(val1), p1(p1), r1(r1), imgNr2(img2), i2(val2), p2(p2), r2(r2)
    {
    }

    short imgNr1;
    VALUE i1;
    hugin_utils::FDiff2D p1;
    float r1;
    short imgNr2;
    VALUE i2;
    hugin_utils::FDiff2D p2;
    float r2;
};

typedef PointPairT<float> PointPair;
typedef PointPairT<vigra::RGBValue<float> > PointPairRGB;



// get the value_type of vector pixels and also single channel pixels.
#define VT_TRAITS_VEC(T1) \
template<> \
struct ValueTypeTraits<vigra::RGBValue<T1, 0u, 1u, 2u> > \
{ \
    typedef vigra::RGBValue<T1, 0u, 1u, 2u>::value_type value_type; \
};

#define VT_TRAITS(T1) \
template<> \
struct ValueTypeTraits<T1> \
{ \
    typedef T1 value_type; \
};

template <class T1>
struct ValueTypeTraits
{
    typedef typename T1::value_type value_type;
};

#if 0
VT_TRAITS_VEC(vigra::UInt8);
VT_TRAITS_VEC(vigra::Int16);
VT_TRAITS_VEC(vigra::UInt16);
VT_TRAITS_VEC(vigra::Int32);
VT_TRAITS_VEC(vigra::UInt32);
VT_TRAITS_VEC(float);
VT_TRAITS_VEC(double);
#endif

VT_TRAITS(vigra::UInt8);
VT_TRAITS(vigra::Int16);
VT_TRAITS(vigra::UInt16);
VT_TRAITS(vigra::Int32);
VT_TRAITS(vigra::UInt32);
VT_TRAITS(float);
VT_TRAITS(double);

#undef VT_TRAITS
#undef VT_TRAITS_VEC


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

/** get the maximum component of a vector (also works for single pixel types...) */
template <class V>
inline
V
getMaxComponent(vigra::RGBValue<V> const & v)
{
    return std::max(std::max(v.red(), v.green()), v.blue());
}

/** get the maximum component of a vector (also works for single pixel types...) */
template <class V>
inline
V
getMaxComponent(V v)
{
    return v;
}

/** get the maximum component of a vector (also works for single pixel types...) */
template <class V>
inline
V
getMinComponent(vigra::RGBValue<V> const & v)
{
    return std::max(std::max(v.red(), v.green()), v.blue());
}

/** get the maximum component of a vector (also works for single pixel types...) */
template <class V>
inline
V
getMinComponent(V v)
{
    return v;
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
                  hugin_utils::FDiff2D middle, double radius)
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

/** does nothing */
template <class T>
struct PassThroughFunctor
{
    typedef T result_type;

    T operator()(const T & a) const
    {
        return a;
    }

    template <class T2>
    T2 operator()(const T2 & a, const hugin_utils::FDiff2D & p) const
    {
        return a;
    }

    template <class T2, class A>
    A
    hdrWeight(T2 v, A a) const
    {
        return a;
    }

};


/** normalize a pixel to 0..1
 *  Only has an effect on integer pixel types
 */
template <class T>
typename vigra::NumericTraits<T>::RealPromote
normalizeValue(T v, vigra::VigraTrueType)
{
    return vigra::NumericTraits<T>::toRealPromote(v) / vigra::NumericTraits<T>::max();
}

template <class T>
typename vigra::NumericTraits<T>::RealPromote
normalizeValue(T v, vigra::VigraFalseType)
{
    return v;
}



template <class DestValueType>
struct LinearTransform
{
  public:
        /* the functors argument type (actually, since 
           <tt>operator()</tt> is a template, much more types are possible)
        */
    typedef DestValueType argument_type;

        /* the functors result type
        */
    typedef DestValueType result_type;

        /* init scale and offset
        */
    LinearTransform(float scale, float offset)
    : scale_(scale), offset_(offset)
    {}
    template <class SrcValueType>
    result_type operator()(SrcValueType const & s) const
    {
        return vigra::NumericTraits<result_type>::fromRealPromote(scale_ * (vigra::NumericTraits<SrcValueType>::toRealPromote(s) + offset_));
    }
  private:

    float scale_;
    float offset_;
};


struct ApplyLogFunctor
{
    float minv;
    float maxv;
    float scale;

    ApplyLogFunctor(float min_, float max_)
    {
        // protect against zeros in image data
        if (min_ == 0.0f) {
            min_ = 1e-5;
        }
        minv = std::log10(min_);
        maxv = std::log10(max_);
        scale = (maxv - minv)/255;
    }

    template <class T>
    unsigned char operator()(T v) const
    {
        typedef vigra::NumericTraits<vigra::UInt8>  DestTraits;
        return DestTraits::fromRealPromote((std::log10(float(v))-minv)/scale);
    }

    template <class T, unsigned int R, unsigned int G, unsigned int B>
    vigra::RGBValue<vigra::UInt8,0,1,2> operator()(const vigra::RGBValue<T,R,G,B> & v) const
    {
        typedef vigra::NumericTraits< vigra::RGBValue<vigra::UInt8,0,1,2> >  DestTraits;
        typedef vigra::NumericTraits< vigra::RGBValue<T,R,G,B> >  SrcTraits;
        return DestTraits::fromRealPromote((log10(SrcTraits::toRealPromote(v)) + (-minv))/scale);
    }
};


template <class TIn, class TOut=vigra::UInt8>
struct ApplyGammaFunctor
{
    float minv;
    float maxv;
    float gamma;
    float scale;

    ApplyGammaFunctor(TIn min_, TIn max_, float gamma_)
    {
        minv = min_;
        maxv = max_;
        gamma = gamma_;
        scale = float(maxv) - minv;
    }

    TOut operator()(TIn v) const
    {
        typedef vigra::NumericTraits<TOut>  DestTraits;
        return DestTraits::fromRealPromote(pow((float(v)-minv)/scale, gamma)*255);
    }

    vigra::RGBValue<TOut> operator()(const vigra::RGBValue<TIn> & v) const
    {
        typedef vigra::NumericTraits< vigra::RGBValue<TOut> >  DestTraits;
        typedef vigra::NumericTraits< vigra::RGBValue<TIn> >  SrcTraits;
        return DestTraits::fromRealPromote(pow((SrcTraits::toRealPromote(v)+(-minv))/scale, gamma)*255);
    }
};

// gamma correction with lookup table
template <>
struct ApplyGammaFunctor<vigra::UInt16, vigra::UInt8>
{
    vigra::UInt8 lut[65536];

    ApplyGammaFunctor(vigra::UInt16 min, vigra::UInt16 max, float gamma)
    {
        float scale = float(max) - min;
        for (int i=0; i<65536; i++) {
            lut[i] = hugin_utils::roundi(pow((float(i)-min)/scale, gamma)*255);
        }
    }

    vigra::UInt8 operator()(vigra::UInt16 v) const
    {
        return lut[v];
    }

    vigra::RGBValue<vigra::UInt8> operator()(const vigra::RGBValue<vigra::UInt16> & v) const
    {
        return vigra::RGBValue<vigra::UInt8>(lut[v[0]], lut[v[1]], lut[v[2]]);
    }
};

template <class SrcIterator, class SrcAccessor, class DestIterator, class DestAccessor, class T>
        void applyMapping(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> img,
                          vigra::pair<DestIterator, DestAccessor> dest, T min, T max, int mapping )
{

    switch (mapping)
    {
        case 0:
        {
            // linear
            float offset_ = -float(min);
            float scale_ = 255/float(max)-float(min);
            vigra::transformImage(img, dest,
                                  LinearTransform<typename DestAccessor::value_type>( scale_, offset_)
                                 );
            break;
        }
        case 1:
        {
            // log
            ApplyLogFunctor logfunc(min, max);
            transformImage(img, dest,
                           logfunc);
            break;
        }
        case 2:
        {
            // gamma
            ApplyGammaFunctor<T> logfunc(min, max, 1/2.2f);
            transformImage(img, dest,
                           logfunc);
            break;
        }
        default:
            vigra_fail("Unknown image mapping mode");
    }
}

template <class SrcImageIterator, class SrcAccessor,
class DestImageIterator, class DestAccessor, class Functor>
void
transformImageSpatial(SrcImageIterator src_upperleft,
                      SrcImageIterator src_lowerright, SrcAccessor sa,
                      DestImageIterator dest_upperleft, DestAccessor da,
                      Functor const & f, vigra::Diff2D ul) 
{
    vigra::Diff2D destSize = src_lowerright - src_upperleft;

    int offsetX=ul.x;
    for(; src_upperleft.y < src_lowerright.y; ++src_upperleft.y, ++dest_upperleft.y, ++ul.y)
    {
        typename SrcImageIterator::row_iterator s(src_upperleft.rowIterator());
        typename SrcImageIterator::row_iterator send(s+ destSize.x);
        typename DestImageIterator::row_iterator d(dest_upperleft.rowIterator());
        ul.x=offsetX;
        for(; s != send; ++s, ++d, ++ul.x) {
            da.set(f(sa(s), ul), d);
        }
    }
}

template <class SrcImageIterator, class SrcAccessor,
class DestImageIterator, class DestAccessor, class Functor>
void
transformImageSpatial(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                      vigra::pair<DestImageIterator, DestAccessor> dest,
                      Functor const & f, vigra::Diff2D ul)
{
    transformImageSpatial(src.first, src.second, src.third, dest.first, dest.second, f, ul);
}


} // namespace

#endif // VIGRA_EXT_UTILS_H

// -*- c-basic-offset: 4 -*-
/** @file impexalpha.hxx
 *
 *  Routines to save images with alpha masks.
 *
 *  These routines handle the conversion of byte alpha
 *  channels into the final output types.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: impexalpha.hxx 1951 2007-04-15 20:54:49Z dangelo $
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

#ifndef VIGRA_EXT_IMPEX_ALPHA_IMAGE_H
#define VIGRA_EXT_IMPEX_ALPHA_IMAGE_H

#include <iostream>
#include <vigra/imageiterator.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/initimage.hxx>
#include <vigra/numerictraits.hxx>

#include <vigra/impex.hxx>

namespace vigra {

#if 0

/** define values for mask true value. max for integers, 1 for floats
 */
template <class T1>
struct GetMaskTrue;

#define VIGRA_EXT_GETMASKTRUE(T1, S) \
template<> \
struct GetMaskTrue<T1> \
{ \
    static T1 get() \
{ \
	return S; \
} \
};

#define VIGRA_EXT_GETMASKMAX(T1) \
template<> \
struct GetMaskTrue<T1> \
{ \
    static T1 get() \
{ \
	return vigra::NumericTraits<T1>::max(); \
} \
};

VIGRA_EXT_GETMASKMAX(vigra::UInt8)
VIGRA_EXT_GETMASKMAX(vigra::Int16)
VIGRA_EXT_GETMASKMAX(vigra::UInt16)
VIGRA_EXT_GETMASKMAX(vigra::Int32)
VIGRA_EXT_GETMASKMAX(vigra::UInt32)
VIGRA_EXT_GETMASKTRUE(float, 1.0f)
VIGRA_EXT_GETMASKTRUE(double, 1.0)

#endif


template <class T1>
struct MaskConv;


template<>
struct MaskConv<vigra::UInt8>
{
    static vigra::UInt8 toUInt8(vigra::UInt8 v)
    {
        return v;
    }

    static vigra::UInt8 fromUInt8(vigra::UInt8 v)
    {
        return v;
    }
};

template<>
        struct MaskConv<vigra::UInt16>
{
    static vigra::UInt8 toUInt8(vigra::UInt16 v)
    {
        return v>>8;
    }


    static vigra::UInt16 fromUInt8(vigra::UInt8 v)
    {
        return (v<<8) + v;
    }
};

template<>
        struct MaskConv<vigra::Int16>
{
    static vigra::UInt8 toUInt8(vigra::Int16 v)
    {
        return v>>7;
    }


    static vigra::Int16 fromUInt8(vigra::UInt8 v)
    {
        return (v<<7)+ (v & 127);
    }
};

template<>
struct MaskConv<vigra::UInt32>
{
    static vigra::UInt8 toUInt8(vigra::UInt32 v)
    {
        return v>>24;
    }


    static vigra::UInt32 fromUInt8(vigra::UInt8 v)
    {
        return (v<<24) + (v<<16) + (v<<8) + v;
    }
};

template<>
struct MaskConv<vigra::Int32>
{
    static vigra::UInt8 toUInt8(vigra::Int32 v)
    {
        return v>>23;
    }


    static vigra::Int32 fromUInt8(vigra::UInt8 v)
    {
        return (v<<23) + (v<<15) + (v<<7) + (v & 127);
    }
};

template<>
struct MaskConv<float>
{
    static vigra::UInt8 toUInt8(float v)
    {
        return vigra::NumericTraits<vigra::UInt8>::fromRealPromote(v*255);
    }

    static float fromUInt8(vigra::UInt8 v)
    {
        return v/255.0f;
    }

};

template<>
struct MaskConv<double>
{
    static vigra::UInt8 toUInt8(double v)
    {
        return vigra::NumericTraits<vigra::UInt8>::fromRealPromote(v*255);
    }

    static double fromUInt8(vigra::UInt8 v)
    {
        return v/255.0;
    }

};


template <class Iter1, class Acc1, class Iter2, class Acc2>
class MultiImageMaskAccessor2
{
public:
        /** The accessors value_type: construct a pair that contains
            the corresponding image values.
        */
    typedef vigra::TinyVector<typename Acc1::value_type, 2> value_type;
    typedef typename Acc1::value_type component_type;
    typedef typename Acc2::value_type alpha_type;

        /** Construct from two image iterators and associated accessors.
        */
    MultiImageMaskAccessor2(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
    : i1_(i1), a1_(a1), i2_(i2), a2_(a2)
    {}

        /** read the current data item
        */
    template <class DIFFERENCE>
    value_type operator()(DIFFERENCE const & d) const
    {
        return value_type(a1_(i1_, d),
                          MaskConv<component_type>::fromUInt8(a2_(i2_, d)));
    }

        /** read the data item at an offset
        */
    template <class DIFFERENCE1, class DIFFERENCE2>
    value_type operator()(DIFFERENCE1 d, DIFFERENCE2 const & d2) const
    {
        d += d2;
        return value_type(a1_(i1_, d),
                          MaskConv<component_type>::fromUInt8(a2_(i2_, d)));
//                          a2_(i2_, d)? GetMaskTrue<component_type>::get() * a2_(i2_, d) : vigra::NumericTraits<component_type>::zero());

//        return std::make_pair(a1_(i1_, d1), a2_(i2_, d1));
    }

        /** write the current data item
         */
    template <class DIFFERENCE>
    value_type set(const value_type & vt, DIFFERENCE const & d) const
    {
        a1_.set(vt[0], i1_, d);
        a2_.set(MaskConv<component_type>::toUInt8(vt[1]));
                //GetMaskTrue<alpha_type>::get() : vigra::NumericTraits<alpha_type>::zero(), i2_, d);
    }

    /** scalar & scalar image */
    template <class V, class ITERATOR>
    void setComponent( V const & value, ITERATOR const & i, int idx ) const
    {
        switch (idx) {
        case 0:
            a1_.set(value, i1_, *i);
            break;
        case 1:
            a2_.set(MaskConv<V>::toUInt8(value), i2_, *i); 
            // ? GetMaskTrue<alpha_type>::get() : vigra::NumericTraits<alpha_type>::zero());
            break;
        default:
            vigra_fail("too many components in input value");
        }
    }

    /** read one component */
    template <class ITERATOR>
    component_type getComponent(ITERATOR const & i, int idx) const
    {
        switch (idx) {
            case 0:
                return a1_( i1_, *i );
            case 1:
                return MaskConv<component_type>::fromUInt8(a2_( i2_, *i ));
                //? GetMaskTrue<component_type>::get() : vigra::NumericTraits<component_type>::zero();
            default:
                vigra_fail("too many components in input value");
            // never reached, but here to silence compiler
                exit(1);
        }
    }

    template <class ITERATOR>
    unsigned int size ( ITERATOR const & i ) const
    {
        return 2;
    }

private:
    Iter1 i1_;
    Acc1 a1_;
    Iter2 i2_;
    Acc2 a2_;
};


// get: convert from UInt8 mask to native type
// read: convert from native type to UInt8 mask
template <class Iter1, class Acc1, class Iter2, class Acc2>
class MultiImageVectorMaskAccessor4
{
public:
        /** The accessors value_type: construct a pair that contains
            the corresponding image values.
        */
    typedef typename Acc1::value_type VT1;
    // todo.. check static_size, currently static_size == 4
    enum { static_size = 4 };

    typedef vigra::TinyVector<typename VT1::value_type, static_size> value_type;
    typedef typename value_type::value_type component_type;

    typedef typename Acc2::value_type alpha_type;

        /** Construct from two image iterators and associated accessors.
        */
    MultiImageVectorMaskAccessor4(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
    : i1_(i1), a1_(a1), i2_(i2), a2_(a2)
    {}

        /** read the current data item
        */
    template <class DIFFERENCE>
    value_type operator()(DIFFERENCE const & d) const
    {
        const VT1 & v1 = a1_.get(i1_,d);
        return value_type(v1[0],
                          v1[1],
                          v1[2],
                          MaskConv<component_type>::fromUInt8(a2_(i2_, d)));
    }

        /** read the data item at an offset
        */
    template <class DIFFERENCE1, class DIFFERENCE2>
    value_type operator()(DIFFERENCE1 d, DIFFERENCE2 const & d2) const
    {
        d += d2;
        const VT1 & v1 = a1_.get(i1_,d);
        return value_type(v1[0],
                          v1[1],
                          v1[2],
                          MaskConv<component_type>::fromUInt8(a2_(i2_, d)));
    }

        /** write the current data item
         */
    template <class DIFFERENCE>
    value_type set(const value_type & vt, DIFFERENCE const & d) const
    {
        Iter1 i1(i1_);
        i1 +=d;
        a1_.setComponent(vt[0], i1_, 0);
        a1_.setComponent(vt[1], i1_, 1);
        a1_.setComponent(vt[2], i1_, 2);
        a2_.set(MaskConv<component_type>::toUInt8(vt[3]), i2_, d);
    }

    /** vector & scalar image */
    template <class V, class ITERATOR>
    void setComponent( V const & value, ITERATOR const & i, int idx ) const
    {
        if ( idx < static_size - 1 ) {
            a1_.setComponent(value, i1_, *i, idx);
        } else if ( idx == static_size - 1 ) {
            a2_.set(MaskConv<V>::toUInt8(value), i2_, *i);
        } else {
            vigra_fail("too many components in input value");
        }
    }

    /** read one component */
    template <class ITERATOR>
    component_type getComponent(ITERATOR const & i, int idx) const
    {
        if ( idx < static_size - 1 ) {
            return a1_.getComponent(i1_, *i, idx);
        } else 
#ifndef DEBUG
            return MaskConv<component_type>::fromUInt8(a2_(i2_, *i));
#else
            if ( idx == static_size - 1 ) {
                return MaskConv<component_type>::fromUInt8(a2_(i2_, *i));
        } else {
            vigra_fail("too many components in input value");
            // just to silence the compiler warning. this is
            // never reached, since vigra_fail will always
            // throw an exception.
            throw 0;
        }
#endif
    }

    template <class ITERATOR>
    unsigned int size ( ITERATOR const & i ) const
    {
        return static_size;
    }

  private:
    Iter1 i1_;
    Acc1 a1_;
    Iter2 i2_;
    Acc2 a2_;
};


// scalar image
template<class SrcIterator, class SrcAccessor,
         class AlphaIterator, class AlphaAccessor>
void exportImageAlpha(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra::ImageExportInfo const & info,
		      vigra::VigraTrueType)
{
    typedef MultiImageMaskAccessor2<SrcIterator, SrcAccessor, AlphaIterator, AlphaAccessor> MAcc;

    exportImage(vigra::CoordinateIterator(),
                vigra::CoordinateIterator() + (image.second - image.first),
                MAcc(image.first, image.third, alpha.first, alpha.second),
                info);
}


// vector image
template<class SrcIterator, class SrcAccessor,
         class AlphaIterator, class AlphaAccessor>
void exportImageAlpha(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra::ImageExportInfo const & info,
		      vigra::VigraFalseType)
{
    typedef MultiImageVectorMaskAccessor4<SrcIterator, SrcAccessor, AlphaIterator, AlphaAccessor> MAcc;

    exportImage(vigra::CoordinateIterator(), vigra::CoordinateIterator(image.second - image.first),
                MAcc(image.first, image.third, alpha.first, alpha.second), info );
}


/** export an image with a differently typed alpha channel.
 *
 *  This function handles the merging of the images and the
 *  scales the alpha channel to the correct values.
 *
 *  can write to all output formats that support 4 channel images.
 *  (currently only png and tiff).
 */
template<class SrcIterator, class SrcAccessor,
         class AlphaIterator, class AlphaAccessor>
void exportImageAlpha(vigra::triple<SrcIterator, SrcIterator, SrcAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra::ImageExportInfo const & info)
		      {
    typedef typename vigra::NumericTraits<typename SrcAccessor::value_type>::isScalar is_scalar;
    // select function for scalar, or vector image, depending on source type.
    // the alpha image has to be scalar all the time. stuff will break with strange
    // compile error if it isn't
    exportImageAlpha( image, alpha, info, is_scalar());
}


// vector image
template<class DestIterator, class DestAccessor,
         class AlphaIterator, class AlphaAccessor>
void importImageAlpha(vigra::ImageImportInfo const & info,
		      std::pair<DestIterator, DestAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      		      vigra::VigraFalseType)
{
    vigra_precondition(image.second(image.first).size() == 3,
                       "only scalar and 3 channel (vector) images supported by impexalpha.hxx");

    typedef MultiImageVectorMaskAccessor4<DestIterator, DestAccessor, AlphaIterator, AlphaAccessor> MAcc;
    importImage(info,
                vigra::CoordinateIterator(),
                MAcc(image.first, image.second, alpha.first, alpha.second) );
}

// scalar image
template<class DestIterator, class DestAccessor,
         class AlphaIterator, class AlphaAccessor>
void importImageAlpha(vigra::ImageImportInfo const & info,
		      std::pair<DestIterator, DestAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha,
		      vigra::VigraTrueType)
{
    typedef MultiImageMaskAccessor2<DestIterator, DestAccessor, AlphaIterator, AlphaAccessor> MAcc;

    importImage(info, vigra::CoordinateIterator(),
                MAcc(image.first, image.second, alpha.first, alpha.second) );
}


/** import an image with a differently typed alpha channel.
 *
 *  This function loads an image, and splits it into a
 *  color image and a separate alpha channel, the alpha channel
 *  should be a 8 bit image.
 *
 *  If the image doesn't contain any alpha channel, a completely
 *  white one is created.
 *
 *  can write to all output formats that support 4 channel images.
 *  (currently only png and tiff).
 */
template<class DestIterator, class DestAccessor,
         class AlphaIterator, class AlphaAccessor>
void importImageAlpha(vigra::ImageImportInfo const & info,
		      vigra::pair<DestIterator, DestAccessor> image,
		      std::pair<AlphaIterator, AlphaAccessor> alpha
		      )
{
    typedef typename vigra::NumericTraits<typename DestAccessor::value_type>::isScalar is_scalar;

    if (info.numExtraBands() == 1 ) {
	// import image and alpha channel
	importImageAlpha(info, image, alpha, is_scalar());
    } else if (info.numExtraBands() == 0 ) {
	// no alphachannel in file, import as usual.
	importImage(info, image);
	// fill alpha image
	vigra::initImage(alpha.first ,
                         alpha.first + vigra::Diff2D(info.width(), info.height()),
                         alpha.second,
                         255);
    } else {
	vigra_fail("Images with two or more channel are not supported");
    }
}

} // namespace

#endif // VIGRA_EXT_IMPEX_ALPHA_IMAGE_H

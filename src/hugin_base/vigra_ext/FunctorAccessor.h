// -*- c-basic-offset: 4 -*-
/** @file FunctorAccessor.h
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

#ifndef _FUNCTORACCESSOR_H
#define _FUNCTORACCESSOR_H

#include <vigra/numerictraits.hxx>

namespace vigra_ext {


/** This class can be used to apply a function when reading
    the input image.

    Can be used to apply point operations temporarily, like scaling,
    gamma correction etc.

    This is a read only accessor, writing is not supported.
 */
template <class Functor, class Accessor>
class ReadFunctorAccessor
{
  public:
    typedef typename Functor::result_type value_type;
    ReadFunctorAccessor(Functor f, Accessor a)
        : m_f(f), m_a(a)
    {
    }

    /** Get functor result
    template <class A, class B>
    void function(A a, B b)
    {
    };
     */

    template <typename ITERATOR_, typename DIFFERENCE_>
    typename Functor::result_type operator()(ITERATOR_ const & i, DIFFERENCE_ d) const
    {
        return m_f(m_a(i,d));
    }

    /** Get functor result
     */
    template <class ITERATOR>
    typename Functor::result_type operator()(ITERATOR const & i) const {
                return m_f(m_a(i)); }


protected:
    Functor m_f;
    Accessor m_a;
};

/** This class can be used to apply a function when writing
    to an image

    Can be used to apply point operations temporarily, like scaling,
    gamma correction etc.

    This is a write only accessor, reading is not supported.
 */
template <class Functor, class Accessor, class ValueType>
class WriteFunctorAccessor
{
public:

    typedef ValueType value_type;

    WriteFunctorAccessor(Functor f, Accessor a)
        : m_f(f), m_a(a)
    {
    }

    /** Set functor result
     */
    template <class Value, class ITERATOR>
    void set(Value const & v, ITERATOR const & i) const
    {
	m_a.set(m_f(v), i);
    }

    /** Set functor result
     */
    template <class Value, class ITERATOR_, class DIFFERENCE_>
    void set(Value const & v, ITERATOR_ const & i, DIFFERENCE_ d) const
    {
        m_a.set(m_f(v),i,d);
    }

    Functor m_f;
    Accessor m_a;
};



/** define a write only accessor for a virtual Image<TinyVector<Acc1::value_type>, 2>
    image, which actually consists of two Images.

    Useful to split an image into gray and alpha images while loading, like it is
    shown in the following example:

    \code
        vigra::ImageImportInfo info(argv[1]);

        if(info.numBands() == 2 && info.numExtraBands() == 1)
        {
	    vigra::BImage image;
	    vigra::BImage mask;
            image.resize(info.width(), info.height());
            mask.resize(info.width(), info.height());

	    // construct special reading accessor, to split
	    // the image into two images while reading
	    vigra_ext::SplitVector2Accessor<BImage::Iterator,
		                            BImage::Accessor,
                                            BImage::Iterator,
		                            BImage::Accessor>
		splitA(image.upperLeft(), image.accessor(),
		       mask.upperLeft(), mask.accessor());

            importImage(info, Diff2D(), splitA );
    \endcode
 */
template <class Iter1, class Acc1, class Iter2, class Acc2>
class SplitVector2Accessor
{
public:
    /** the vector's value_type
     */
    typedef vigra::TinyVector<typename Acc1::value_type, 2> value_type;
    typedef typename value_type::value_type component_type;

    /** Construct from two image iterators and associated accessors.
     */
    SplitVector2Accessor(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
	: i1_(i1), a1_(a1), i2_(i2), a2_(a2)
	{}

    /** scalar & scalar image */
    template <class V, class ITERATOR>
    void setComponent( V const & value, ITERATOR const & i, int idx ) const
    {
	switch (idx) {
	case 0:
	    a1_.set(value, i1_, *i);
	    break;
	case 1:
	    a2_.set(value, i2_, *i);
	    break;
	default:
	    vigra_fail("too many components in input value");
	}
    }

	/** return the size (Number of Bands) */
	template <class ITERATOR>
	unsigned int size(ITERATOR const & i) const
	{
		return 2;
	}

    Iter1 i1_;
    Acc1  a1_;
    Iter2 i2_;
    Acc2  a2_;
};

/** split a vector image into a vector and a scalar image
 *
 *  like SplitVector2Accessor, but for the vector -> vector, scalar
 *  case.
 *
 *  the template parameter SIZE gives the length of each vector
 *  in the input image. components 0..SIZE-2, are put into the
 *  image 1 (must be a vector image), and component SIZE-1
 *  is stored in image 2 (should be a scalar image)
 */
template <class Iter1, class Acc1, class Iter2, class Acc2, int SIZE>
class SplitVectorNAccessor
{
public:
    /** the vector's value_type
     */
    typedef vigra::TinyVector<typename Acc1::value_type, SIZE> value_type;
    typedef typename value_type::value_type component_type;

    /** Construct from two image iterators and associated accessors.
     */
    SplitVectorNAccessor(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
	: i1_(i1), a1_(a1), i2_(i2), a2_(a2)
	{}

    /** vector & scalar image */
    template <class V, class ITERATOR>
    void setComponent( V const & value, ITERATOR const & i, int idx ) const
    {
	if ( idx < SIZE - 1 ) {
	    a1_.setComponent(value, i1_, *i, idx);
	} else if ( idx == SIZE - 1 ) {
	    a2_.set(value, i2_, *i);
	} else {
	    vigra_fail("too many components in input value");
	}
    }

	/** return the size (Number of Bands) */
	template <class ITERATOR>
	unsigned int size(ITERATOR const & i) const
	{
		return SIZE;
	}

    Iter1 i1_;
    Acc1  a1_;
    Iter2 i2_;
    Acc2  a2_;
};

/** merge two scalar images into a vector image.
 *
 *  the inverse to SplitVector2Accessor.
 *
 */
template <class Iter1, class Acc1, class Iter2, class Acc2>
class MergeScalarScalar2VectorAccessor
{
public:
    /** the vector's value_type
     */
    typedef vigra::TinyVector<typename Acc1::value_type, 2> value_type;
    typedef typename value_type::value_type component_type;

    /** Construct from two image iterators and associated accessors.
     */
    MergeScalarScalar2VectorAccessor(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
	: i1_(i1), a1_(a1), i2_(i2), a2_(a2)
	{}

        /** read the current data item
        */
    template <class DIFFERENCE_>
    value_type operator()(DIFFERENCE_ const & d) const
    {
        return value_type(a1_(i1_, d), a2_(i2_, d));
    }

        /** read one component */
    template <class ITERATOR>
    component_type getComponent(ITERATOR const & i, int idx) const
    {
	switch (idx) {
	case 0:
	    return a1_( i1_, *i );
	case 1:
	    return a2_( i2_, *i );
	default:
	    vigra_fail("too many components in input value");
            // never reached, but here to silence compiler
            exit(1);
	}
    }

        /** read one component, with offset */
    template <class ITERATOR, class DIFFERENCE_>
    component_type const & getComponent(ITERATOR const & i, DIFFERENCE_ const & d, int idx) const
    {
        i += d;
	switch (idx) {
	case 0:
	    return a1_.getComponent(i1_, *i, idx);
	case 1:
	    return a2_.getComponent(i2_, *i, idx);
	default:
	    vigra_fail("too many components in input value");
	}
    }

	/** return the size (Number of Bands) */
	template <class ITERATOR>
	unsigned int size(ITERATOR const & i) const
	{
		return 2;
	}

    Iter1 i1_;
    Acc1  a1_;
    Iter2 i2_;
    Acc2  a2_;
};


/** merge a vector and a scalar image into a vector image.
 *
 *  This virtually "appends" the scalar image plane to
 *  the vector image.
 *
 *  the inverse to SplitVectorNAccessor.
 *
 */
template <class Iter1, class Acc1, class Iter2, class Acc2, int SIZE>
class MergeVectorScalar2VectorAccessor
{
public:
    /** the vector's value_type
     */
    typedef typename Acc1::value_type image1_type;
    typedef typename Acc2::value_type image2_type;

    typedef typename image1_type::value_type component_type;

    typedef vigra::TinyVector<component_type, SIZE> value_type;

    /** Construct from two image iterators and associated accessors.
     */
    MergeVectorScalar2VectorAccessor(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
	: i1_(i1), a1_(a1), i2_(i2), a2_(a2)
	{}

        /** read the current data item
        */
    template <class DIFFERENCE_>
    value_type operator()(DIFFERENCE_ const & d) const
    {
	value_type ret;
	typename value_type::iterator it = ret.begin();
	const image1_type & i1 = a1_(i1_, d);
	for ( typename image1_type::const_iterator it1 = i1.begin();
	     it1 != i1.end(); ++it1 )
	{
	    *it = *it1;
	    it++;
	}
	*it = a2_(i2_, d);
        return ret;
    }

        /** read one component */
    template <class ITERATOR>
    component_type getComponent(ITERATOR const & i, int idx) const
    {
	if ( idx < SIZE - 1 ) {
	    return a1_.getComponent(i1_, *i, idx);
	} else if ( idx == SIZE - 1 ) {
	    return a2_(i2_, *i);
	} else {
	    vigra_fail("too many components in input value");
            // just to silence the compiler warning. this is
            // never reached, since vigra_fail will always
            // throw an exception.
            throw 0;
	}
    }

        /** read one component, with offset */
    template <class ITERATOR, class DIFFERENCE_>
    component_type const getComponent(ITERATOR i, DIFFERENCE_ const & d, int idx) const
    {
        i += d;
	if ( idx < SIZE - 1 ) {
	    return a1_.getComponent(i1_, *i, idx);
	} else if ( idx == SIZE - 1 ) {
	    return a2_(i2_, *i);
	} else {
	    vigra_fail("too many components in input value");
            // just to silence the compiler warning. this is
            // never reached, since vigra_fail will always
            // throw an exception.
            throw 0;
	}
    }


	/** return the size (Number of Bands) */
	template <class ITERATOR>
	unsigned int size(ITERATOR const & i) const
	{
		return SIZE;
	}

    Iter1 i1_;
    Acc1  a1_;
    Iter2 i2_;
    Acc2  a2_;
};


/** An accessor to encapsulate write access to a multiband image,
    and move divide it into two images.

    This is particulary useful, if a multiband image should be splitted
    into separate images during import operations. Then one doesn't
    need to create a temporary image.

    This can be used to copy a 4 band image into a 3 band image
    and a 1 band image, with a single copyImage, or during other
    operations.

    For example, some images contain an alpha channel, and depending
    on the application, this doesn't need to have the same type, for
    example, float RGB channels, uint8 mask channel. Many algorithms
    provided by vigra also expect the masks and the image in separate
    images.

    The following image combinations are supported so far:

      - vector -> scalar, scalar
      - vector -> vector, scalar

    This accessor is quite slow. It checks the vector indicies on
    every access.

    @bug This is not a complete accessor, only write operations are supported.
    @bug value_type is not specified correctly, I don't know how to merge
         them properly with template programming.

    Requirements:
     both images need to have the same elementary type
*/
template <class Iter1, class Acc1, class Iter2, class Acc2, int SIZE>
class ImageSplittingAccessor
{
public:
    /** value type of image 1 */
    typedef typename Acc1::value_type image_type1;

    /** value type of image 2 */
    typedef typename Acc2::value_type image_type2;

    /** @bug how to combine two value types into one? */
    //    typedef image_type1 value_type;

        /** Construct from two image iterators and associated accessors.
        */
    ImageSplittingAccessor(Iter1 i1, Acc1 a1, Iter2 i2, Acc2 a2)
	: i1_(i1), a1_(a1), i2_(i2), a2_(a2)
    {}

    /** write value V into the two images.
     *
     *  V has to be a stl compatible vector type,
     *  the two images can be of vector or scalar types.
     *
     */
    template <class V, class ITERATOR>
    void setComponent(V const & value, ITERATOR const & i, int idx) const
    {
	setComponentIsScalar(value, i, idx,
			     vigra::NumericTraits<image_type1>::isScalar() );
    }


#if 0
        /** Write the current data item. The type <TT>V</TT> must be
            a sequence type, Likewise the value types
            of image 1 and image 2.

	    Its size must be size(image_type1) + size(image_type2);
	
            The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR>
    void setVector2VectorVector(V const & value, ITERATOR const & i) const
    {
	// get pixels at current position indicated by iterator
	image_type1 & v1 = a1_(i);
	image_type2 & v2 = a2_(i2_, i - i1_);
	// TODO: check if the size of both images is correct

	// copy into first image
	typename V::iterator vIt = value.begin();
	typename image_type1::iterator v1It = v1.begin();
	while ( v1It != v1.end() && vIt != value.end()) {
	    *v1It = detail::RequiresExplicitCast<VALUETYPE>::cast(*vIt);
	    ++v1It;
	    ++vIt;
	}
	// copy rest into second image
	typename image_type2::iterator v2It = v2.begin();
	while ( v2It != v1.end() && vIt != value.end()) {
	    *v2It = detail::RequiresExplicitCast<VALUETYPE>::cast(*vIt);
	    ++v2It;
	    ++vIt;
	}
	
	

	for (int i=0; i < value.size(); i++) {
	    if (i <

	*i = detail::RequiresExplicitCast<VALUETYPE>::cast(value); }

        /** Write the data item at an offset (can be 1D or 2D or higher order difference)..
            The type <TT>V</TT> of the passed
            in <TT>value</TT> is automatically converted to <TT>VALUETYPE</TT>.
            In case of a conversion floating point -> intergral this includes rounding and clipping.
        */
    template <class V, class ITERATOR, class DIFFERENCE_>
    void set(V const & value, ITERATOR const & i, DIFFERENCE_ const & diff) const
    {
        i[diff]= detail::RequiresExplicitCast<VALUETYPE>::cast(value);
    }

#endif

protected:
    /** if first dest image is scalar */
    template <class V, class ITERATOR>
    void setComponentIsScalar(V const & value, ITERATOR const & i, int idx,
			      vigra::VigraTrueType) const
    {
	setComponentScalarIsScalar(value, i, idx,
				   vigra::NumericTraits<image_type2>::isScalar() );
    }

    /** if first dest image is vector image */
    template <class V, class ITERATOR>
    void setComponentIsScalar(V const & value, ITERATOR const & i, int idx,
			      vigra::VigraFalseType) const
    {
	setComponentVectorIsScalar(value, i, idx,
				   vigra::NumericTraits<image_type2>::isScalar() );
    }

    /** if scalar & scalar image */
    template <class V, class ITERATOR>
    void setComponentScalarIsScalar(V const & value, ITERATOR const & i, int idx,
				    vigra::VigraTrueType) const
    {
	switch (idx) {
	case 0:
	    a1_.set(value, i);
	    break;
	case 1:
	    a2_.set(value, i2_, i - i1_);
	    break;
	default:
	    vigra_fail("too many components in input value");
	}
    }

    /** if scalar & vector image */
    template <class V, class ITERATOR>
    void setComponentScalarIsVector(V const & value, ITERATOR const & i, int idx,
				    vigra::VigraTrueType) const
    {
	vigra_fail("vector -> scalar, vector accessor not implemented");
    }

    /** if vector & scalar image */
    template <class V, class ITERATOR>
    void setComponentVectorIsScalar(V const & value, ITERATOR const & i, int idx,
				    vigra::VigraTrueType) const
    {
	image_type1 & v1 = a1_(i);
	typename image_type1::size_type s1 = v1.size();
	if (idx < s1) {
	    a1_.setComponent(value, i, idx);
	} else if ( idx == s1) {
	    a2_.set(value, i2_, i - i1_);
	} else {
	    vigra_fail("too many components in input value");
	}
    }

    /** if vector & vector image */
    template <class V, class ITERATOR>
    void setComponentVectorIsVector(V const & value, ITERATOR const & i, int idx,
				    vigra::VigraTrueType) const
    {
	vigra_fail("vector -> vector, vector accessor not implemented");
    }

    Iter1 i1_;
    Acc1  a1_;
    Iter2 i2_;
    Acc2  a2_;
};

/** a sample functor that can be used to multiply pixel values with a constant*/
template <class T>
struct Multiply
{
    typedef T result_type;

    Multiply(T factor)
        : m_factor(factor)
    {}

    template <class PixelType>
    PixelType operator()(PixelType const& v) const
    {
        return vigra::NumericTraits<result_type>::fromRealPromote(v * m_factor);
    }

    T m_factor;
};

}  // namespace



#endif // _FUNCTORACCESSOR_H

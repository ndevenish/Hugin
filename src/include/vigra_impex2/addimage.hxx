/************************************************************************/
/*                                                                      */
/*               Copyright 1998-2002 by Ullrich Koethe                  */
/*       Cognitive Systems Group, University of Hamburg, Germany        */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    ( Version 1.2.0, Aug 07 2003 )                                    */
/*    You may use, modify, and distribute this software according       */
/*    to the terms stated in the LICENSE file included in               */
/*    the VIGRA distribution.                                           */
/*                                                                      */
/*    The VIGRA Website is                                              */
/*        http://kogs-www.informatik.uni-hamburg.de/~koethe/vigra/      */
/*    Please direct questions, bug reports, and contributions to        */
/*        koethe@informatik.uni-hamburg.de                              */
/*                                                                      */
/*  THIS SOFTWARE IS PROVIDED AS IS AND WITHOUT ANY EXPRESS OR          */
/*  IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED      */
/*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. */
/*                                                                      */
/************************************************************************/


#ifndef VIGRA_ADDIMAGE_HXX
#define VIGRA_ADDIMAGE_HXX

#include "vigra/stdimage.hxx"

namespace vigra {

#define VIGRA_DEFINE_ITERATORTRAITS(VALUETYPE, ACCESSOR, CONSTACCESSOR) \
    template<> \
    struct IteratorTraits< \
        BasicImageIterator<VALUETYPE, VALUETYPE **> > \
    { \
        typedef BasicImageIterator<VALUETYPE, VALUETYPE **> \
                                                     Iterator; \
        typedef Iterator                             iterator; \
        typedef iterator::iterator_category          iterator_category; \
        typedef iterator::value_type                 value_type; \
        typedef iterator::reference                  reference; \
        typedef iterator::index_reference            index_reference; \
        typedef iterator::pointer                    pointer; \
        typedef iterator::difference_type            difference_type; \
        typedef iterator::row_iterator               row_iterator; \
        typedef iterator::column_iterator            column_iterator; \
        typedef ACCESSOR<VALUETYPE >                 default_accessor; \
        typedef ACCESSOR<VALUETYPE >                 DefaultAccessor; \
    }; \
    template<> \
    struct IteratorTraits< \
        ConstBasicImageIterator<VALUETYPE, VALUETYPE **> > \
    { \
        typedef \
          ConstBasicImageIterator<VALUETYPE, VALUETYPE **> \
                                                     Iterator; \
        typedef Iterator                             iterator; \
        typedef iterator::iterator_category          iterator_category; \
        typedef iterator::value_type                 value_type; \
        typedef iterator::reference                  reference; \
        typedef iterator::index_reference            index_reference; \
        typedef iterator::pointer                    pointer; \
        typedef iterator::difference_type            difference_type; \
        typedef iterator::row_iterator               row_iterator; \
        typedef iterator::column_iterator            column_iterator; \
        typedef CONSTACCESSOR<VALUETYPE >            default_accessor; \
        typedef CONSTACCESSOR<VALUETYPE >            DefaultAccessor; \
    };

/** \addtogroup StandardImageTypes Additional Standard Image Types

    \brief Some more common instantiations of the \ref vigra::BasicImage template
*/
//@{

VIGRA_DEFINE_ITERATORTRAITS(unsigned short, StandardValueAccessor, StandardConstValueAccessor)

    /** Unsigned Short (16-bit unsigned) image.
        It uses \ref vigra::BasicImageIterator and \ref vigra::StandardAccessor and
        their const counterparts to access the data.

        <b>\#include</b> "<a href="stdimage_8hxx-source.html">vigra/stdimage.hxx</a>"<br>
        Namespace: vigra
    */
typedef BasicImage<unsigned short> USImage;

VIGRA_DEFINE_ITERATORTRAITS(unsigned int, StandardValueAccessor, StandardConstValueAccessor)

    /** Unsigned Integer (32-bit unsigned) image.
        It uses \ref vigra::BasicImageIterator and \ref vigra::StandardAccessor and
        their const counterparts to access the data.

        <b>\#include</b> "<a href="stdimage_8hxx-source.html">vigra/stdimage.hxx</a>"<br>
        Namespace: vigra
    */
typedef BasicImage<unsigned int> UIImage;

VIGRA_DEFINE_ITERATORTRAITS(RGBValue<short>, RGBAccessor, RGBAccessor)

    /** Short (3x 16-bit signed) RGB image.
        The pixel type is \ref vigra::RGBValue "vigra::RGBValue<short>".
        It uses \ref vigra::BasicImageIterator and \ref vigra::RGBAccessor and
        their const counterparts to access the data.

        <b>\#include</b> "<a href="stdimage_8hxx-source.html">vigra/stdimage.hxx</a>"<br>
        Namespace: vigra
    */
typedef BasicImage<RGBValue<short> > SRGBImage;

VIGRA_DEFINE_ITERATORTRAITS(RGBValue<unsigned short>, RGBAccessor, RGBAccessor)

    /** Unsigned short (3x 16-bit unsigned) RGB image.
        The pixel type is \ref vigra::RGBValue "vigra::RGBValue<unsigned short>".
        It uses \ref vigra::BasicImageIterator and \ref vigra::RGBAccessor and
        their const counterparts to access the data.

        <b>\#include</b> "<a href="stdimage_8hxx-source.html">vigra/stdimage.hxx</a>"<br>
        Namespace: vigra
    */
typedef BasicImage<RGBValue<unsigned short> > USRGBImage;

VIGRA_DEFINE_ITERATORTRAITS(RGBValue<unsigned int>, RGBAccessor, RGBAccessor)

    /** Unsigned Integer (3x 32-bit unsigned) RGB image.
        The pixel type is \ref vigra::RGBValue "RGBValue<unsigned int>".
        It uses \ref vigra::BasicImageIterator and \ref vigra::RGBAccessor and
        their const counterparts to access the data.

        <b>\#include</b> "<a href="stdimage_8hxx-source.html">vigra/stdimage.hxx</a>"<br>
        Namespace: vigra
    */
typedef BasicImage<RGBValue<unsigned int> > UIRGBImage;

#define VIGRA_PIXELTYPE TinyVector<short, 2>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<short, 3>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<short, 4>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<int, 2>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<int, 3>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<int, 4>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<unsigned int, 2>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<unsigned int, 3>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE
#define VIGRA_PIXELTYPE TinyVector<unsigned int, 4>
VIGRA_DEFINE_ITERATORTRAITS(VIGRA_PIXELTYPE, VectorAccessor, VectorAccessor)
#undef VIGRA_PIXELTYPE

#undef VIGRA_DEFINE_ITERATORTRAITS

} // namespace

#endif


#ifndef _VIGRA_IMPEX2_EXTRAIMAGETRAITS_HXX
#define _VIGRA_IMPEX2_EXTRAIMAGETRAITS_HXX


#include "vigra/stdimage.hxx"
#include "vigra/iteratortraits.hxx"

namespace vigra {

#define VIGRA_DEFINE_ITERATORTRAITS(VALUETYPE, ACCESSOR, CONSTACCESSOR) \
    template<> \
    struct IteratorTraits< \
        vigra::BasicImageIterator<VALUETYPE, VALUETYPE **> > \
    { \
        typedef vigra::BasicImageIterator<VALUETYPE, VALUETYPE **> \
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
          vigra::ConstBasicImageIterator<VALUETYPE, VALUETYPE **> \
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

VIGRA_DEFINE_ITERATORTRAITS(unsigned short, vigra::StandardValueAccessor, vigra::StandardConstValueAccessor)

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


#ifndef NO_PARTIAL_TEMPLATE_SPECIALIZATION

// define traits for BasicImageIterator instanciations that
// were not explicitly defined above
template <class T>
struct IteratorTraits<BasicImageIterator<T, T **> >
{
    typedef BasicImageIterator<T, T **>          Iterator;
    typedef Iterator                             iterator;
    typedef typename iterator::iterator_category iterator_category;
    typedef typename iterator::value_type        value_type;
    typedef typename iterator::reference         reference;
    typedef typename iterator::index_reference   index_reference;
    typedef typename iterator::pointer           pointer;
    typedef typename iterator::difference_type   difference_type;
    typedef typename iterator::row_iterator      row_iterator;
    typedef typename iterator::column_iterator   column_iterator;
    typedef StandardAccessor<T>                  DefaultAccessor;
    typedef StandardAccessor<T>                  default_accessor;
};

template <class T>
struct IteratorTraits<ConstBasicImageIterator<T, T **> >
{
    typedef ConstBasicImageIterator<T, T **> Iterator;
    typedef Iterator                               iterator;
    typedef typename iterator::iterator_category   iterator_category;
    typedef typename iterator::value_type          value_type;
    typedef typename iterator::reference           reference;
    typedef typename iterator::index_reference     index_reference;
    typedef typename iterator::pointer             pointer;
    typedef typename iterator::difference_type     difference_type;
    typedef typename iterator::row_iterator        row_iterator;
    typedef typename iterator::column_iterator     column_iterator;
    typedef StandardConstAccessor<T>               DefaultAccessor;
    typedef StandardConstAccessor<T>               default_accessor;
};

#endif

#endif // _VIGRA_IMPEX2_EXTRAIMAGETRAITS_HXX

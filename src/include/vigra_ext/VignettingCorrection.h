// -*- c-basic-offset: 4 -*-
/** @file VignettingCorrection.h
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

#ifndef VIGRA_EXT_VIGNETTING_CORRECTION_H
#define VIGRA_EXT_VIGNETTING_CORRECTION_H

#include <vector>
#include <functional>

#include <vigra/stdimage.hxx>
#include <vigra/transformimage.hxx>
#include <vigra/combineimages.hxx>
#include <vigra/functorexpression.hxx>

#include <boost/bind.hpp>

//#define DEBUG_WRITE_FILES

namespace vigra_ext{

template <class VT1, class VT2, class InvResp, class Adjust>
class VigCorrFlatDivFunctor
{
public:
    /** the functor's first argument type
     */
    typedef VT1 first_argument_type;
    
    /** the functor's second argument type
     */
    typedef VT2 second_argument_type;
     
    /** the functor's result type
     */
    typedef typename vigra::NumericTraits<VT1>::RealPromote result_type;

    typedef result_type RVT1;
    typedef typename vigra::NumericTraits<VT2>::RealPromote RVT2;

    VigCorrFlatDivFunctor(RVT2 mean, const InvResp & fr, const Adjust & adj)
        : m_InvResp(fr), m_Adjust(adj), m_mean(mean)
        { }

    InvResp m_InvResp;
    Adjust m_Adjust;
    RVT2 m_mean;
    
    /** calculate transform. 
     */
    result_type operator()(first_argument_type const & v1, second_argument_type const & v2) const
    {
        // apply inverse response/gamma correction, vignetting correction by
        // division and possible brightness adjust
        RVT1 i(m_InvResp(v1));
        RVT2 corr((v2)/m_mean);
        i /= corr;
        return m_Adjust( i );
    }
};

template <class VT1, class InvResp, class VigFunc, class Adjust>
class VigCorrDivFunctor
{
public:
    /** the functor's first argument type
     */
    typedef VT1 first_argument_type;
    
    /** the functor's result type
     */
    typedef typename vigra::NumericTraits<VT1>::RealPromote result_type;
    typedef result_type RealVT1;

    VigCorrDivFunctor(const InvResp & fr, const VigFunc & vf, const Adjust & adj)
        : m_InvResp(fr), m_VigFunc(vf), m_Adjust(adj)
        { }

    InvResp m_InvResp;
    VigFunc m_VigFunc;
    Adjust m_Adjust;
    
    /** calculate transform. 
     */
    result_type operator()(first_argument_type const & v1, float x, float y) const
    {
        // apply inverse response/gamma correction, vignetting correction by
        // division and possible brightness adjust
        return m_Adjust(m_InvResp(v1) / m_VigFunc(x,y));
    }
};



template <class VT1, class VT2, class InvResp, class Adjust>
class VigCorrFlatAddFunctor
{
public:
    /** the functor's first argument type
     */
    typedef VT1 first_argument_type;
    
    /** the functor's second argument type
     */
    typedef VT2 second_argument_type;
     
    /** the functor's result type
     */
    typedef typename vigra::NumericTraits<VT1>::RealPromote result_type;

    typedef result_type RealVT1;
    typedef typename vigra::NumericTraits<VT2>::RealPromote RVT2;

    VigCorrFlatAddFunctor(const InvResp & fr, const Adjust & adj)
        : m_InvResp(fr), m_Adjust(adj)
        { }

    InvResp m_InvResp;
    Adjust m_Adjust;
    
    /** calculate transform. 
     */
    result_type operator()(first_argument_type const & v1, second_argument_type const & v2) const
    {
        // apply inverse response/gamma correction, vignetting correction by
        // division and possible brightness adjust
        return m_Adjust(m_InvResp(v1) + v2);
    }
};

template <class VT1, class InvResp, class VigFunc, class Adjust>
class VigCorrAddFunctor
{
public:
    /** the functor's first argument type
     */
    typedef VT1 first_argument_type;

    /** the functor's result type
     */
    typedef typename vigra::NumericTraits<VT1>::RealPromote result_type;

    typedef result_type RealVT1;

    VigCorrAddFunctor(const InvResp & fr, const VigFunc & vf, const Adjust & adj)
        : m_InvResp(fr), m_VigFunc(vf), m_Adjust(adj)
        { }

    InvResp m_InvResp;
    VigFunc m_VigFunc;
    Adjust m_Adjust;
    
    /** calculate transform. 
     */
    result_type operator()(first_argument_type const & v1, float x, float y) const
    {
        // apply inverse response/gamma correction, vignetting correction by
        // division and possible brightness adjust
        return m_Adjust(m_InvResp(v1) + m_VigFunc(x,y));
    }
};


template <int NTERMS=4>
struct PolySqDistFunctor
{
    double m_coeff[NTERMS];

    PolySqDistFunctor(double coeff[])
    { 
        for (unsigned int i=0; i<NTERMS; i++) m_coeff[i] = coeff[i];
    };

    double operator()(double x, double y) const
    {
        double ret = m_coeff[0];
        double r2 = x*x + y*y;
        double r = r2;
        for (unsigned int i = 1; i < NTERMS; i++) {
            ret += m_coeff[i] * r;
            r *= r2;
        }
        return ret;
    }
};


/** Traits to define the maximum value for all types.
 *  The case of float and double differs from vigra::NumericTraits::max() */
#define VIG_CORR_TRAITS(T1,S) \
template<> \
struct VigCorrTraits<T1> \
{ \
    static double max() \
{ \
	return S; \
} \
}; \
template<> \
struct VigCorrTraits<vigra::RGBValue<T1> > \
{ \
    static double max() \
{ \
	return S; \
} \
};

    template <class T1>
    struct VigCorrTraits;

    VIG_CORR_TRAITS(unsigned char, UCHAR_MAX);
    VIG_CORR_TRAITS(signed char, SCHAR_MAX);
    VIG_CORR_TRAITS(unsigned short, USHRT_MAX);
    VIG_CORR_TRAITS(signed short, SHRT_MAX);
    VIG_CORR_TRAITS(unsigned int, UINT_MAX);
    VIG_CORR_TRAITS(signed int, INT_MAX);
    VIG_CORR_TRAITS(float, 1.0);
    VIG_CORR_TRAITS(double, 1.0);

#undef VIG_CORR_TRAITS

struct GammaFunctor
{
    double gamma;
    double maxval;
    GammaFunctor(double g, double m)
    : gamma(g), maxval(m)
    {}

    template <class T>
    typename vigra::NumericTraits<T>::RealPromote operator()(T p) const
    {
        typedef typename vigra::NumericTraits<T>::RealPromote RT;
        RT pix(p);
        pix /= maxval;
        return vigra_ext::pow(pix, gamma)*maxval;
//        return vigra::NumericTraits<T>::fromRealPromote(vigra_ext::pow(pix, gamma)*maxval);
//        return pow(pix, gamma)*maxval;
    }
};

/** Calculate ret = p * a + b 
 */
template <class PT>
struct LinearTransformFunctor
{
    typedef PT result_type;

    PT m_a;
    PT m_b;
    LinearTransformFunctor(PT a, PT b)
    : m_a(a), m_b(b)
    {  };

    PT operator()(PT p) const
    {
        return p * m_a + m_b;
    }
};

/** does nothing */
template <class T>
struct PassThroughFunctor
{
    typedef T result_type;
    
    T operator()(const T & a) const
    {
        return a;
    }
};

template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor, class Functor>
void
applyRadialVigCorrection(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                         vigra::pair<DestImageIterator, DestAccessor> dest,
                         double cx, double cy,
                         Functor const & f) 
{
    applyRadialVigCorrection(src.first, src.second, src.third, dest.first, dest.second,
                             cx, cy, f);
}

template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor, class Functor>
void
applyRadialVigCorrection(SrcImageIterator src_upperleft,
                         SrcImageIterator src_lowerright, SrcAccessor sa,
                         DestImageIterator dest_upperleft, DestAccessor da,
                         double cx, double cy,
                         Functor const & f) 
{
    vigra::Diff2D destSize = src_lowerright - src_upperleft;

    double sf = 1.0/sqrt(destSize.x/2.0*destSize.x/2.0 + destSize.y/2.0*destSize.y/2.0);
    double ynorm = -cy * sf;

    for(; src_upperleft.y < src_lowerright.y; ++src_upperleft.y, ++dest_upperleft.y)
    {
        typename SrcImageIterator::row_iterator s(src_upperleft.rowIterator());
        typename SrcImageIterator::row_iterator send(s+ destSize.x);
        typename DestImageIterator::row_iterator d(dest_upperleft.rowIterator());
        double xnorm = -cx*sf;
        for(; s != send; ++s, ++d) {
            da.set(f(sa(s), xnorm, ynorm), d);
            xnorm +=sf;
        }
        ynorm += sf;
    } 
}


template <class ImgIter, class ImgAccessor, class FFIter, class FFAccessor, class DestIter, class DestAccessor>
void flatfieldVigCorrection(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                            vigra::pair<FFIter, FFAccessor> ffImg,
                            vigra::pair<DestIter, DestAccessor> destImg,
                            double gamma, double gammaMaxVal, bool division,
                            typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote a,
                            typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote b)
{
    typedef typename ImgAccessor::value_type PT;
    typedef typename vigra::NumericTraits<PT>::RealPromote RPT;
    typedef typename FFAccessor::value_type FFT;
    typedef typename vigra::NumericTraits<FFT>::RealPromote RFFT;
    typedef typename DestAccessor::value_type OPT;

    typedef PassThroughFunctor<RPT> RnF;
    typedef LinearTransformFunctor<RPT> LTF;
    LTF adjust(a,b);

    RFFT mean;
    if (division) {
        // calculate mean of flatfield image
        vigra::FindAverage<FFT> average;   // init functor
        vigra::inspectImage(ffImg.first, ffImg.first + (srcImg.second - srcImg.first), ffImg.second, average);
        mean = average();
    }

    if (gamma == 1.0) {
        RnF Rf;
        if (division) {
            vigra::combineTwoImages(srcImg, ffImg, destImg,
                                    VigCorrFlatDivFunctor<PT, FFT, RnF, LTF>(mean, Rf, adjust));
        } else {
            vigra::combineTwoImages(srcImg, ffImg, destImg,
                                    VigCorrFlatAddFunctor<PT, FFT, RnF, LTF>(Rf, adjust));
        }
    } else {
        GammaFunctor Rf(gamma, gammaMaxVal);
        if (division) {
            vigra::combineTwoImages(srcImg, ffImg, destImg,
                                    VigCorrFlatDivFunctor<PT, FFT, GammaFunctor, LTF>(mean, Rf, adjust));
        } else {
            vigra::combineTwoImages(srcImg, ffImg, destImg,
                                    VigCorrFlatAddFunctor<PT, FFT, GammaFunctor, LTF>(Rf, adjust));
        }
    }
}


template <class ImgIter, class ImgAccessor, class DestIter, class DestAccessor>
void radialVigCorrection(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                         vigra::pair<DestIter, DestAccessor> destImg, double gamma, double gammaMaxVal,
                         double radCoeff[], double cx, double cy, bool division,
                         typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote a,
                         typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote b)
{
    typedef typename ImgAccessor::value_type PT;
    typedef typename vigra::NumericTraits<PT>::RealPromote RPT;
    typedef typename ImgAccessor::value_type OutPixelType;

    typedef PolySqDistFunctor<4> PolyF;
    typedef PassThroughFunctor<RPT> RnF;
    typedef LinearTransformFunctor<RPT> LTF;

    PolyF poly(radCoeff);
    LTF adjust(a,b);

    // adjust functor

    if (gamma == 1.0) {
        RnF Rf;
        if (division) {
            applyRadialVigCorrection(srcImg.first, srcImg.second, srcImg.third ,destImg.first, destImg.second, cx, cy,
                                     VigCorrDivFunctor<PT, RnF, PolyF, LTF>(Rf, poly, adjust) );
        } else {
            applyRadialVigCorrection(srcImg.first, srcImg.second, srcImg.third ,destImg.first, destImg.second, cx, cy, 
                                     VigCorrAddFunctor<PT, RnF, PolyF, LTF>(Rf, poly, adjust) );
        }
    } else {
        GammaFunctor Rf(gamma, gammaMaxVal);
        if (division) {
            applyRadialVigCorrection(srcImg.first, srcImg.second, srcImg.third ,destImg.first, destImg.second, cx, cy, 
                                     VigCorrDivFunctor<PT, GammaFunctor, PolyF, LTF>(Rf, poly, adjust) );
        } else {
            applyRadialVigCorrection(srcImg.first, srcImg.second, srcImg.third ,destImg.first, destImg.second, cx, cy, 
                                     VigCorrAddFunctor<PT, GammaFunctor, PolyF, LTF>(Rf, poly, adjust));
        }
    }
}

#if 0
// Hmm, the functor espressions in vigra do not work for me...
namespace detail
{
struct plus
{
template <class T1, class T2>
T1 operator()(const T1 & l, T2 const & r) const
{
//    T1 ret(l);
//    ret+=r;
    typedef typename vigra::NumericTraits<T1> traits;
    typedef typename traits::RealPromote PT;
    return traits::fromRealPromote(PT(l) + PT(r));
}
};

template <class T>
struct divcorr
{
    T mean;
    divcorr(T m) : mean(m) {};

    template <class T1, class T2>
    T1 operator()(const T1 &l, const T2 &r) const
    {
        return vigra::detail::RequiresExplicitCast<T1>::cast(l / ( r / mean));
    }
};
}
#endif

/*
template <class ImgIter, class ImgAccessor, class FFIter, class FFAccessor, class DestIter, class DestAccessor>
void flatfieldVigCorrection(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                            vigra::pair<FFIter, FFAccessor> ffImg,
                            vigra::pair<DestIter, DestAccessor> destImg,
                            bool division)
{
    typedef typename ImgAccessor::value_type PixelType;
    typedef typename FFAccessor::value_type FFType;
    typedef typename vigra::NumericTraits<FFType>::RealPromote RealFFType;
    // take scale factor into accout..
    if (division) {
        // calculate mean of flatfield image
        vigra::FindAverage<FFType> average;   // init functor
        vigra::inspectImage(ffImg.first, ffImg.first + (srcImg.second - srcImg.first), ffImg.second, average);
        RealFFType mean = average();
        vigra::combineTwoImages(srcImg, ffImg, destImg, detail::divcorr<RealFFType>(mean) );
//        vigra::combineTwoImages(srcImg, ffImg, destImg,
//             vigra::functor::Arg1() / ( vigra::functor::Arg2() - vigra::functor::Param(mean) ) );
    } else {
//        vigra::combineTwoImages(srcImg, ffImg, destImg,
//             vigra::functor::Arg1() + vigra::functor::Arg2() );
        vigra::combineTwoImages(srcImg, ffImg, destImg,
            detail::plus() );

    }
}
*/

template <class ImgIter, class ImgAccessor, class DestIter, class DestAccessor>
void applyBrightnessCorrection(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                                   vigra::pair<DestIter, DestAccessor> destImg,
                                   typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote a,
                                   typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote b)
{
    typedef typename ImgAccessor::value_type PT;
    typedef typename vigra::NumericTraits<PT>::RealPromote RPT;
    
    vigra::transformImage(srcImg, destImg, LinearTransformFunctor<RPT>(a,b));
}


template <class ImgIter, class ImgAccessor, class DestIter, class DestAccessor>
void applyGammaAndBrightCorrection(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                                   vigra::pair<DestIter, DestAccessor> destImg,
                                   double gamma, double maxGVal,
                                   typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote a,
                                   typename vigra::NumericTraits<typename ImgAccessor::value_type>::RealPromote b)
{
    typedef typename ImgAccessor::value_type PT;
    typedef typename vigra::NumericTraits<PT>::RealPromote RPT;
    typedef LinearTransformFunctor<RPT> LTF;

    LTF ltf(a,b);
    GammaFunctor gf(gamma, maxGVal);
    vigra::transformImage(srcImg, destImg, NestFunctor<LTF, GammaFunctor>(ltf, gf));
}

template <class ImgIter, class ImgAccessor, class DestIter, class DestAccessor>
void applyGammaCorrection(vigra::triple<ImgIter, ImgIter, ImgAccessor> srcImg,
                          vigra::pair<DestIter, DestAccessor> destImg,
                          double gamma, double maxGVal)
{
    GammaFunctor gf(gamma, maxGVal);
    vigra::transformImage(srcImg, destImg, gf );
}


} // namespace

#endif // VIGRA_EXT_VIGNETTING_CORRECTION_H

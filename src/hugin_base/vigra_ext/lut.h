// -*- c-basic-offset: 4 -*-
/** @file lut.h
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

#ifndef _VIGRA_EXT_LUT_H
#define _VIGRA_EXT_LUT_H

#include <vector>
#include <functional>

#include <hugin_math/hugin_math.h>

#include <vigra/stdimage.hxx>
#include <vigra/numerictraits.hxx>
//#include <vigra/transformimage.hxx>
//#include <vigra/combineimages.hxx>
//#include <vigra/functorexpression.hxx>
#include <vigra/array_vector.hxx>

#include <vigra_ext/utils.h>
#include <vigra_ext/HDRUtils.h>
#include <vigra_ext/emor.h>
//#include <boost/random/mersenne_twister.hpp>


//#define DEBUG_WRITE_FILES

namespace vigra_ext{

template <class VECTOR>
inline void createGammaLUT(double gamma, VECTOR & lut)
{
    typedef typename VECTOR::value_type VT;
    VT s = vigra_ext::LUTTraits<VT>::max();

    // lookup tables
    for (size_t i=0; i<lut.size(); ++i) {
        double x = i*1.0/(lut.size() -1);
        lut[i] = vigra::NumericTraits<VT>::fromRealPromote(pow(x, gamma)*s);
    }
}


template <class VEC, class VEC2>
void resizeLUT(const VEC & iLUT, VEC2 & oLUT)
{
    assert(iLUT.size());
    assert(oLUT.size());

    for(size_t oIdx = 0; oIdx < oLUT.size(); oIdx++) {
        double ix = oIdx/(oLUT.size()-1.0) * (iLUT.size()-1);
        unsigned iIdx = unsigned(ix);
        double deltaix = ix-iIdx;
        if (deltaix == 0.0) {
            // no interpolation required.
            oLUT[oIdx] = iLUT[iIdx];
        } else if (iIdx+1 <= iLUT.size()){
            // linear interpolation
            oLUT[oIdx] = (1-deltaix) * iLUT[iIdx] + deltaix * iLUT[iIdx+1];
        } else {
            oLUT[oIdx] = iLUT.back();
        }
    }
}

/** enforce monotonicity of an array (mostly used for lookup tables) */
template <class LUT>
void enforceMonotonicity(LUT & lut)
{
    typedef typename LUT::value_type lut_type;
	int lutsize = lut.size();

	if (lutsize) {
		lut_type max = lut.back();
		for (int j=0; j < lutsize-1; j++)
		{
			if (lut[j+1] > max) {
				lut[j+1] = max;
			} else if (lut[j+1] < lut[j]) {
				lut[j+1] = lut[j];
			}
		}
	}
}

/** functor to apply a LUT to gray and color images.
 *  This is a safe, and iterpolatating table lookup.
 *
 *  if the argument has a higher bit count than lut.size(),
 *  interpolation will take place.
 *
 *  This is slower, because there is a size check at runtime,
 *  which might be placed in a template parameter.
 *
 *  floating point values between 0..1 are mapped to
 *  min and max of the lookup table.
 */
template <class VTIn, class LUT>
struct LUTFunctor
{
    typedef typename vigra_ext::ValueTypeTraits<VTIn>::value_type VT1;

    /** the functor's first argument type
     */
    //typedef VT1 first_argument_type;

    typedef typename LUT::value_type lut_type;

    /** the functor's result type
     */
    //typedef typename vigra::NumericTraits<VT1>::RealPromote result_type;

    LUTFunctor()
    {

    }

    /** create a LUT functor. */
    LUTFunctor(LUT & lut)
    : m_lut(lut)
    {
        /*
        if (sizeof(VT1) ==1)
            assert(m_lut.size() >= 256);
        else
            assert(m_lut.size() >= 1<<10);
        */
    }

    lut_type applyLutInteger(VT1 v) const
    {
        // lut type.
        assert(m_lut.size() > 0);
        if (m_lut.size() == LUTTraits<VT1>::max()) {
            return m_lut[v];
        } else {
            // calculate new index
            double m = LUTTraits<VT1>::max();
            double x=v/m*(m_lut.size()-1);
            unsigned i = unsigned(x);
            x = x-i;
            if ( x != 0 && i+1 < m_lut.size()) {
                // linear interpolation
                return ((1-x)*m_lut[i]+x*m_lut[i+1]);
            } else {
                return m_lut[i];
            }
        }
    }

    lut_type applyLutFloat(double v) const
    {
        assert(m_lut.size() > 0);
        if (v > 1) return m_lut.back();
        if (v < 0) return 0;
        double x=v*(m_lut.size()-1);
        unsigned i = unsigned(x);
        // interpolate
        x = x-i;
        if ( i+1 < m_lut.size()) {
            // linear interpolation
            return vigra::NumericTraits<lut_type>::fromRealPromote((1-x)*m_lut[i]+x*m_lut[i+1]);
        } else {
            return m_lut[i];
        }
    }

    // lookup on floating point values, interpolate if nessecary
    vigra::RGBValue<lut_type> applyVector( vigra::RGBValue<VT1> v, vigra::VigraFalseType) const
    {
        vigra::RGBValue<VT1> ret;
        for (size_t i=0; i < v.size(); i++) {
            ret[i] = applyLutFloat((v[i]));
        }
        return ret;
    }

    // lookup vector types (the lut needs to be long enought!)
    vigra::RGBValue<lut_type>  applyVector(vigra::RGBValue<VT1> v, vigra::VigraTrueType) const
    {
        assert(m_lut.size() > 0);
        vigra::RGBValue<lut_type> ret;
        for (size_t i=0; i < v.size(); i++) {
            ret[i] = applyLutInteger(v[i]);
        }
        return ret;
    }

    // lookup floating point types
    lut_type applyScalar(VT1 v, vigra::VigraFalseType) const
    {
        return applyLutFloat(v);
    }

    // lookup scalar types
    lut_type applyScalar(VT1 v, vigra::VigraTrueType) const
    {
        return applyLutInteger(v);
    }

    lut_type apply(VT1 v, vigra::VigraTrueType) const
    {
        typedef typename vigra::NumericTraits<VT1>::isIntegral isIntegral;
        return applyScalar(v, isIntegral());
    }

    vigra::RGBValue<lut_type> apply(vigra::RGBValue<VT1> v, vigra::VigraFalseType) const
    {
        typedef typename vigra::NumericTraits<VT1>::isIntegral isIntegral;
        return applyVector(v, isIntegral());
    }

    template <class T>
    typename vigra::NumericTraits<T>::RealPromote operator()(T v) const
    {
        typedef typename vigra::NumericTraits<T>::isScalar is_scalar;
        return apply(v, is_scalar());
    }

    LUT m_lut;
};

/** functor to apply a LUT to gray and color images.
 *  This functor works by using a binary search and
 *  does linear interpolation.
 * 
 *  floating point values between 0..1 are mapped to
 *  min and max of the lookup table.
 *
 *  integers are also mapped to 0..1 before applying the lut.
 */
template <class VT1, class LUT>
struct InvLUTFunctor
{
    /** the functor's first argument type
     */
    //typedef VT1 first_argument_type;

    typedef typename LUT::value_type lut_type;

    /** the functor's result type
     */
    //typedef typename vigra::NumericTraits<VT1>::RealPromote result_type;

    InvLUTFunctor()
    {
    }

    /** create a LUT functor. */
    InvLUTFunctor(LUT & lut)
    : m_lut(lut)
    {
    }

    // assume float is scaled 0..1
    lut_type applyLutFloat(lut_type v) const
    {
        assert(m_lut.size() > 0);
        if (v >= m_lut.back()) return m_lut.back();
        if (v < m_lut[0]) return 0;

        // find the lower bound, p will point to the first *p >= v
        typename LUT::const_iterator p = lower_bound(m_lut.begin(), m_lut.end(), v);


        int x = p-m_lut.begin();
#ifdef DEBUG
// just for usage in the debugger
        //const lut_type *plut = &(*(m_lut.begin()));
#endif
        if (v == 1) {
            return 1;
        } else if (x == 0) {
            return 0;
        } else if (v == *p) {
            return x/(m_lut.size()-1.0);
        } else {
            // interpolate position.
            // p points to the first element > v
            double lower = *(p-1);
            double upper = *(p);
            lut_type delta =  (v - lower) / (upper - lower);
            return (x-1 + delta) / (m_lut.size()-1.0);
        }

    }

    template <class T>
    lut_type applyLutInteger(T i) const
    {
        return applyLutFloat(i / lut_type(vigra::NumericTraits<T>::max()));
    }

    // lookup on floating point values. convert to 16 bit
    // and use lookup table there.
    template <class T>
    vigra::RGBValue<lut_type> applyVector( vigra::RGBValue<T> v, vigra::VigraFalseType) const
    {
        vigra::RGBValue<VT1> ret;
        for (size_t i=0; i < v.size(); i++) {
            ret[i] = applyLutFloat((v[i]));
        }
        return ret;
    }

    template <class T>
    vigra::RGBValue<lut_type>  applyVector(vigra::RGBValue<T> v, vigra::VigraTrueType) const
    {
        vigra::RGBValue<lut_type> ret;
        for (size_t i=0; i < v.size(); i++) {
            ret[i] = applyLutInteger(v[i]);
        }
        return ret;
    }

    // lookup integers,
    template <class T>
    lut_type applyScalar(T v, vigra::VigraFalseType) const
    {
        return applyLutFloat(v);
    }

    // lookup scalar types (the lut needs to be long enought!)
    template <class T>
    lut_type applyScalar(T v, vigra::VigraTrueType) const
    {
        return applyLutInteger(v);
    }

    template <class T>
    lut_type apply(T v, vigra::VigraTrueType) const
    {
        typedef typename vigra::NumericTraits<T>::isIntegral isIntegral;
        return applyScalar(v, isIntegral());
    }

    template <class T>
    vigra::RGBValue<lut_type> apply(vigra::RGBValue<T> v, vigra::VigraFalseType) const
    {
        typedef typename vigra::NumericTraits<T>::isIntegral isIntegral;
        return applyVector(v, isIntegral());
    }

    template <class T>
    typename vigra::NumericTraits<T>::RealPromote operator()(T v) const
    {
        typedef typename vigra::NumericTraits<T>::isScalar is_scalar;
        return apply(v, is_scalar());
    }

    LUT m_lut;
};

/** just apply exposure and response to linear data
 */
template <class OP>
struct ExposureResponseFunctor
{
    ExposureResponseFunctor(double exposure, OP & operation)
    : op(operation), e(exposure)
    {
        e=exposure;
        op = operation;
    }
    OP op;
    double e;

    template <class VT>
    typename vigra::NumericTraits<VT>::RealPromote
    operator()(VT v)
    {
        return op(v*e);
    }
};

} // namespace

#endif // _H

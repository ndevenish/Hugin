// -*- c-basic-offset: 4 -*-
/** @file HDRUtils.h
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

#ifndef _VIGRA_EXT_HDRUTILS_H
#define _VIGRA_EXT_HDRUTILS_H

#include "ROIImage.h"
#include "utils.h"
#include "lut.h"


namespace vigra_ext {

    
template<class VALUETYPE=vigra::RGBValue<float> >
class ReduceToHDRFunctor
{
public:
    typedef VALUETYPE 	argument_type;
    typedef VALUETYPE 	first_argument_type;
    typedef VALUETYPE 	second_argument_type;
    typedef VALUETYPE 	result_type;
    
    typedef typename vigra::NumericTraits<VALUETYPE> Traits;
    typedef typename Traits::RealPromote real_type;

    ReduceToHDRFunctor()
    {
        reset();
    }

    void reset ()
    {
        result = vigra::NumericTraits<real_type>::zero();
        weight = 0;

        maxComp = DBL_MIN;
        minComp = DBL_MAX;
        maxW= 0;
        minW= 1;
    }

    // add a new measurement for the current pixel
    template<class T, class M> 
    void operator() (T const &v, M const &m)
    {
        // normalize to 0..1
        double nm = m / (double)vigra_ext::LUTTraits<M>::max();

        // use a gaussian weight function.
//        double w = (nm - 0.5)/0.18;
//        w = exp( -w*w);
        // a simple triangular function should also work ok
        double w = 0.5-fabs(nm-0.5);

        result += w*v;
        weight += w;

        // store minimum and maximum weight and pixel values
        if (nm > maxW) {
            maxW = w;
        }
        if ( w < minW) {
            minW = w;
        }

        double cmax = getMaxComponent(v);

        if (cmax > maxComp)
        {
            maxComp = cmax;
            maxValue = v;
        }
        if (cmax < minComp)
        {
            minComp = cmax;
            minValue = v;
        }
    }

    /** return the result */ 
    real_type operator() () const
    {
        double eps = 1e-7;
        // heuristics to deal with over and underexposed images.
        if (minW > (1.0-eps) && maxW > (1.0-eps)) {
            // all pixels overexposed, just use smallest value
            return minValue;
        } else if (minW < eps && maxW < eps) {
            // all pixels underexposed. use brightest value
            return maxValue;
        }
        if (weight > 0)
            return result/weight;
        else
            return result;
    }
    
protected:
    real_type result;
    double weight;

    real_type maxValue;
    double maxComp;
    real_type minValue;
    double minComp;
    double maxW;
    double minW;
};

#if 0
/** This is a sigmoid function. It is monotonous, and needs to be
 *  transformed before used for weighting the HDR image. However,
 *  it contains information about wether a point is saturated or not
 *  This is very important for merging the images with the reduce
 *  to HDR functor
 */
template <class In>
double calcSigmoidHDRWeight(In val)
{
    // normalize to 0..1
    double x = getMaxComponent(val)/((double) vigra_ext::LUTTraits<In>::max());

    // sigomid function between -6 ... 6
    x = (x-0.5)*12;

    // calculate sigmoidal stuff
    double y = 1.0 / ( 1.0 + exp(-x));

    double min = 1.0 / ( 1.0 + exp(+6.0));
    double max = 1.0 / ( 1.0 + exp(-6.0));
    return (y - min)/(max-min);
}


/** functor to calculate the blending weights for a HDR image, given original gray values 
 *  This is a hat function. Not useful for later blending, unfortunately..
 */

struct HDRWeightFunctor
{
    HDRWeightFunctor()
    {
        m_lut.resize(256);
        std::cerr << "w = [";
        for (int i=0;i<256;i++) {
            // gaussian 
            float x = (i/255.0f - 0.5f)/0.18f;
            vigra::UInt8 w = std::min(utils::ceili(255*exp( -x*x)), 255);

            // hat function
            //vigra::UInt8 w = std::max( utils::ceili( (1-pow((2.0*i/255.0)-1.0, 4.0) )*255), 1);
            m_lut[i] = w;
            std::cerr << (int) w << " ";
        }
    }

    template <class T>
    vigra::UInt8 operator()(vigra::RGBValue<T> rgb) const
    {
        // mean is not so good since it can create overexposed
        // pixels in the image.
        //T x = (rgb[0] + rgb[1] + rgb[2])/T(3);

        T x = std::max(rgb[0], std::max(rgb[1], rgb[2]));
        return operator()(x);
    }

    vigra::UInt8 operator()(float x) const
    {
        x = x*255;
        vigra::UInt8 i = 0;
        if (x> 0 || x <=255)
            i = (vigra::UInt8) x;
        return m_lut[i];
    }


    vigra::UInt8 operator()(vigra::UInt16 x) const
    {
        x <<= 8;
        return m_lut[x];
    }

    vigra::UInt8 operator()(vigra::UInt8 x) const
    {
        return m_lut[x];
    }

    vigra::ArrayVector<vigra::UInt8> m_lut;
};

#endif


} //namespace
#endif //_H

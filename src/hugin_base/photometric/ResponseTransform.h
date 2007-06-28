// -*- c-basic-offset: 4 -*-
/** @file lut.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: lut.h 1969 2007-04-18 22:25:04Z dangelo $
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

#ifndef VIGRA_EXT_LUT_H
#define VIGRA_EXT_LUT_H

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
#include <boost/random/mersenne_twister.hpp>

#include <panodata/SrcPanoImage.h>

//#define DEBUG_WRITE_FILES

namespace vigra_ext{
/** radiometric transformation, includes exposure,
 *  vignetting and white balance.
 *
 *  scene referred irradiance -> camera color values
 */
template <class VTIn>
struct ResponseTransform
{
    typedef typename vigra_ext::ValueTypeTraits<VTIn>::value_type VT1;

    typedef std::vector<double> LUT;

    ResponseTransform()
    {
        m_radiusScale=0;
        m_flatfield = 0;
    }

    ResponseTransform(const HuginBase::SrcPanoImage & src)
    {
        init(src);
    }

    void init(const HuginBase::SrcPanoImage & src)
    {
//        DEBUG_DEBUG(" " << src.getFilename() << ": resp type:" <<   src.getResponseType() << " expo: " << src.getExposure());
        m_flatfield = 0;
        m_src = src;
        m_radiusScale = 1.0/sqrt(m_src.getSize().x/2.0*m_src.getSize().x/2.0 + m_src.getSize().y/2.0*m_src.getSize().y/2.0);
        m_srcExposure = m_src.getExposure();

        // build response function lookup table, if required
        if (m_src.getResponseType() != HuginBase::SrcPanoImage::RESPONSE_LINEAR) {
            // scale lut to right byte size..
            double lutLenD = LUTTraits<VT1>::max();
            // maximum lut size: 10 bits. Should be enought for most purposes.
            // and fit into the L1 cache.
            size_t lutLen=0;
            if (lutLenD == 1.0 || (lutLenD > ((1<<10)-1))) {
                lutLen = (1<<10);
            } else {
                lutLen = size_t(lutLenD);
            }
            switch (m_src.getResponseType()) 
            {
                case HuginBase::SrcPanoImage::RESPONSE_EMOR:
                    {
                    if (lutLen == 1<<10) {
                        vigra_ext::EMoR::createEMoRLUT(m_src.getEMoRParams(), m_lutR);
                    } else {
                        // resize lut, if size doesn't fit
                        LUT tmp;
                        vigra_ext::EMoR::createEMoRLUT(m_src.getEMoRParams(), tmp);
                        m_lutR.resize(lutLen);
                        resizeLUT(tmp, m_lutR);
                    }
                    }
                    break;
                case HuginBase::SrcPanoImage::RESPONSE_GAMMA:
                    m_lutR.resize(lutLen);
                    vigra_ext::createGammaLUT(m_src.getGamma(), m_lutR);
                    break;
                default:
                    // response curve is stored in src image
                    vigra_fail("ResponseTransform: unknown response function type");
                    break;
            }
            m_lutRFunc = vigra_ext::LUTFunctor<VT1, LUT>(m_lutR);
        }
    }

    void setFlatfield(const vigra::FImage * flat)
    {
        m_flatfield = flat;
    }

    double calcVigFactor(FDiff2D d) const
    {
        if (m_src.getVigCorrMode() & HuginBase::SrcPanoImage::VIGCORR_RADIAL) {
            d = d - m_src.getRadialVigCorrCenter();
            // scale according to 
            d *= m_radiusScale;
            double vig = m_src.getRadialVigCorrCoeff()[0];
            double r2 = d.x*d.x + d.y*d.y;
            double r = r2;
            for (unsigned int i = 1; i < 4; i++) {
                vig += m_src.getRadialVigCorrCoeff()[i] * r;
                r *= r2;
            }
            return vig;
        } else if (m_src.getVigCorrMode() & HuginBase::SrcPanoImage::VIGCORR_FLATFIELD) {
            // TODO: implement flatfield
            if (m_flatfield) {
                int x = std::min(std::max(hugin_utils::roundi(d.x),0), m_flatfield->width()-1);;
                int y = std::min(std::max(hugin_utils::roundi(d.y),0), m_flatfield->height()-1);;
                return (*m_flatfield)(x,y);
            } else {
                return 1;
            }
        } else {
            return 1;
        }
    }

    /** function for gray values (ignores white balance :-) */
    typename vigra::NumericTraits<VT1>::RealPromote
    apply(VT1 v, const FDiff2D & pos, vigra::VigraTrueType) const
    {
        typename vigra::NumericTraits<VT1>::RealPromote ret = v;
        // first, apply vignetting

        ret = ret*calcVigFactor(pos)*m_srcExposure;
        if (m_lutR.size()) {
            return m_lutRFunc(ret);
        } else {
            return ret;
        }
    }

    /** function for color values */
    typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote
    apply(vigra::RGBValue<VT1> v, const FDiff2D & pos, vigra::VigraFalseType) const
    {
        typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote ret = v;
        // first, apply vignetting
        double common = calcVigFactor(pos)*m_srcExposure;
        ret = ret*common;
        // apply white balance factors
        ret.red() = ret.red() * m_src.getWhiteBalanceRed();
        ret.blue() = ret.blue() * m_src.getWhiteBalanceBlue();
        // apply response curve
        if (m_lutR.size()) {
            return m_lutRFunc(ret);
        } else {
            return ret;
        }
    }

    template <class T>
    typename vigra::NumericTraits<T>::RealPromote
    operator()(T v, const FDiff2D & pos) const
    {
        typedef typename vigra::NumericTraits<T>::isScalar is_scalar;
        return apply(v, pos, is_scalar());
    }

    double m_radiusScale;
    LUT m_lutR;
    vigra_ext::LUTFunctor<VT1, LUT> m_lutRFunc;
    const vigra::FImage * m_flatfield;
    double m_srcExposure;

    HuginBase::SrcPanoImage m_src;
};

/** radiometric transformation, includes exposure,
 *  vignetting and white balance 
 *
 *  camera color values -> scene referred irradiance
 */
template <class VTIn, class VTOut>
struct InvResponseTransform : public ResponseTransform<VTIn>
{
    typedef ResponseTransform<VTIn> Base;

    typedef typename vigra_ext::ValueTypeTraits<VTIn>::value_type VT1;
    typedef typename vigra::NumericTraits<VT1>::RealPromote VTInCompReal;
    typedef typename vigra_ext::ValueTypeTraits<VTOut>::value_type dest_type;

    typedef std::vector<double> LUT;
    typedef std::vector<dest_type> LUTD;


    boost::mt19937 Twister;

    InvResponseTransform()
    {
        m_destExposure = 1.0;
        m_hdrMode = false;
        m_intScale = 1;
    }

    InvResponseTransform(const HuginBase::SrcPanoImage & src)
    : Base(src), m_hdrMode(false)
    {
        m_destExposure = 1.0;
        m_intScale = 1;
        if (Base::m_lutR.size()) {
            // todo: invert lut, instead of using this functor?
            m_lutRInvFunc = vigra_ext::InvLUTFunctor<VT1, LUT>(Base::m_lutR);
        }
    }

    void init(const HuginBase::SrcPanoImage & src)
    {
        m_destExposure = 1.0;
        m_intScale = 1;
        Base::init(src);
        if (Base::m_lutR.size()) {
            // todo: invert lut, instead of using this functor?
            m_lutRInvFunc = vigra_ext::InvLUTFunctor<VT1, LUT>(Base::m_lutR);
        }
    }

    void setHDROutput(bool hdrMode=true)
    {
        m_hdrMode = hdrMode;
        m_intScale = 1;
        m_destExposure = 1.0;
    }

    // output lut
    void setOutput(double destExposure, const LUTD & destLut, double scale)
    {
        m_hdrMode = false;
        m_destLut = destLut;
        if (m_destLut.size() > 0) {
            m_destLutFunc = vigra_ext::LUTFunctor<VTInCompReal, LUTD>(m_destLut);
        }
        m_destExposure = destExposure;
        m_intScale = scale;
    }

    // Dithering is used to fool the eye into seeing gradients that are finer
    // than the precision of the pixel type.
    // This prevents the occurence of cleanly-bordered regions in the output where
    // the pixel values suddenly change from N to N+1.
    // Such regions are especially objectionable in the green channel of 8-bit images.
    double dither(const double &v) const
    {
        boost::mt19937 &mt = const_cast<boost::mt19937 &>(Twister);
        double vFraction = v - floor(v);
        // Only dither values within a certain range of the rounding cutoff point.
        if (vFraction > 0.25 && vFraction <= 0.75) {
            // Generate a random number between 0 and 0.5.
            double random = 0.5 * (double)mt() / UINT_MAX;
            if ((vFraction - 0.25) >= random) {
                return ceil(v);
            } else {
                return floor(v);
            }
        } else {
            return v;
        }
    }

    /** function for gray values (ignores white balance :-) */
    typename vigra::NumericTraits<dest_type>::RealPromote
    apply(VT1 v, const FDiff2D & pos, vigra::VigraTrueType) const
    {
        // inverse response
        typename vigra::NumericTraits<VT1>::RealPromote ret;
        if (Base::m_lutR.size()) {
            ret = m_lutRInvFunc(v);
        } else {
            ret = v;
        }
        // inverse vignetting and exposure
        ret *= m_destExposure / (Base::calcVigFactor(pos) * Base::m_srcExposure);
        // apply output transform if required
        if (m_destLut.size() > 0) {
            ret = m_destLutFunc(ret);
        }
        // dither all integer images
        if ( m_intScale > 1) {
            return dither(ret * m_intScale);
        }
        return ret;
    }

    /** function for color values */
    typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote
    apply(vigra::RGBValue<VT1> v, const FDiff2D & pos, vigra::VigraFalseType) const
    {
        typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote ret;
        if (Base::m_lutR.size()) {
            ret = m_lutRInvFunc(v);
        } else {
            ret = v;
        }

        // inverse vignetting and exposure
        ret *= m_destExposure/(Base::calcVigFactor(pos)*Base::m_srcExposure);
        ret.red() /= Base::m_src.getWhiteBalanceRed();
        ret.blue() /= Base::m_src.getWhiteBalanceBlue();
        // apply output transform if required
        if (m_destLut.size() > 0) {
            ret = m_destLutFunc(ret);
        }
        // dither 8 bit images.
        if (m_intScale > 1) {
            for (size_t i=0; i < 3; i++) {
                ret[i] = dither(ret[i] * m_intScale);
            }
        }
        return ret;
    }

    template <class T>
    typename vigra::NumericTraits<T>::RealPromote 
    operator()(T v, const FDiff2D & pos) const
    {
        typedef typename vigra::NumericTraits<T>::isScalar is_scalar;
        return apply(v, pos, is_scalar());
    }
    
    template <class T, class A>
    A
    hdrWeight(T v, A a) const
    {
        if (m_hdrMode && a > 0) {
            return vigra::NumericTraits<A>::fromRealPromote(getMaxComponent(v)/(double)vigra_ext::LUTTraits<T>::max()*vigra_ext::LUTTraits<A>::max());
        } else {
            return a;
        }
    }

    LUT m_lutRInv;
    InvLUTFunctor<VT1, LUT> m_lutRInvFunc;
    LUTD m_destLut;
    vigra_ext::LUTFunctor<VTInCompReal, LUTD> m_destLutFunc;
    double m_destExposure;
    bool m_hdrMode;
    double m_intScale;
};

// usecases for hdr:
// 1. map to new exposure (complete done here)
// 2. map to extended exposure (dito)
// 3. map to linear HDR output

} // namespace

#endif // VIGRA_EXT_VIGNETTING_CORRECTION_H

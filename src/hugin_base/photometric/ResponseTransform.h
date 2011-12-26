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

#ifndef _PHOTOMETRIC_VIGNETTING_CORRECTION_H
#define _PHOTOMETRIC_VIGNETTING_CORRECTION_H

#include <vector>
#include <functional>
#include <boost/version.hpp>
#if BOOST_VERSION>104700
#include <boost/random/taus88.hpp>
#define RANDOMGENERATOR boost::random::taus88
#else
#include <boost/random/mersenne_twister.hpp>
#define RANDOMGENERATOR boost::mt19937
#endif

#include <vigra/stdimage.hxx>
#include <vigra/numerictraits.hxx>
#include <vigra/array_vector.hxx>

#include <hugin_math/hugin_math.h>
#include <vigra_ext/lut.h>
#include <vigra_ext/utils.h>
#include <panodata/SrcPanoImage.h>


namespace HuginBase { namespace Photometric {
    

/** radiometric transformation, includes exposure,
 *  vignetting and white balance.
 *
 *  scene referred irradiance -> camera color values
 */
template <class VTIn>
class ResponseTransform
{
        
    public:
        ///
        typedef typename vigra_ext::ValueTypeTraits<VTIn>::value_type VT1;
        
        ///
        typedef std::vector<double> LUT;

        
    public:
        ///
        ResponseTransform();
        
        ///
        ResponseTransform(const HuginBase::SrcPanoImage & src);
        
        ///
        virtual ~ResponseTransform() {};
        
    private:
        ///
        void initWithSrcImg(const HuginBase::SrcPanoImage & src);
        
        
    public:
        ///
        void setFlatfield(const vigra::FImage * flat)
        { m_flatfield = flat; }

        ///
        double calcVigFactor(hugin_utils::FDiff2D d) const;

		void enforceMonotonicity()
		{
			vigra_ext::enforceMonotonicity(m_lutR);
		}

        /** function for gray values (ignores white balance :-) */
        typename vigra::NumericTraits<VT1>::RealPromote
            apply(VT1 v, const hugin_utils::FDiff2D & pos, vigra::VigraTrueType) const;
        
        /** function for color values */
        typename vigra::NumericTraits<VT1>::RealPromote
            apply(VT1 v, const hugin_utils::FDiff2D & pos) const;

        /** function for color values */
        typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote
            apply(vigra::RGBValue<VT1> v, const hugin_utils::FDiff2D & pos, vigra::VigraFalseType) const;
        
        /** function for color values */
        typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote
            apply(vigra::RGBValue<VT1> v, const hugin_utils::FDiff2D & pos) const;
        
        
        /// deprecated
        template <class T>
        typename vigra::NumericTraits<T>::RealPromote
            operator()(T v, const hugin_utils::FDiff2D & pos) const { return apply(v, pos); }

        
        
    public:
            
        LUT m_lutR;
        double m_radiusScale;
        vigra_ext::LUTFunctor<VT1, LUT> m_lutRFunc;
        const vigra::FImage * m_flatfield;
        double m_srcExposure;
        std::vector<double> m_RadialVigCorrCoeff;
        hugin_utils::FDiff2D m_RadialVigCorrCenter;
        int m_VigCorrMode;
        double m_WhiteBalanceRed;
        double m_WhiteBalanceBlue;

        HuginBase::SrcPanoImage m_src;
};


/** radiometric transformation, includes exposure,
 *  vignetting and white balance 
 *
 *  camera color values -> scene referred irradiance
 */
template <class VTIn, class VTOut>
class InvResponseTransform : public ResponseTransform<VTIn>
{

        typedef ResponseTransform<VTIn> Base;
     
    public:
        typedef typename vigra_ext::ValueTypeTraits<VTIn>::value_type VT1;
        typedef typename vigra::NumericTraits<VT1>::RealPromote VTInCompReal;
        typedef typename vigra_ext::ValueTypeTraits<VTOut>::value_type dest_type;

        typedef std::vector<double> LUT;
        typedef std::vector<dest_type> LUTD;


    public:
        ///
        InvResponseTransform();

        ///
        InvResponseTransform(const HuginBase::SrcPanoImage & src);
        
        ///
        virtual ~InvResponseTransform() {};
        
    private:
        ///
        void init(const HuginBase::SrcPanoImage & src);

        
    public:
        ///
        void setHDROutput(bool hdrMode, double destExposure);
        
        /// output lut
        void setOutput(double destExposure, const LUTD & destLut, double scale);
        
		void enforceMonotonicity()
		{
		    if (Base::m_lutR.size()) {
				vigra_ext::enforceMonotonicity(Base::m_lutR);
		        // todo: invert lut, instead of using this functor?
		        m_lutRInvFunc = vigra_ext::InvLUTFunctor<VT1, LUT>(Base::m_lutR);
			}
		}

        /** Dithering is used to fool the eye into seeing gradients that are finer
         * than the precision of the pixel type.
         * This prevents the occurence of cleanly-bordered regions in the output where
         * the pixel values suddenly change from N to N+1.
         * Such regions are especially objectionable in the green channel of 8-bit images.
         */
        double dither(const double &v) const;
        
        /** function for gray values (ignores white balance :-) */
        typename vigra::NumericTraits<dest_type>::RealPromote
            apply(VT1 v, const hugin_utils::FDiff2D & pos, vigra::VigraTrueType) const;
        
        /**  */
        typename vigra::NumericTraits<dest_type>::RealPromote
            apply(VT1 v, const hugin_utils::FDiff2D & pos) const;
        
        /** function for color values */
        typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote
            apply(vigra::RGBValue<VT1> v, const hugin_utils::FDiff2D & pos, vigra::VigraFalseType) const;
        
        /**  */
        typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote
            apply(vigra::RGBValue<VT1> v, const hugin_utils::FDiff2D & pos) const;
        
        
        /// deprecated
        template <class T>
        typename vigra::NumericTraits<T>::RealPromote 
            operator()(T v, const hugin_utils::FDiff2D & pos) const
        {
            return apply(v, pos);
        }
        
        ///
        template <class T, class A>
        A hdrWeight(T v, A a) const
        {
            if (m_hdrMode && a > 0) {
                return vigra::NumericTraits<A>::fromRealPromote(vigra_ext::getMaxComponent(v)/(double)vigra_ext::LUTTraits<T>::max()*vigra_ext::LUTTraits<A>::max());
            } else {
                return a;
            }
        }
        
        void emitGLSL(std::ostringstream& oss, std::vector<double>& invLut, std::vector<double>& destLut) const;

    protected: // needs be public?
        //LUT m_lutRInv;
        vigra_ext::InvLUTFunctor<VT1, LUT> m_lutRInvFunc;
        LUTD m_destLut;
        vigra_ext::LUTFunctor<VTInCompReal, LUTD> m_destLutFunc;
        double m_destExposure;
        bool m_hdrMode;
        double m_intScale;
        
    private:
        RANDOMGENERATOR Twister;
};


}} // namespace




// templated implementation ----------------------------------------------------


namespace HuginBase { namespace Photometric {


template <class VTIn>
ResponseTransform<VTIn>::ResponseTransform()
{
    m_radiusScale=0;
    m_flatfield = 0;
}

template <class VTIn>
ResponseTransform<VTIn>::ResponseTransform(const HuginBase::SrcPanoImage & src)
{
    initWithSrcImg(src);
}


template <class VTIn>
void ResponseTransform<VTIn>::initWithSrcImg(const HuginBase::SrcPanoImage & src)
{
//        DEBUG_DEBUG(" " << src.getFilename() << ": resp type:" <<   src.getResponseType() << " expo: " << src.getExposure());
    m_flatfield = 0;
    m_src = src;
    m_radiusScale = 1.0/sqrt(m_src.getSize().x/2.0*m_src.getSize().x/2.0 + m_src.getSize().y/2.0*m_src.getSize().y/2.0);
    m_srcExposure = m_src.getExposure();
    //save some variables, direct access is slower since merging of layout mode
    m_RadialVigCorrCoeff = m_src.getRadialVigCorrCoeff();
    m_RadialVigCorrCenter = m_src.getRadialVigCorrCenter();
    m_VigCorrMode = m_src.getVigCorrMode();
    m_WhiteBalanceRed = m_src.getWhiteBalanceRed();
    m_WhiteBalanceBlue = m_src.getWhiteBalanceBlue();

    // build response function lookup table, if required
    if (m_src.getResponseType() != HuginBase::SrcPanoImage::RESPONSE_LINEAR) {
        // scale lut to right byte size..
        double lutLenD = vigra_ext::LUTTraits<VT1>::max();
        // maximum lut size: 10 bits. Should be enought for most purposes.
        // and fit into the L1 cache.
        size_t lutLen=0;
        if (lutLenD == 1.0 || (lutLenD > ((1<<10)-1))) {
            lutLen = (1<<10);
        } else {
            lutLen = size_t(lutLenD) + 1;
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
                    vigra_ext::resizeLUT(tmp, m_lutR);
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


template <class VTIn>
double ResponseTransform<VTIn>::calcVigFactor(hugin_utils::FDiff2D d) const
{
    if (m_VigCorrMode & HuginBase::SrcPanoImage::VIGCORR_RADIAL) {
        d = d - m_RadialVigCorrCenter;
        // scale according to 
        d *= m_radiusScale;
        double vig = m_RadialVigCorrCoeff[0];
        double r2 = d.x*d.x + d.y*d.y;
        double r = r2;
        for (unsigned int i = 1; i < 4; i++) {
            vig += m_RadialVigCorrCoeff[i] * r;
            r *= r2;
        }
        return vig;
    } else if (m_VigCorrMode & HuginBase::SrcPanoImage::VIGCORR_FLATFIELD) {
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


template <class VTIn>
typename vigra::NumericTraits<typename ResponseTransform<VTIn>::VT1>::RealPromote
ResponseTransform<VTIn>::apply(typename ResponseTransform<VTIn>::VT1 v, const hugin_utils::FDiff2D & pos, vigra::VigraTrueType) const
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

template <class VTIn>
typename vigra::NumericTraits<vigra::RGBValue<typename ResponseTransform<VTIn>::VT1> >::RealPromote
ResponseTransform<VTIn>::apply(vigra::RGBValue<typename ResponseTransform<VTIn>::VT1> v, const hugin_utils::FDiff2D & pos, vigra::VigraFalseType) const
{
    typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote ret = v;
    // first, apply vignetting
    double common = calcVigFactor(pos)*m_srcExposure;
    ret = ret*common;
    // apply white balance factors
    ret.red() = ret.red() * m_WhiteBalanceRed;
    ret.blue() = ret.blue() * m_WhiteBalanceBlue;
    // apply response curve
    if (m_lutR.size()) {
        return m_lutRFunc(ret);
    } else {
        return ret;
    }
}

template <class VTIn>
typename vigra::NumericTraits<typename ResponseTransform<VTIn>::VT1>::RealPromote
ResponseTransform<VTIn>::apply(typename ResponseTransform<VTIn>::VT1 v, const hugin_utils::FDiff2D & pos) const
{
    typedef typename vigra::NumericTraits<VT1>::isScalar is_scalar;
    return apply(v, pos, is_scalar());
}

template <class VTIn>
typename vigra::NumericTraits<vigra::RGBValue<typename ResponseTransform<VTIn>::VT1> >::RealPromote
ResponseTransform<VTIn>::apply(vigra::RGBValue<typename ResponseTransform<VTIn>::VT1> v, const hugin_utils::FDiff2D & pos) const
{
    typedef typename vigra::NumericTraits<vigra::RGBValue<VT1> >::isScalar is_scalar;
    return apply(v, pos, is_scalar());
}

template <class VTIn, class VTOut>
InvResponseTransform<VTIn,VTOut>::InvResponseTransform()
{
    m_destExposure = 1.0;
    m_hdrMode = false;
    m_intScale = 1;
}

template <class VTIn, class VTOut>
InvResponseTransform<VTIn,VTOut>::InvResponseTransform(const HuginBase::SrcPanoImage & src)
: Base(src), m_hdrMode(false)
{
    m_destExposure = 1.0;
    m_intScale = 1;
    if (Base::m_lutR.size()) {
        // todo: invert lut, instead of using this functor?
        m_lutRInvFunc = vigra_ext::InvLUTFunctor<VT1, LUT>(Base::m_lutR);
    }
}

template <class VTIn, class VTOut>
void InvResponseTransform<VTIn,VTOut>::init(const HuginBase::SrcPanoImage & src)
{
    m_destExposure = 1.0;
    m_intScale = 1;
    Base::init(src);
    if (Base::m_lutR.size()) {
        // todo: invert lut, instead of using this functor?
        m_lutRInvFunc = vigra_ext::InvLUTFunctor<VT1, LUT>(Base::m_lutR);
    }
}

template <class VTIn, class VTOut>
void InvResponseTransform<VTIn,VTOut>::setHDROutput(bool hdrMode, double destExposure)
{
    m_hdrMode = hdrMode;
    m_intScale = 1;
    m_destExposure = destExposure;
    m_destLut.clear();
}

template <class VTIn, class VTOut>
void InvResponseTransform<VTIn,VTOut>::setOutput(double destExposure, const LUTD & destLut, double scale)
{
    m_hdrMode = false;
    m_destLut = destLut;
    if (m_destLut.size() > 0) {
        m_destLutFunc = vigra_ext::LUTFunctor<VTInCompReal, LUTD>(m_destLut);
    }
    m_destExposure = destExposure;
    m_intScale = scale;
}


template <class VTIn, class VTOut>
double InvResponseTransform<VTIn,VTOut>::dither(const double &v) const
{
    RANDOMGENERATOR &mt = const_cast<RANDOMGENERATOR &>(Twister);
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


template <class VTIn, class VTOut>
typename vigra::NumericTraits<typename InvResponseTransform<VTIn,VTOut>::dest_type>::RealPromote
InvResponseTransform<VTIn,VTOut>::apply(VT1 v, const hugin_utils::FDiff2D & pos, vigra::VigraTrueType) const
{
    // inverse response
    typename vigra::NumericTraits<VT1>::RealPromote ret(v);
    if (Base::m_lutR.size()) {
        ret = m_lutRInvFunc(v);
    } else {
        ret /= vigra_ext::LUTTraits<VT1>::max();
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


template <class VTIn, class VTOut>
typename vigra::NumericTraits<vigra::RGBValue<typename InvResponseTransform<VTIn,VTOut>::VT1> >::RealPromote
InvResponseTransform<VTIn,VTOut>::apply(vigra::RGBValue<VT1> v, const hugin_utils::FDiff2D & pos, vigra::VigraFalseType) const
{
    typename vigra::NumericTraits<vigra::RGBValue<VT1> >::RealPromote ret(v);
    if (Base::m_lutR.size()) {
        ret = m_lutRInvFunc(v);
    } else {
        ret /= vigra_ext::LUTTraits<VT1>::max();
    }

    // inverse vignetting and exposure
    ret *= m_destExposure/(Base::calcVigFactor(pos)*Base::m_srcExposure);
    ret.red() /= Base::m_WhiteBalanceRed;
    ret.blue() /= Base::m_WhiteBalanceBlue;
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


template <class VTIn, class VTOut>
typename vigra::NumericTraits<typename InvResponseTransform<VTIn,VTOut>::dest_type>::RealPromote
InvResponseTransform<VTIn,VTOut>::apply(VT1 v, const hugin_utils::FDiff2D & pos) const
{
    typedef typename vigra::NumericTraits<VT1>::isScalar is_scalar;
    return apply(v, pos, is_scalar());
}

template <class VTIn, class VTOut>
typename vigra::NumericTraits<vigra::RGBValue<typename InvResponseTransform<VTIn,VTOut>::VT1> >::RealPromote
InvResponseTransform<VTIn,VTOut>::apply(vigra::RGBValue<VT1> v, const hugin_utils::FDiff2D & pos) const
{
    typedef typename vigra::NumericTraits<vigra::RGBValue<VT1> >::isScalar is_scalar;
    return apply(v, pos, is_scalar());
}

template <class VTIn, class VTOut>
void
InvResponseTransform<VTIn,VTOut>::emitGLSL(std::ostringstream& oss, std::vector<double>& invLut, std::vector<double>& destLut) const
{
    invLut.clear();
    invLut.reserve(Base::m_lutR.size());

    for (int i = 0; i < Base::m_lutR.size(); i++) {
        double f = static_cast<double>(i) / (Base::m_lutR.size() - 1);
        double v = m_lutRInvFunc(f);
        invLut.push_back(v);
    }
        
    destLut.clear();
    destLut.reserve(m_destLut.size());

    for (typename LUTD::const_iterator lutI = m_destLut.begin(); lutI != m_destLut.end(); ++lutI) {
        typename LUTD::value_type entry = *lutI;
        destLut.push_back(entry);
    }

    double invLutSize = Base::m_lutR.size();
    double pixelMax = vigra_ext::LUTTraits<VT1>::max();
    double destLutSize = m_destLut.size();

    oss << "    // invLutSize = " << invLutSize << endl
        << "    // pixelMax = " << pixelMax << endl
        << "    // destLutSize = " << destLutSize << endl
        << "    // destExposure = " << m_destExposure << endl
        << "    // srcExposure = " << Base::m_srcExposure << endl
        << "    // whiteBalanceRed = " << Base::m_src.getWhiteBalanceRed() << endl
        << "    // whiteBalanceBlue = " << Base::m_src.getWhiteBalanceBlue() << endl;

    if (Base::m_lutR.size() > 0) {
        oss << "    p.rgb = p.rgb * " << (invLutSize - 1.0) << ";" << endl
            << "    vec2 invR = texture2DRect(InvLutTexture, vec2(p.r, 0.0)).sq;" << endl
            << "    vec2 invG = texture2DRect(InvLutTexture, vec2(p.g, 0.0)).sq;" << endl
            << "    vec2 invB = texture2DRect(InvLutTexture, vec2(p.b, 0.0)).sq;" << endl
            << "    vec3 invX = vec3(invR.x, invG.x, invB.x);" << endl
            << "    vec3 invY = vec3(invR.y, invG.y, invB.y);" << endl
            << "    vec3 invA = fract(p.rgb);" << endl
            << "    p.rgb = mix(invX, invY, invA);" << endl;
    }

    if (Base::m_src.getVigCorrMode() & HuginBase::SrcPanoImage::VIGCORR_RADIAL) {
        oss << "    // VigCorrMode=VIGCORR_RADIAL" << endl
            << "    float vig = 1.0;" << endl
            << "    {" << endl
            << "        vec2 vigCorrCenter = vec2(" << Base::m_src.getRadialVigCorrCenter().x << ", "
            << Base::m_src.getRadialVigCorrCenter().y << ");" << endl
            << "        float radiusScale=" << Base::m_radiusScale << ";" << endl
            << "        float radialVigCorrCoeff0 = " << Base::m_src.getRadialVigCorrCoeff()[0] << ";" << endl
            << "        float radialVigCorrCoeff1 = " << Base::m_src.getRadialVigCorrCoeff()[1] << ";" << endl
            << "        float radialVigCorrCoeff2 = " << Base::m_src.getRadialVigCorrCoeff()[2] << ";" << endl
            << "        float radialVigCorrCoeff3 = " << Base::m_src.getRadialVigCorrCoeff()[3] << ";" << endl
            << "        vec2 src = texture2DRect(CoordTexture, gl_TexCoord[0].st).sq;" << endl
            << "        vec2 d = src - vigCorrCenter;" << endl
            << "        d *= radiusScale;" << endl
            << "        vig = radialVigCorrCoeff0;" << endl
            << "        float r2 = dot(d, d);" << endl
            << "        float r = r2;" << endl
            << "        vig += radialVigCorrCoeff1 * r;" << endl
            << "        r *= r2;" << endl
            << "        vig += radialVigCorrCoeff2 * r;" << endl
            << "        r *= r2;" << endl
            << "        vig += radialVigCorrCoeff3 * r;" << endl
            << "    }" << endl;
    } else if (Base::m_src.getVigCorrMode() & HuginBase::SrcPanoImage::VIGCORR_FLATFIELD) {
        oss << "    // VigCorrMode=VIGCORR_FLATFIELD" << endl
            << "    float vig = 1.0;" << endl;
    } else {
        oss << "    // VigCorrMode=none" << endl
            << "    float vig = 1.0;" << endl;
    }

    oss << "    vec3 exposure_whitebalance = vec3("
        << (m_destExposure / (Base::m_srcExposure * Base::m_src.getWhiteBalanceRed())) << ", "
        << (m_destExposure / (Base::m_srcExposure)) << ", "
        << (m_destExposure / (Base::m_srcExposure * Base::m_src.getWhiteBalanceBlue())) << ");" << endl
        << "    p.rgb = (p.rgb * exposure_whitebalance) / vig;" << endl;

    if (m_destLut.size() > 0) {
        oss << "    p.rgb = p.rgb * " << (destLutSize - 1.0) << ";" << endl
            << "    vec2 destR = texture2DRect(DestLutTexture, vec2(p.r, 0.0)).sq;" << endl
            << "    vec2 destG = texture2DRect(DestLutTexture, vec2(p.g, 0.0)).sq;" << endl
            << "    vec2 destB = texture2DRect(DestLutTexture, vec2(p.b, 0.0)).sq;" << endl
            << "    vec3 destX = vec3(destR.x, destG.x, destB.x);" << endl
            << "    vec3 destY = vec3(destR.y, destG.y, destB.y);" << endl
            << "    vec3 destA = fract(p.rgb);" << endl
            << "    p.rgb = mix(destX, destY, destA);" << endl;
    }

    // alpha hdrWeight
    if (m_hdrMode) {
        oss << "    p.a = max(p.r, max(p.g, p.b));" << endl;
    }
}

}} // namespace

#endif // _H

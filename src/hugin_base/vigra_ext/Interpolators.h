// -*- c-basic-offset: 4 -*-
/** @file Interpolators.h
 *
 *  The pano tools interpolators ported to vigra
 *
 *  @author Helmut Dersch <der@fh-furtwangen.de> and
 *          Pablo d'Angelo <pablo.dangelo@web.de> (port to vigra)
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

#ifndef VIGRA_EXT_INTERPOLATORS_H
#define VIGRA_EXT_INTERPOLATORS_H

#include <iostream>
#include <iomanip>

#include <math.h>
#include <hugin_math/hugin_math.h>
#include <algorithm>

#include <vigra/accessor.hxx>
#include <vigra/diff2d.hxx>

using std::endl;

namespace vigra_ext {

// Some locally needed math functions

static double 	sinc		( double x );
static double 	cubic01		( double x );
static double 	cubic12		( double x );

static double sinc( double x )
{
    x *= M_PI;
    if(x != 0.0)
        return(sin(x) / x);
    return(1.0);
}


// Cubic polynomial with parameter A
// A = -1: sharpen; A = - 0.5 homogeneous
// make sure x >= 0
static const double	A(-0.75);

// 0 <= x < 1
static double cubic01( double x )
{
    return	(( A + 2.0 )*x - ( A + 3.0 ))*x*x +1.0;
}
// 1 <= x < 2

static double cubic12( double x )
{
    return	(( A * x - 5.0 * A ) * x + 8.0 * A ) * x - 4.0 * A;

}

/** enum with all interpolation methods */
enum Interpolator {
    INTERP_CUBIC = 0,
    INTERP_SPLINE_16,
    INTERP_SPLINE_36,
    INTERP_SINC_256,
    INTERP_SPLINE_64,
    INTERP_BILINEAR,
    INTERP_NEAREST_NEIGHBOUR,
    INTERP_SINC_1024
};


/** several classes to calculate interpolator weights,
 *
 *  for usage with Interpolating Accessor
 */

/** nearest neighbour, stupid, but might be useful somewhere */
struct interp_nearest
{
    // size of neighbourhood
    static const int size = 2;

    void calc_coeff(double x, double * w) const
        {
            w[1] = (x >= 0.5) ? 1 : 0;
            w[0] = (x < 0.5) ? 1 : 0;
        }

    void emitGLSL(std::ostringstream& oss) const {
        oss << "    return (i == 0.0) ? float(f < 0.5) : float(f >= 0.5);" << endl;
    }
};


/** simple bilinear interpolation */
struct interp_bilin
{
    // size of neighbourhood
    static const int size = 2;

    void calc_coeff(double x, double * w) const
        {
            w[1] = x;
            w[0] = 1.0-x;
        }

    void emitGLSL(std::ostringstream& oss) const {
        oss << "    return abs(i + f - 1.0);" << endl;
    }
};

/** cubic interpolation */
struct interp_cubic
{
    // size of neighbourhood
    static const int size = 4;

    /** initialize weights for given @p x */
    void calc_coeff(double x, double * w) const
        {
            w[3] = cubic12( 2.0 - x );
            w[2] = cubic01( 1.0 - x );
            w[1] = cubic01( x );
            w[0] = cubic12( x + 1.0 );
        }

    void emitGLSL(std::ostringstream& oss) const {
        oss << "    float A = " << A << ";" << endl
            << "    float c = abs(i - 1.0);" << endl
            << "    float m = (i > 1.0) ? -1.0 : 1.0;" << endl
            << "    float p = c + m * f;" << endl
            << "    if (i == 1.0 || i == 2.0) {" << endl
            << "        return (( A + 2.0 )*p - ( A + 3.0 ))*p*p + 1.0;" << endl
            << "    } else {" << endl
            << "        return (( A * p - 5.0 * A ) * p + 8.0 * A ) * p - 4.0 * A;" << endl
            << "    }" << endl;
    }
};

/** spline16 interpolation */
struct interp_spline16
{
    // size of neighbourhood
    static const int size = 4;

    /** initialize weights for given @p x */
    void calc_coeff(double x, double * w) const
        {
            w[3] = ( ( 1.0/3.0  * x - 1.0/5.0 ) * x -   2.0/15.0 ) * x;
            w[2] = ( ( 6.0/5.0 - x     ) * x +   4.0/5.0 ) * x;
            w[1] = ( ( x - 9.0/5.0 ) * x -   1.0/5.0     ) * x + 1.0;
            w[0] = ( ( -1.0/3.0 * x + 4.0/5.0     ) * x -   7.0/15.0 ) * x;
        }

    void emitGLSL(std::ostringstream& oss) const {
        oss << "    return (i > 1.0) ? (i == 3.0) ? (( ( 1.0/3.0  * f - 1.0/5.0 ) * f -   2.0/15.0 ) * f)" << endl
            << "                                  : (( ( 6.0/5.0 - f     ) * f +   4.0/5.0 ) * f)" << endl
            << "                     : (i == 1.0) ? (( ( f - 9.0/5.0 ) * f -   1.0/5.0     ) * f + 1.0)" << endl
            << "                                  : (( ( -1.0/3.0 * f + 4.0/5.0     ) * f -   7.0/15.0 ) * f);" << endl;
    }
};

/** spline36 interpolation */
struct interp_spline36
{
    // size of neighbourhood
    static const int size = 6;

    /** calculate weights for given offset @p x.
     *
     *  @param x position mod 1
     *  @param w destination array. must be at least size elements long.
     *
     */
    void calc_coeff(double x, double* w) const
        {
            w[5] = ( ( -  1.0/11.0  * x +  12.0/ 209.0 ) * x +   7.0/ 209.0  ) * x;
            w[4] = ( (    6.0/11.0  * x -  72.0/ 209.0 ) * x -  42.0/ 209.0  ) * x;
            w[3] = ( ( - 13.0/11.0  * x + 288.0/ 209.0 ) * x + 168.0/ 209.0  ) * x;
            w[2] = ( (   13.0/11.0  * x - 453.0/ 209.0 ) * x -   3.0/ 209.0  ) * x + 1.0;
            w[1] = ( ( -  6.0/11.0  * x + 270.0/ 209.0 ) * x - 156.0/ 209.0  ) * x;
            w[0] = ( (    1.0/11.0  * x -  45.0/ 209.0 ) * x +  26.0/ 209.0  ) * x;
        }

    void emitGLSL(std::ostringstream& oss) const {
        oss << "    return (i > 3.0) ? (i == 5.0) ? (( ( -  1.0/11.0  * f +  12.0/ 209.0 ) * f +   7.0/ 209.0  ) * f)" << endl
            << "                                  : (( (    6.0/11.0  * f -  72.0/ 209.0 ) * f -  42.0/ 209.0  ) * f)" << endl
            << "                     : (i > 1.0) ? (i == 3.0) ? (( ( - 13.0/11.0  * f + 288.0/ 209.0 ) * f + 168.0/ 209.0  ) * f)" << endl
            << "                                              : (( (   13.0/11.0  * f - 453.0/ 209.0 ) * f -   3.0/ 209.0  ) * f + 1.0)" << endl
            << "                                 : (i == 1.0) ? (( ( -  6.0/11.0  * f + 270.0/ 209.0 ) * f - 156.0/ 209.0  ) * f)" << endl
            << "                                              : (( (    1.0/11.0  * f -  45.0/ 209.0 ) * f +  26.0/ 209.0  ) * f);" << endl;
    }
};


/** spline64 interpolation */
struct interp_spline64
{
    // size of neighbourhood
    static const int size = 8;

    /** initialize weights for given offset @p x */
    void calc_coeff(double x, double * w) const
        {
            w[7] = ((  1.0/41.0 * x -   45.0/2911.0) * x -   26.0/2911.0) * x;
            w[6] = ((- 6.0/41.0 * x +  270.0/2911.0) * x +  156.0/2911.0) * x;
            w[5] = (( 24.0/41.0 * x - 1080.0/2911.0) * x -  624.0/2911.0) * x;
            w[4] = ((-49.0/41.0 * x + 4050.0/2911.0) * x + 2340.0/2911.0) * x;
            w[3] = (( 49.0/41.0 * x - 6387.0/2911.0) * x -    3.0/2911.0) * x + 1.0;
            w[2] = ((-24.0/41.0 * x + 4032.0/2911.0) * x - 2328.0/2911.0) * x;
            w[1] = ((  6.0/41.0 * x - 1008.0/2911.0) * x +  582.0/2911.0) * x;
            w[0] = ((- 1.0/41.0 * x +  168.0/2911.0) * x -   97.0/2911.0) * x;
        }

    void emitGLSL(std::ostringstream& oss) const {
        oss << "    return (i > 3.0) ? (i > 5.0) ? (i == 7.0) ? (((  1.0/41.0 * f -   45.0/2911.0) * f -   26.0/2911.0) * f)" << endl
            << "                                              : (((- 6.0/41.0 * f +  270.0/2911.0) * f +  156.0/2911.0) * f)" << endl
            << "                                 : (i == 5.0) ? ((( 24.0/41.0 * f - 1080.0/2911.0) * f -  624.0/2911.0) * f)" << endl
            << "                                              : (((-49.0/41.0 * f + 4050.0/2911.0) * f + 2340.0/2911.0) * f)" << endl
            << "                     : (i > 1.0) ? (i == 3.0) ? ((( 49.0/41.0 * f - 6387.0/2911.0) * f -    3.0/2911.0) * f + 1.0)" << endl
            << "                                              : (((-24.0/41.0 * f + 4032.0/2911.0) * f - 2328.0/2911.0) * f)" << endl
            << "                                 : (i == 1.0) ? (((  6.0/41.0 * f - 1008.0/2911.0) * f +  582.0/2911.0) * f)" << endl
            << "                                              : (((- 1.0/41.0 * f +  168.0/2911.0) * f -   97.0/2911.0) * f);" << endl;
    }
};

/** sinc interpolation, with variable width */
template <int size_>
struct interp_sinc
{
    // size of neighbourhood
    static const int size = size_;

    /** initialize weights for given offset @p x */
    void calc_coeff(double x, double * w) const
        {
            int idx;
            double xadd;
            for( idx = 0, xadd = size / 2 - 1.0 + x;
                 idx < size / 2;
                 xadd-=1.0)
            {
                w[idx++] = sinc( xadd ) * sinc( xadd / ( size / 2 ));
            }
            for( xadd = 1.0 - x;
                 idx < size;
                 xadd+=1.0)
            {
                w[idx++] = sinc( xadd ) * sinc( xadd / ( size / 2 ));
            }
	}

    void emitGLSL(std::ostringstream& oss) const {
        oss << "    float c = (i < " << (size/2.0) << ") ? 1.0 : -1.0;" << endl
            << "    float x = c * (" << (size/2 - 1.0) << " - i + f);" << endl
            << "    vec2 xpi = vec2(x, x / " << (size/2.0) << ") * " << M_PI << ";" << endl
            << "    vec2 xsin = sin(xpi);" << endl
            << "    vec2 result = vec2(1.0, 1.0);" << endl
            << "    if (xpi.x != 0.0) result.x = xsin.x / xpi.x;" << endl
            << "    if (xpi.y != 0.0) result.y = xsin.y / xpi.y;" << endl
            << "    return result.x * result.y;" << endl;
    }
};


/** "wrapper" for efficient interpolation access to an image
 *
 *  Tailored for panorama remapping. Supports warparound boundary condition of left and right
 */
template <typename SrcImageIterator, typename SrcAccessor,
          typename INTERPOLATOR>
class ImageInterpolator
{
public:
    typedef typename SrcAccessor::value_type PixelType;
    // dummy mask type to be compatible to algorithms expecting a ImageMaskInterpolator object
    typedef typename vigra::UInt8 MaskType;
private:
    typedef typename vigra::NumericTraits<PixelType>::RealPromote RealPixelType;

    SrcImageIterator m_sIter;
    SrcAccessor m_sAcc;
    int m_w;
    int m_h;
    bool m_warparound;

    INTERPOLATOR m_inter;

public:
    /** Construct interpolator for an given image */
    ImageInterpolator(vigra::triple<SrcImageIterator, SrcImageIterator,SrcAccessor> const & src,
                          INTERPOLATOR & inter,
                          bool warparound)
    : m_sIter(src.first),
      m_sAcc(src.third),
      m_w(src.second.x - src.first.x),
      m_h(src.second.y - src.first.y),
      m_warparound(warparound),
      m_inter(inter)
    {
    }

    /** Construct interpolator for specific image.
     *
     */
    ImageInterpolator(SrcImageIterator src_upperleft,
                          SrcImageIterator src_lowerright,
                          SrcAccessor sa,
                          INTERPOLATOR & inter,
                          bool warparound)
    : m_sIter(src_upperleft),
      m_sAcc(sa),
      m_w(src_lowerright.x - src_upperleft.x),
      m_h(src_lowerright.y - src_upperleft.y),
      m_warparound(warparound),
      m_inter(inter)
    {
    }

    /** Interpolate without mask, but return dummy alpha value nevertheless */
    bool operator()(double x, double y,
                    PixelType & result, MaskType & mask) const
    {
        mask = 255;
        return operator()(x,y, result);
    }

    /** Interpolate without mask */
    bool operator()(double x, double y,
                    PixelType & result) const
    {

        // skip all further interpolation if we cannot interpolate anything
        if (x < -INTERPOLATOR::size/2 || x > m_w + INTERPOLATOR::size/2) return false;
        if (y < -INTERPOLATOR::size/2 || y > m_h + INTERPOLATOR::size/2) return false;

        double t = floor(x);
        double dx = x - t;
        int srcx = int(t);
        t = floor(y);
        double dy = y - t;
        int srcy = int(t);

        if ( srcx > INTERPOLATOR::size/2 && srcx < m_w -INTERPOLATOR::size/2 &&
             srcy > INTERPOLATOR::size/2 && srcy < m_h - INTERPOLATOR::size/2)
        {
            return interpolateNoMaskInside(srcx, srcy, dx, dy, result);
        }

        double wx[INTERPOLATOR::size];
        double wy[INTERPOLATOR::size];

        // calculate x interpolation coefficients
        m_inter.calc_coeff(dx, wx);
        m_inter.calc_coeff(dy, wy);

        RealPixelType p(vigra::NumericTraits<RealPixelType>::zero());
        double weightsum = 0.0;
        for (int ky = 0; ky < INTERPOLATOR::size; ky++) {
            int bounded_ky = srcy + 1 + ky - INTERPOLATOR::size/2;

            // Boundary condition: do not replicate top and bottom
            if (bounded_ky < 0 || bounded_ky >= m_h) {
                continue;
            }

            for (int kx = 0; kx < INTERPOLATOR::size; kx++) {
                int bounded_kx = srcx + 1 + kx - INTERPOLATOR::size/2;

                if (m_warparound) {
                    // Boundary condition: wrap around the image.
                    if (bounded_kx < 0) 
                        bounded_kx += m_w;
                    if (bounded_kx >= m_w) 
                        bounded_kx -= m_w;
                } else {
                    // Boundary condition: replicate first and last column.
                    //                if (srcx + kx < 0) bounded_kx -= (srcx + kx);
                    //                if (srcx + kx >= src_w) bounded_kx -= (srcx + kx - (src_w - 1));
                    // Boundary condition: do not replicate left and right
                    if (bounded_kx < 0) 
                        continue;
                    if (bounded_kx >= m_w)
                        continue;
                }

                // check mask
                double f = wx[kx]*wy[ky];
                p += f * m_sAcc(m_sIter, vigra::Diff2D(bounded_kx, bounded_ky));
                weightsum += f;
            }
        }

        // force a certain weight
        if (weightsum <= 0.2) return false;
        // Adjust filter for any ignored transparent pixels.
        if (weightsum != 1.0) p /= weightsum;

        result = vigra::detail::RequiresExplicitCast<PixelType>::cast(p);
        return true;
    }


    /** Interpolate without boundary check and mask */
    bool interpolateNoMaskInside(int srcx, int srcy, double dx, double dy,
                                    PixelType & result) const
    {
        double w[INTERPOLATOR::size];
        RealPixelType resX[INTERPOLATOR::size];

        // calculate x interpolation coefficients
        m_inter.calc_coeff(dx, w);

        RealPixelType p;

        // first pass of separable filter, x pass
        vigra::Diff2D offset(srcx - INTERPOLATOR::size/2 + 1,
                             srcy - INTERPOLATOR::size/2 + 1);
        SrcImageIterator ys(m_sIter + offset);
        for (int ky = 0; ky < INTERPOLATOR::size; ky++, ++(ys.y)) {
            p = vigra::NumericTraits<RealPixelType>::zero();
            typename SrcImageIterator::row_iterator xs(ys.rowIterator());
            //SrcImageIterator xs(ys);
            for (int kx = 0; kx < INTERPOLATOR::size; kx++, ++xs) {
                p += w[kx] * m_sAcc(xs);
            }
            resX[ky] = p;
        }

        // y pass.
        m_inter.calc_coeff(dy, w);
        p = vigra::NumericTraits<RealPixelType>::zero();
        for (int ky = 0; ky < INTERPOLATOR::size; ky++) {
            p += w[ky] * resX[ky];
        }

        result = vigra::detail::RequiresExplicitCast<PixelType>::cast(p);
        return true;
    }

    void emitGLSL(std::ostringstream& oss) const {
        m_inter.emitGLSL(oss);
    }

};


/** "wrapper" for efficient interpolation access to an image
 *
 *  Tailored for panorama remapping. Supports warparound boundary condition of left and right
 *  as well as masks
 */
template <typename SrcImageIterator, typename SrcAccessor,
          typename MaskIterator, typename MaskAccessor,
          typename INTERPOLATOR>
class ImageMaskInterpolator
{
public:
    typedef typename SrcAccessor::value_type PixelType;
    typedef typename MaskAccessor::value_type MaskType;
private:
    typedef typename vigra::NumericTraits<PixelType>::RealPromote RealPixelType;

    SrcImageIterator m_sIter;
    SrcAccessor m_sAcc;
    MaskIterator m_mIter;
    MaskAccessor m_mAcc;
    int m_w;
    int m_h;
    bool m_warparound;

    INTERPOLATOR m_inter;

public:

    /** Construct interpolator for an given image */
    ImageMaskInterpolator(vigra::triple<SrcImageIterator, SrcImageIterator,SrcAccessor> const & src,
                          std::pair<MaskIterator, MaskAccessor> mask,
                          INTERPOLATOR & inter,
                          bool warparound)
    : m_sIter(src.first),
      m_sAcc(src.third),
      m_mIter(mask.first),
      m_mAcc(mask.second),
      m_w(src.second.x - src.first.x),
      m_h(src.second.y - src.first.y),
      m_warparound(warparound),
      m_inter(inter)
    {
    }

    /** Construct interpolator for specific image.
     *
     */
    ImageMaskInterpolator(SrcImageIterator src_upperleft,
                          SrcImageIterator src_lowerright,
                          SrcAccessor sa,
                          MaskIterator mask_upperleft,
                          MaskAccessor ma,
                          INTERPOLATOR & inter,
                          bool warparound)
    : m_sIter(src_upperleft),
      m_sAcc(sa),
      m_mIter(mask_upperleft),
      m_mAcc(ma),
      m_w(src_lowerright.x - src_upperleft.x),
      m_h(src_lowerright.y - src_upperleft.y),
      m_warparound(warparound),
      m_inter(inter)
    {
    }
#if 0
    /** Interpolate the data item at a non-integer position @p x, @p y
     *
     *  It checks if the interpolation would access a pixel with alpha = 0
     *  and returns false in that case.
     *
     *  be careful, no bounds checking is done here. take
     *  INTERPOLATOR::size into accout before iterating over the
     *  picture.
     *
     *  the used image pixels are [i-(n/2 -1) .. i+n/2], where n is
     *  the size of the interpolator
     *
     *  @param x      x position, relative to \p i and \p alpha.first
     *  @param y      y position, relative to \p i and \p alpha.first
     *  @param result the interpolation result
     *
     *  @return true if interpolation ok, false if one or more pixels were masked out
     *
     */
//    bool operator()(float x, float y,
    // this is slower than the full version, thanks to the normalized interpolation (with masks).
    bool interpolateSeperable(float x, float y,
                     PixelType & result) const
    {

        // skip all further interpolation if we cannot interpolate anything
        if (x < -INTERPOLATOR::size/2 || x > m_w + INTERPOLATOR::size/2) return false;
        if (y < -INTERPOLATOR::size/2 || y > m_h + INTERPOLATOR::size/2) return false;

        double t = floor(x);
        double dx = x - t;
        int srcx = int(t);
        t = floor(y);
        double dy = y - t;
        int srcy = int(t);


        double w[INTERPOLATOR::size];

        double weightsX[INTERPOLATOR::size];
        PixelType resX[INTERPOLATOR::size];

        // calculate x interpolation coefficients
        m_inter.calc_coeff(dx, w);

        // first pass of separable filter

        for (int ky = 0; ky < INTERPOLATOR::size; ky++) {
            int bounded_ky = srcy + 1 + ky - INTERPOLATOR::size/2;

            // Boundary condition: replicate top and bottom rows.
            //                 if (srcy + ky < 0) bounded_ky -= (srcy + ky);
            //                 if (srcy + ky >= src_h) bounded_ky -= (srcy + ky - (src_h - 1));

            // Boundary condition: do not replicate top and bottom
            if (bounded_ky < 0 || bounded_ky >= m_h) {
                weightsX[ky] = 0;
                resX[ky] = 0;
                continue;
            }

            RealPixelType p(vigra::NumericTraits<RealPixelType>::zero());
            double weightsum = 0.0;

            for (int kx = 0; kx < INTERPOLATOR::size; kx++) {
                int bounded_kx = srcx + 1 + kx - INTERPOLATOR::size/2;

                if (m_warparound) {
                    // Boundary condition: wrap around the image.
                    if (bounded_kx < 0) 
                        bounded_kx += m_w;
                    if (bounded_kx >= m_w) 
                        bounded_kx -= m_w;
                } else {
                    // Boundary condition: replicate first and last column.
                    //                if (srcx + kx < 0) bounded_kx -= (srcx + kx);
                    //                if (srcx + kx >= src_w) bounded_kx -= (srcx + kx - (src_w - 1));
                    // Boundary condition: do not replicate left and right
                    if (bounded_kx < 0) 
                        continue;
                    if (bounded_kx >= m_w)
                        continue;
                }
                if (m_mIter(bounded_kx, bounded_ky)) {
                    // check mask
                    p += w[kx] * m_sIter(bounded_kx, bounded_ky);
                    weightsum += w[kx];
                }
            }
            weightsX[ky] = weightsum;
            resX[ky] = p;
        }

        // y pass.
        m_inter.calc_coeff(dy, w);
        RealPixelType p(vigra::NumericTraits<RealPixelType>::zero());
        double weightsum = 0.0;
        for (int ky = 0; ky < INTERPOLATOR::size; ky++) {
            weightsum += weightsX[ky] * w[ky];
            p += w[ky] * resX[ky];
        }

        if (weightsum == 0.0) return false;
        // Adjust filter for any ignored transparent pixels.
        if (weightsum != 1.0) p /= weightsum;

        result = vigra::detail::RequiresExplicitCast<PixelType>::cast(p);
        return true;
    }
#endif

    /** Interpolate the data item at a non-integer position @p x, @p y
     *
     *  It checks if the interpolation would access a pixel with alpha = 0
     *  and returns false in that case.
     *
     *  be careful, no bounds checking is done here. take
     *  INTERPOLATOR::size into accout before iterating over the
     *  picture.
     *
     *  the used image pixels are [i-(n/2 -1) .. i+n/2], where n is
     *  the size of the interpolator
     *
     *  @param x      x position, relative to \p i and \p alpha.first
     *  @param y      y position, relative to \p i and \p alpha.first
     *  @param result the interpolation result
     *
     *  @return true if interpolation ok, false if one or more pixels were masked out
     *
     */
    bool operator()(double x, double y,
                    PixelType & result, MaskType & mask) const
    {

        // skip all further interpolation if we cannot interpolate anything
        if (x < -INTERPOLATOR::size/2 || x > m_w + INTERPOLATOR::size/2) return false;
        if (y < -INTERPOLATOR::size/2 || y > m_h + INTERPOLATOR::size/2) return false;

        double t = floor(x);
        double dx = x - t;
        int srcx = int(t);
        t = floor(y);
        double dy = y - t;
        int srcy = int(t);

        if ( srcx > INTERPOLATOR::size/2 && srcx < m_w -INTERPOLATOR::size/2 &&
             srcy > INTERPOLATOR::size/2 && srcy < m_h - INTERPOLATOR::size/2)
        {
            return interpolateInside(srcx, srcy, dx, dy, result, mask);
        }

        double wx[INTERPOLATOR::size];
        double wy[INTERPOLATOR::size];

        // calculate x interpolation coefficients
        m_inter.calc_coeff(dx, wx);
        m_inter.calc_coeff(dy, wy);

        // first pass of separable filter

        RealPixelType p(vigra::NumericTraits<RealPixelType>::zero());
        double m = 0; 
        double weightsum = 0.0;
        for (int ky = 0; ky < INTERPOLATOR::size; ky++) {
            int bounded_ky = srcy + 1 + ky - INTERPOLATOR::size/2;

            // Boundary condition: do not replicate top and bottom
            if (bounded_ky < 0 || bounded_ky >= m_h) {
                continue;
            }

            for (int kx = 0; kx < INTERPOLATOR::size; kx++) {
                int bounded_kx = srcx + 1 + kx - INTERPOLATOR::size/2;

                if (m_warparound) {
                    // Boundary condition: wrap around the image.
                    if (bounded_kx < 0) 
                        bounded_kx += m_w;
                    if (bounded_kx >= m_w) 
                        bounded_kx -= m_w;
                } else {
                    // Boundary condition: replicate first and last column.
                    //                if (srcx + kx < 0) bounded_kx -= (srcx + kx);
                    //                if (srcx + kx >= src_w) bounded_kx -= (srcx + kx - (src_w - 1));
                    // Boundary condition: do not replicate left and right
                    if (bounded_kx < 0) 
                        continue;
                    if (bounded_kx >= m_w)
                        continue;
                }

                MaskType cmask = m_mIter(bounded_kx, bounded_ky);
                if (cmask) {
                    // check mask
                    double f = wx[kx]*wy[ky];
                    // TODO: check if this is good, influences the HDR stitching masks
                    m += f * cmask;
                    p += f * m_sAcc(m_sIter, vigra::Diff2D(bounded_kx, bounded_ky));
                    weightsum += f;
                }
            }
        }

        // force a certain weight
        if (weightsum <= 0.2) return false;
        // Adjust filter for any ignored transparent pixels.
        if (weightsum != 1.0) {
            p /= weightsum;
            m /= weightsum;
        }

        mask = vigra::detail::RequiresExplicitCast<MaskType>::cast(m);
        result = vigra::detail::RequiresExplicitCast<PixelType>::cast(p);
        return true;
    }


    /** Interpolate without boundary check. */
    bool interpolateInside(int srcx, int srcy, double dx, double dy,
                                    PixelType & result, MaskType & mask) const
    {

        double wx[INTERPOLATOR::size];
        double wy[INTERPOLATOR::size];

        // calculate x interpolation coefficients
        m_inter.calc_coeff(dx, wx);
        m_inter.calc_coeff(dy, wy);

        RealPixelType p(vigra::NumericTraits<RealPixelType>::zero());
        double weightsum = 0.0;
        double m = 0.0;
        vigra::Diff2D offset(srcx - INTERPOLATOR::size/2 + 1,
                             srcy - INTERPOLATOR::size/2 + 1);
        SrcImageIterator ys(m_sIter + offset);
        MaskIterator yms(m_mIter + offset);
        for (int ky = 0; ky < INTERPOLATOR::size; ky++, ++(ys.y), ++(yms.y)) {
//            int bounded_ky = srcy + 1 + ky - INTERPOLATOR::size/2;
            typename SrcImageIterator::row_iterator xs(ys.rowIterator());
            typename MaskIterator::row_iterator xms(yms.rowIterator());
            for (int kx = 0; kx < INTERPOLATOR::size; kx++, ++xs, ++xms) {
//                int bounded_kx = srcx + 1 + kx - INTERPOLATOR::size/2;

                MaskType cmask = *xms;
                if (cmask) {
                    // check mask
                    double f = wx[kx]*wy[ky];
                    // TODO: check if this is good, influences the HDR stitching masks
                    m += f * cmask;
                    p += f * m_sAcc(xs);
                    weightsum += f;
                }
            }
        }

        // force a certain weight
        if (weightsum <= 0.2) return false;
        // Adjust filter for any ignored transparent pixels.
        if (weightsum != 1.0) {
            p /= weightsum;
            m /= weightsum;
        }

        result = vigra::detail::RequiresExplicitCast<PixelType>::cast(p);
        mask = vigra::detail::RequiresExplicitCast<MaskType>::cast(m);
        return true;
    }

};

/********************************************************/
/*                                                      */
/*                  InterpolatingAccessor               */
/*                                                      */
/********************************************************/

/** \brief interpolation at non-integer positions.

    This accessor allows an image be accessed at arbitrary non-integer
    coordinates and performs an interpolation to
    obtain a pixel value.
    It uses the given ACCESSOR (which is usually the
    accessor originally associated with the iterator)
    to access data.

    The interpolation algorithm is given as a template parameter,
    INTERPOLATOR. See the interp_bilin interp_cubic, interp_spline16,
    interp_spline36, interp_spline64 and interp_sinc for possible
    interpolators

    Namespace: vigra

    <b> Usage </b>

    \code

    // used variables:
    // iterator  : pointing to upperLeft
    // accessor  : accessor to the source image
    // dest      : value, interpolated at sx, m_sIter (relative to iterator)

    interp_cubic iterp;
    InterpolatingAccessor<SrcAccessor,
	typename SrcAccessor::value_type,Interpolator> interpol(accessor, interp);

    ...
    double sx = 102.1;
    double m_sIter = 58,81;
    dest = interpol(iterator, sx, m_sIter);

    <b> Required Interface:</b>

    \code
    ITERATOR iter;
    ACCESSOR a;
    VALUETYPE destvalue;
    INTERPOLATOR interp;
    const int interp.size;
    float s;
    int x, y;

    void interp::calc_coeff(double x);
    destvalue = s * a(iter, x, y) + s * a(iter, x, y);

    \endcode */
template <class ACCESSOR, class VALUETYPE, class INTERPOLATOR>
class InterpolatingAccessor
{
public:
    /** the iterators' pixel type
     */
    typedef VALUETYPE value_type;

    /** init from given accessor
     */
    InterpolatingAccessor(ACCESSOR a, INTERPOLATOR inter)
        : a_(a), inter_x(inter), inter_y(inter)
    {}

    /** Interpolate the data item at a non-integer position @p x, @p y
     *
     *  be careful, no bounds checking is done here. take
     *  INTERPOLATOR::size into accout before iterating over the
     *  picture.
     *
     *  the used image pixels are [i-(n/2 -1) .. i+n/2], where n is
     *  the size of the interpolator
     *
     *  @param i      base iterator
     *  @param x      x position, relative to \p i and \p alpha.first
     *  @param y      y position, relative to \p i and \p alpha.first
     *
     *  @return interpolation result
     *
     */
    template <class ITERATOR>
    value_type operator()(ITERATOR const & i, float x, float y) const
    {
        int ix = int(x);
        int iy = int(y);
        float dx = x - ix;
        float dy = y - iy;
        double wx[INTERPOLATOR::size];
        double wy[INTERPOLATOR::size];

        // promote value_type for multiplication
        typename vigra::NumericTraits<value_type>::RealPromote
            ret (vigra::NumericTraits<value_type>::zero());

        // calculate interpolation coefficients
        inter_x.calc_coeff(dx, wx);
        inter_y.calc_coeff(dy, wy);

        ITERATOR ys(i + vigra::Diff2D(ix - inter_x.size/2 + 1,
                                      iy - inter_y.size/2 + 1));
        for(int y = 0; y < inter_y.size; ++y, ++ys.y) {
            ITERATOR xs(ys);
            for(int x = 0; x < inter_x.size; x++, ++xs.x) {
                ret += wx[x] * wy[y] * a_(xs);
            }
        }
        return vigra::detail::RequiresExplicitCast<value_type>::cast(ret);
    }

    /** Interpolate the data item at a non-integer position @p x, @p y
     *
     *  It checks if the interpolation would access a pixel with alpha = 0
     *  and returns false in that case.
     *
     *  be careful, no bounds checking is done here. take
     *  INTERPOLATOR::size into accout before iterating over the
     *  picture.
     *
     *  the used image pixels are [i-(n/2 -1) .. i+n/2], where n is
     *  the size of the interpolator
     *
     *  @param i      base iterator
     *  @param alpha  alpha image
     *  @param x      x position, relative to \p i and \p alpha.first
     *  @param y      y position, relative to \p i and \p alpha.first
     *  @param result the interpolation result
     *
     *  @return true if interpolation ok, false if one or more pixels were masked out
     *
     */
    template <class ITERATOR, class ALPHAITERATOR, class ALPHAACCESSOR>
    bool operator()(ITERATOR const & i, std::pair<ALPHAITERATOR, ALPHAACCESSOR> const & alpha,
                    float x, float y, value_type & result) const
    {
        int ix = int(x);
        int iy = int(y);
        float dx = x - ix;
        float dy = y - iy;
        double wx[INTERPOLATOR::size];
        double wy[INTERPOLATOR::size];

        // promote value_type for multiplication
        typename vigra::NumericTraits<value_type>::RealPromote
            ret (vigra::NumericTraits<value_type>::zero());

        // calculate interpolation coefficients
        inter_x.calc_coeff(dx, wx);
        inter_y.calc_coeff(dy, wy);

        ITERATOR ys(i + vigra::Diff2D(ix - inter_x.size/2 + 1,
                                      iy - inter_y.size/2 + 1));
        ALPHAITERATOR ays(alpha.first + vigra::Diff2D(ix - inter_x.size/2 + 1,
                                      iy - inter_y.size/2 + 1));
        for(int y = 0; y < inter_y.size; ++y, ++ys.y, ++ays.y) {
            ITERATOR xs(ys);
            ALPHAITERATOR axs(ays);
            for(int x = 0; x < inter_x.size; x++, ++xs.x, ++axs.x) {
                if (alpha.second(axs) <= 0 ) {
                    return false;
                }
                ret += wx[x] * wy[y] * a_(xs);
            }
        }
        result = vigra::detail::RequiresExplicitCast<value_type>::cast(ret);
        return true;
    }


private:
    ACCESSOR a_;
    INTERPOLATOR inter_x, inter_y;
};

} // namespace

#endif // VIGRA_EXT_INTERPOLATORS_H

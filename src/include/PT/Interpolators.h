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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef PT_INTERPOLATORS_H
#define PT_INTERPOLATORS_H

#include <math.h>

#include "vigra/accessor.hxx"
#include "vigra/diff2d.hxx"

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
            w[1] = int(1-x);
            w[0] = int(x+0.5);
        }
};


/** simple bilinear interpolation */
struct interp_bilin
{
    // size of neighbourhood
    static const int size = 2;

    void calc_coeff(double x, double * w) const
        {
            w[1] = x+0.5;
            w[0] = 1.0-x;
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
        typename vigra::NumericTraits<value_type>::RealPromote ret;
        
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
    
private:
    ACCESSOR a_;
    INTERPOLATOR inter_x, inter_y;
};


#endif // PT_INTERPOLATORS_H

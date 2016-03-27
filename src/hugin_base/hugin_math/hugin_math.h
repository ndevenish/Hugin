// -*- c-basic-offset: 4 -*-
/** @file hugin_math.h
 *
 *  @brief misc math function & classes used by other parts
 *         of the program
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

#ifndef _HUGIN_MATH_HUGIN_MATH_H
#define _HUGIN_MATH_HUGIN_MATH_H

#include <hugin_shared.h>
#include <cmath>
#include <math.h>
#include <iostream>
#include <vigra/diff2d.hxx>

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

#ifndef PI
	#define PI 3.14159265358979323846
#endif

#define DEG_TO_RAD( x )		( (x) * 2.0 * PI / 360.0 )
#define RAD_TO_DEG( x )		( (x) * 360.0 / ( 2.0 * PI ) )

/** namespace for various utils */
namespace hugin_utils
{
    inline double round(double x)
    {
        return floor(x+0.5);
    }

    inline float roundf(float x)
    {
        return (float) floor(x+0.5f);
    }

    inline int ceili(double x)
    {
        return (int) ceil(x);
    }

    inline int floori(double x)
    {
        return (int) floor(x);
    }

    
    
    template <class T>
    inline int roundi(T x)
    {
        return ((x < 0.0) ?
                    ((x < (float)INT_MIN) ? INT_MIN : static_cast<int>(x - 0.5)) :
                    ((x > (float)INT_MAX) ? INT_MAX : static_cast<int>(x + 0.5)));
    }

    // a simple point class
    template <class T>
    struct TDiff2D
    {
        TDiff2D()
            : x(0), y(0)
            { }
        TDiff2D(T x, T y)
            : x(x), y(y)
            { }
        TDiff2D(const vigra::Diff2D &d)
            : x(d.x), y(d.y)
            { }

        bool operator==(TDiff2D rhs) const
            {
                return x == rhs.x &&  y == rhs.y;
            }
        
        bool operator!=(TDiff2D rhs) const
            {
                return x != rhs.x || y != rhs.y;
            }

        TDiff2D operator+(TDiff2D rhs) const
            {
                return TDiff2D (x+rhs.x, y+rhs.y);
            }

        TDiff2D operator-(TDiff2D rhs) const
            {
                return TDiff2D (x-rhs.x, y-rhs.y);
            }

        TDiff2D & operator*=(double val)
            {
                x = x*val;
                y = y*val;
                return *this;
            }
        
        TDiff2D operator*(double val)
            {
                TDiff2D<T> result;
                result.x = x * val;
                result.y = y * val;
                return result;
            }

        vigra::Diff2D toDiff2D() const
            {
                return vigra::Diff2D(roundi(x), roundi(y));
            }
        
        /// Return square of the distance to another point 
        T squareDistance(TDiff2D<T> other) const
            {
                return (other - *this).squareLength();
            }
        
        /// Return the square of the length of the vector
        T squareLength() const
            {
                return x*x + y*y;
            }

        double x,y;
    };
    
    ///
    typedef TDiff2D<double> FDiff2D;
    

    /** clip a point to fit int [min, max]
     *  does not do a mathematical clipping, just sets p.x and p.y
     *  to the borders if they are outside.
     */
    template <class T>
    T simpleClipPoint(const T & point, const T & min, const T & max)
    {
        T p(point);
        if (p.x < min.x) p.x = min.x;
        if (p.x > max.x) p.x = max.x;
        if (p.y < min.y) p.y = min.y;
        if (p.y > max.y) p.y = max.y;
        return p;
    }

    template <class T>
    T sqr(T t)
    {
        return t*t;
    }

    template <class T>
    double norm(T t)
    {
        return sqrt(t.x*t.x + t.y*t.y);
    }

    /** calculate squared Euclidean distance between two vectors.
     */
    template <class InputIterator1, class InputIterator2>
    double euclid_dist(InputIterator1 first1, InputIterator1 last1,
                         InputIterator2 first2)
    {
        typename InputIterator1::value_type res = 0;
        InputIterator1 i(first1);
        while (i != last1) {
            double a = *i;
            double b = *(first2 + (i - first1));
            res = res + a*a + b*b;
            ++i;
        }
        return sqrt(res);
    }

    /** calculate squared Euclidean distance between two vectors.
     */
    template <class InputIterator1, class InputIterator2, class T>
    T sqr_dist(InputIterator1 first1, InputIterator1 last1,
                         InputIterator2 first2, T res)
    {
        InputIterator1 i(first1);
        while (i != last1) {
            T a = (T)(*i) - (T) (*(first2 + (i - first1)));
            res = res + a*a;
            ++i;
        }
        return res;
    }

    /** calculate the bounding box of a circle that goes through
     *  both points. the center of the circle is halfway between
     *  the two points
     */
    template <class POINT>
    vigra::Rect2D calcCircleROIFromPoints(const POINT& p1, const POINT & p2)
    {
        double dx = p2.x - p1.x;
        double dy = p2.y - p1.y;
        double r = sqrt(dx*dx + dy*dy) / 2.0;
        double mx = p1.x + dx/2;
        double my = p1.y + dy/2;

        vigra::Rect2D rect;
        rect.setUpperLeft(vigra::Point2D(roundi(mx-r), roundi(my -r)));
        rect.setLowerRight(vigra::Point2D(roundi(mx+r), roundi(my+r)));
        return rect;
    }
    
    

} // namespace

template <class T>
inline std::ostream & operator<<(std::ostream & o, const hugin_utils::TDiff2D<T> & d)
{
    return o << "( " << d.x << " " << d.y << " )";
}

inline hugin_utils::FDiff2D operator/(const hugin_utils::FDiff2D & lhs, double val)
{
    return hugin_utils::FDiff2D(lhs.x/val, lhs.y/val);
}

// uses ceil for rounding.
inline vigra::Diff2D operator*(const vigra::Diff2D & d, double scale)
{
    return vigra::Diff2D((int)(ceil(d.x * scale)), 
                         (int)(ceil(d.y * scale)));
}

/// uses floor for left and top and ceil for right and bottom -> extend image when rounding..
inline vigra::Rect2D operator*(const vigra::Rect2D & r, double scale)
{
    return vigra::Rect2D( (int)floor(r.left()*scale),
                          (int)floor(r.top()*scale),
                          (int)ceil(r.right()*scale),
                          (int)ceil(r.bottom()*scale));
}

#endif // _H

// -*- c-basic-offset: 4 -*-
/** @file math.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef MY_MATH_H
#define MY_MATH_H

#include <iostream>
#include <vigra/basicimage.hxx>

/** namespace for various math utils */
namespace utils
{

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

    TDiff2D operator+(TDiff2D rhs) const
        {
            return TDiff2D (x+rhs.x, y+rhs.y);
        }

    TDiff2D operator-(TDiff2D rhs) const
        {
            return TDiff2D (x-rhs.x, y-rhs.y);
        }

    vigra::Diff2D toDiff2D() const
        {
			// windows fix
            return vigra::Diff2D((int)floor(x+0.5f), (int)floor(y+0.5f));
        }

    double x,y;
};

/** clip a point to fit int [min, max]
 *  does not do a mathematical clipping, just sets p.x and p.y
 *  to the borders if they are outside.
 */
template <class T>
void simpleClipPoint(T & p, const T & min, const T & max)
{
    if (p.x < min.x) p.x = min.x;
    if (p.x > max.x) p.x = max.x;
    if (p.y < min.y) p.y = min.y;
    if (p.y > max.y) p.y = max.y;
}


/** calculate Euclidean distance between two vectors.
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

/** calculate Euclidean distance between two vectors.
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


} // namespace

typedef utils::TDiff2D<double> FDiff2D;

template <class T>
inline std::ostream & operator<<(std::ostream & o, const utils::TDiff2D<T> & d)
{
    return o << "( " << d.x << " " << d.y << " )";
}


#endif // MY_MATH_H

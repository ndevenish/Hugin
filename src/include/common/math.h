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

#include <vigra/basicimage.hxx>
#include <iostream>

// a simple vector class...
struct FDiff2D
{
    FDiff2D()
        : x(0), y(0)
        { }
    FDiff2D(float x, float y)
        : x(x), y(y)
        { }
    FDiff2D(const vigra::Diff2D &d)
        : x(d.x), y(d.y)
        { }

    FDiff2D operator+(FDiff2D rhs) const
        {
            return FDiff2D (x+rhs.x, y+rhs.y);
        }

    FDiff2D operator-(FDiff2D rhs) const
        {
            return FDiff2D (x-rhs.x, y-rhs.y);
        }

    vigra::Diff2D toDiff2D() const
        {
            return vigra::Diff2D((int)round(x), (int)round(y));
        }

    float x,y;
};

inline std::ostream & operator<<(std::ostream & o, const FDiff2D & d)
{
    return o << "( " << d.x << " " << d.y << " )";
}

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

#endif // MY_MATH_H

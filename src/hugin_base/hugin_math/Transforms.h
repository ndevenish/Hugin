// -*- c-basic-offset: 4 -*-
/** @file hugin_math/Transforms.h
 *
 *  Contains varius transformation functions
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

#ifndef _HUGIN_MATH_TRANSFORMS_H
#define _HUGIN_MATH_TRANSFORMS_H

#include <cmath>
#include <hugin_math/hugin_math.h>


namespace hugin_utils {
namespace TRANSFORM {

struct DegToRad
{
    double operator()(double x)
        { return (x/180.0*M_PI); };
};

struct RadToDeg
{
    double operator()(double x)
        { return (x/M_PI*180.0); };
};


inline double calcRectFocalLength(double hfov, double width)
{
    return width / ( 2*tan(hfov/2));
}

inline void rotatePoint(FDiff2D & dest, FDiff2D src, double angle)
{
    double cos_r = cos(angle);
    double sin_r = sin(angle);
    dest.x = src.x*cos_r - src.y*sin_r;
    dest.y = src.x*sin_r + src.y*cos_r;
}


/** transform for equirectangular to rectangular.
 *
 *  rectangular coordinates use a math coordinate system:
 *  origin at 0,0, y points up and x points right.
 */
struct ERectToRect
{
public:
    /** construct transform for equirectangular to rectangular.
     *
     *  @param focalLength in pixels
     */
    ERectToRect(double focalLength)
        : f(focalLength) { };

    void operator()(FDiff2D & dest, const FDiff2D & src)
        {
            dest.x = f * tan(src.x);
            dest.y = f * tan(src.y);
        }
    double f;
};


/** transform for rectangular to equirectangular.
 */
struct RectToERect
{
public:
    /** construct transform for rectangular to equirectangular.
     *
     *  @param focalLength in pixels (of rectangular image)
     */
    RectToERect(double focalLength)
        : f(focalLength) { };

    void operator()(FDiff2D & dest, const FDiff2D & src)
        {
            dest.x = atan2(src.x,f);
            dest.y = atan2(src.y,f);
        }
    double f;
};

/** convert cartesian coordinates to image coordinates
 */
struct CartToImg
{
    CartToImg(double width, double height)
        {
            tx = width/2.0;
            ty = height/2.0;
        };

    void operator()(FDiff2D & dest, const FDiff2D & src)
        {
            dest.x = src.x + tx;
            dest.y = src.y + ty;
        }
    double tx, ty;
};


/** convert image coordinates to mathematical rectangular coordinates.
 */
struct ImgToCart
{
    ImgToCart(double width, double height)
        {
            tx = width/2.0;
            ty = height/2.0;
        };

    void operator()(FDiff2D & dest, const FDiff2D & src)
        {
            dest.x = src.x - tx;
            dest.y = src.y - ty;
        }
    double tx, ty;
};

/** rotate and move equirectangular orientation.
 */
struct RotERect
{
public:
    RotERect(double yaw,  double pitch, double roll)
        : p(pitch), y(yaw), sin_r(sin(roll)), cos_r(cos(roll))
        { };

    void operator()(FDiff2D & dest, const FDiff2D & rsrc)
        {
            FDiff2D src(rsrc);
            // order of rotations: r, p, y
            dest.x = src.x*cos_r - src.y*sin_r;
            dest.y = src.x*sin_r + src.y*cos_r;
            dest.x += y;
            dest.y += p;
        }
    double p,y;
    double sin_r, cos_r;
};


/** translation: dest = src - [x y]
 */
struct Translate
{
public:
    Translate(double x, double y)
        : x(x), y(y) { }

    void operator()(FDiff2D & dest, const FDiff2D & src)
        {
            dest.x = src.x - x;
            dest.y = src.y - y;
        }
    double x,y;
};

/** inverse rotate and move equirectangular orientation.
 */
struct InvRotERect
{
public:
    InvRotERect(double yaw,  double pitch, double roll)
        : p(pitch), y(yaw), msin_r(sin(-roll)), mcos_r(cos(-roll))
        { };

    void operator()(FDiff2D & dest, const FDiff2D & src)
        {
            // order of rotations: r, p, y
            dest.x -= y;
            dest.y -= p;
            dest.x = src.x*mcos_r - src.y*msin_r;
            dest.y = src.x*msin_r + src.y*mcos_r;
        }
    double p,y;
    double msin_r, mcos_r;
};


} // namespace
} // namespace
#endif // _TRANSFORMS_H
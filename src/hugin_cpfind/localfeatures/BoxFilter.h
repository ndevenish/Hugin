/*
* Copyright (C) 2007-2008 Anael Orlinski
*
* This file is part of Panomatic.
*
* Panomatic is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* Panomatic is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Panomatic; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __lfeat_boxfilter_h
#define __lfeat_boxfilter_h

#include "MathStuff.h"

namespace lfeat
{

class Image;

class BoxFilter
{
public:
    BoxFilter(double iBaseSize, Image& iImage);

    void				setY(unsigned int y);
    double getDxxWithX(unsigned int x) const;
    double getDyyWithX(unsigned int x) const;
    double getDxyWithX(unsigned int x) const;
    double getDetWithX(unsigned int x) const;

    bool checkBounds(int x, int y) const;

    // orig image info
    double**	_ii;
    unsigned int	_im_width;
    unsigned int	_im_height;

    int _basesize;

    // precomp values for det
    double _sqCorrectFactor;


    // the relative values of rectangle position for the first derivatives on x
    int _lxy_d2;

    // new ones
    int _lxx_x_mid;
    int _lxx_x_right;
    int _lxx_y_bottom;

    // stored Y

    unsigned int _y_minus_lxx_y_bottom;
    unsigned int _y_plus_lxx_y_bottom;
    unsigned int _y_minus_lxx_x_right;
    unsigned int _y_plus_lxx_x_right;
    unsigned int _y_minus_lxx_x_mid;
    unsigned int _y_plus_lxx_x_mid;
    unsigned int _y_minus_lxy_d2;
    unsigned int _y_plus_lxy_d2;
    unsigned int _y;

};

inline BoxFilter::BoxFilter(double iBaseSize, Image& iImage)
{
    _ii = iImage.getIntegralImage();
    _im_width = iImage.getWidth();
    _im_height = iImage.getHeight();

    _basesize = Math::Round(iBaseSize); // convert to integer


    // precomputed values for det
    double aCorrectFactor = 9.0 / (iBaseSize * iBaseSize);
    _sqCorrectFactor = aCorrectFactor * aCorrectFactor;

    // the values for lxy are all positive. will negate in the getDxy
    _lxy_d2 = ((int)(iBaseSize * 3) - 1) / 2 - 1;

    // new ones
    _lxx_x_mid = _basesize / 2;
    _lxx_x_right = _lxx_x_mid + _basesize;
    _lxx_y_bottom = _lxx_x_mid * 2;


}

#define CALC_INTEGRAL_SURFACE(II, STARTX, ENDX, STARTY, ENDY) \
    (II[ENDY+1][ENDX+1] + II[STARTY][STARTX] - II[ENDY+1][STARTX] - II[STARTY][ENDX+1])

inline double BoxFilter::getDxxWithX(unsigned int x) const
{
    return	CALC_INTEGRAL_SURFACE(_ii,x - _lxx_x_right,	x + _lxx_x_right, _y_minus_lxx_y_bottom, _y_plus_lxx_y_bottom)
            - 3.0 * CALC_INTEGRAL_SURFACE(_ii,x - _lxx_x_mid,	x + _lxx_x_mid,   _y_minus_lxx_y_bottom, _y_plus_lxx_y_bottom);
}

inline double BoxFilter::getDyyWithX(unsigned int x) const
{
    // calculates the Lyy convolution a point x,y with filter base size, using integral image
    // use the values of Lxx, but rotate them.
    return	CALC_INTEGRAL_SURFACE(_ii,		x - _lxx_y_bottom,		x + _lxx_y_bottom, _y_minus_lxx_x_right, _y_plus_lxx_x_right)
            -  3.0 * CALC_INTEGRAL_SURFACE(_ii,	x - _lxx_y_bottom,		x + _lxx_y_bottom, _y_minus_lxx_x_mid, _y_plus_lxx_x_mid);

}

inline double BoxFilter::getDxyWithX(unsigned int x) const
{
    // calculates the Lxy convolution a point x,y with filter base size, using integral image

    return	CALC_INTEGRAL_SURFACE(_ii,		x,						x + _lxy_d2, _y,			_y_plus_lxy_d2)
            +	CALC_INTEGRAL_SURFACE(_ii,		x - _lxy_d2,			x, _y_minus_lxy_d2,			_y)
            -	CALC_INTEGRAL_SURFACE(_ii,		x,						x + _lxy_d2, _y_minus_lxy_d2,			_y)
            -	CALC_INTEGRAL_SURFACE(_ii,		x - _lxy_d2,			x, _y,			_y_plus_lxy_d2);
}

#undef CALC_INTEGRAL_SURFACE

inline double BoxFilter::getDetWithX(unsigned int x) const
{
    double aDxy = getDxyWithX(x) * 0.9 * 2 / 3.0;
    double aDxx = getDxxWithX(x);
    double aDyy = getDyyWithX(x);

    return  ((aDxx * aDyy) - (aDxy * aDxy)) * _sqCorrectFactor;
}

inline void	BoxFilter::setY(unsigned int y)
{
    _y_minus_lxx_y_bottom = y - _lxx_y_bottom;
    _y_plus_lxx_y_bottom = y + _lxx_y_bottom;
    _y_minus_lxx_x_right = y - _lxx_x_right;
    _y_plus_lxx_x_right = y + _lxx_x_right;
    _y_minus_lxx_x_mid = y - _lxx_x_mid;
    _y_plus_lxx_x_mid = y + _lxx_x_mid;
    _y_minus_lxy_d2 = y - _lxy_d2;
    _y_plus_lxy_d2 = y + _lxy_d2;
    _y = y;

}

inline bool BoxFilter::checkBounds(int x, int y) const
{
    return (	x > _lxx_x_right && x + _lxx_x_right < (int)_im_width - 1
                &&	y > _lxx_x_right && y + _lxx_x_right < (int)_im_height - 1);
}

} // namespace lfeat

#endif //__lfeat_boxfilter_h

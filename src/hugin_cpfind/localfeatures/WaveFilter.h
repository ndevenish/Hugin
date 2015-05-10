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
* <http://www.gnu.org/licenses/>.
*/

#ifndef __lfeat_wavefilter_h
#define __lfeat_wavefilter_h

namespace lfeat
{

class Image;

class WaveFilter
{
public:
    WaveFilter(double iBaseSize, Image& iImage);

    double getWx(unsigned int x, unsigned int y);
    double getWy(unsigned int x, unsigned int y);

    bool checkBounds(int x, int y) const;

    double getSum(unsigned int x, unsigned int y, int scale);
    double getWx(unsigned int x, unsigned int y, int scale);
    double getWy(unsigned int x, unsigned int y, int scale);

    bool checkBounds(int x, int y, int scale) const;

private:

    // orig image info
    double**		_ii;
    unsigned int	_im_width;
    unsigned int	_im_height;

    // internal values
    int _wave_1;

};

inline WaveFilter::WaveFilter(double iBaseSize, Image& iImage)
{
    _ii = iImage.getIntegralImage();
    _im_width = iImage.getWidth();
    _im_height = iImage.getHeight();

    _wave_1 = (int)iBaseSize;
}

#define CALC_INTEGRAL_SURFACE(II, STARTX, ENDX, STARTY, ENDY) \
    (II[ENDY+1][ENDX+1] + II[STARTY][STARTX] - II[ENDY+1][STARTX] - II[STARTY][ENDX+1])

inline double WaveFilter::getWx(unsigned int x, unsigned int y)
{
    return	-	CALC_INTEGRAL_SURFACE(_ii, x - _wave_1,	x,				y - _wave_1,	y + _wave_1	)
            +	CALC_INTEGRAL_SURFACE(_ii,	x,				x + _wave_1,	y - _wave_1,	y + _wave_1	);
}

inline double WaveFilter::getWy(unsigned int x, unsigned int y)
{
    return	+	CALC_INTEGRAL_SURFACE(_ii,x - _wave_1,	x + _wave_1,	y - _wave_1,	y			)
            -	CALC_INTEGRAL_SURFACE(_ii,x - _wave_1,	x + _wave_1,	y,				y + _wave_1	);
}

inline bool WaveFilter::checkBounds(int x, int y) const
{
    return (	x > _wave_1 && x + _wave_1 < (int)_im_width - 1
                &&	y > _wave_1 && y + _wave_1 < (int)_im_height - 1);
}

// versions without precomputed width
inline double WaveFilter::getSum(unsigned int x, unsigned int y, int s)
{
    return	CALC_INTEGRAL_SURFACE(_ii, x - s, x + s, y - s, y + s );
}

inline double WaveFilter::getWx(unsigned int x, unsigned int y, int _wave_1)
{
    return	-	CALC_INTEGRAL_SURFACE(_ii, x - _wave_1,	x,				y - _wave_1,	y + _wave_1	)
            +	CALC_INTEGRAL_SURFACE(_ii,	x,				x + _wave_1,	y - _wave_1,	y + _wave_1	);
}

inline double WaveFilter::getWy(unsigned int x, unsigned int y, int _wave_1)
{
    return	+	CALC_INTEGRAL_SURFACE(_ii,x - _wave_1,	x + _wave_1,	y - _wave_1,	y			)
            -	CALC_INTEGRAL_SURFACE(_ii,x - _wave_1,	x + _wave_1,	y,				y + _wave_1	);
}

inline bool WaveFilter::checkBounds(int x, int y, int _wave_1) const
{
    return (	x > _wave_1 && x + _wave_1 < (int)_im_width - 1
                &&	y > _wave_1 && y + _wave_1 < (int)_im_height - 1);
}

} // namespace lfeat

#endif //__lfeat_wavefilter_h

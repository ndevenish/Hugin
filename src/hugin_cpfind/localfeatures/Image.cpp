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

#include <iostream>
#include <vector>

#include "Image.h"

using namespace lfeat;
using namespace std;

Image::Image(vigra::DImage &img)
{
    init(img);
}

void Image::init(vigra::DImage &img)
{
    // store values
    _width = img.width();
    _height = img.height();

    // allocate the integral image data
    _ii = AllocateImage(_width + 1, _height + 1);

    // create the integral image
    buildIntegralImage(img);
}

void Image::clean()
{
    if (_ii)
    {
        DeallocateImage(_ii, _height + 1);
    }
    else if (_ii)
    {
        delete[] _ii;
    }
    _ii = 0;
}

Image::~Image()
{
    clean();
}

void Image::buildIntegralImage(vigra::DImage &img)
{
    // to make easier the later computation, shift the image by 1 pix (x and y)
    // so the image has a size of +1 for width and height compared to orig image.

    // fill first line with zero
    for(unsigned int i = 0; i <= _width; ++i)
    {
        _ii[0][i] = 0;
    }

    // fill first row with zero
    for(unsigned int i = 0; i <= _height; ++i)
    {
        _ii[i][0] = 0;
    }

    // compute all the others pixels
    for(unsigned int i = 1; i <= _height; ++i)
        for(unsigned int j = 1; j <= _width; ++j)
        {
            _ii[i][j] = img[i-1][j-1] + _ii[i-1][j] + _ii[i][j-1] - _ii[i-1][j-1];
        }

}

// allocate and deallocate pixels
double** Image::AllocateImage(unsigned int iWidth, unsigned int iHeight)
{
    // create the lines holder
    double** aImagePtr = new double* [iHeight];

    // create the lines
    for(unsigned int i = 0; i < iHeight; ++i)
    {
        aImagePtr[i] = new double[iWidth];
    }

    return aImagePtr;
}

void Image::DeallocateImage(double** iImagePtr, unsigned int iHeight)
{
    // delete the lines
    for(unsigned int i = 0; i < iHeight; ++i)
    {
        delete[] iImagePtr[i];
    }

    // delete the lines holder
    delete[] iImagePtr;
}

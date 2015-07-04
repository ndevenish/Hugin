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

#ifndef __lfeat_image_h
#define __lfeat_image_h

#include "KeyPoint.h"
#include "vigra/stdimage.hxx"

namespace lfeat
{

// forward declaration
class IntegralImage;

class LFIMPEX Image
{
public:
    Image() : _width(0), _height(0), _ii(0) {};

    // Constructor from a pixel array (C style)
    explicit Image(vigra::DImage &img);
    // setup the integral image
    void init(vigra::DImage &img);

    // cleanup
    void clean();

    // Destructor
    ~Image();

    // Accessors
    inline double** getIntegralImage()
    {
        return _ii;
    }
    inline unsigned int getWidth()
    {
        return _width;
    }
    inline unsigned int getHeight()
    {
        return _height;
    }

    // allocate and deallocate integral image pixels
    static double** AllocateImage(unsigned int iWidth, unsigned int iHeight);
    static void DeallocateImage(double** iImagePtr, unsigned int iHeight);

private:

    // prepare the integral image
    void buildIntegralImage(vigra::DImage &img);

    // image size
    unsigned int _width;
    unsigned int _height;

    // integral image
    double**_ii; // Data of the integral image Like data[lines][rows]
};

}

#endif //__lfeat_image_h


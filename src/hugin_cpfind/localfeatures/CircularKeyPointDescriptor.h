/*
* Copyright (C) 2007-2009 Anael Orlinski & Pablo d'Angelo
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

#ifndef __lfeat_circularkeypointdescriptor_h
#define __lfeat_circularkeypointdescriptor_h

#include "Image.h"
#include "KeyPoint.h"
#include "KeyPointDescriptor.h"

namespace lfeat
{

struct SampleSpec
{
    double x,y, size;
};

class LFIMPEX CircularKeyPointDescriptor : public KeyPointDescriptor
{
public:
    CircularKeyPointDescriptor(Image& iImage,
                               std::vector<int> rings = std::vector<int>(),
                               std::vector<double> ring_radius = std::vector<double>(),
                               std::vector<double> ring_gradient_width = std::vector<double>(),
                               int ori_bins=18, double ori_sample_scale=4, int ori_gridsize=11);
    ~CircularKeyPointDescriptor();

    void makeDescriptor(KeyPoint& ioKeyPoint) const;
    int getDescriptorLength() const
    {
        return _descrLen;
    };
    int assignOrientation(KeyPoint& ioKeyPoint, double angles[4]) const;

protected:
    void createDescriptor(KeyPoint& ioKeyPoint) const;

private:
    // orig image info
    Image&			_image;
    int _vecLen;
    int _subRegions;
    int _descrLen;
    SampleSpec* _samples;
    const int _ori_nbins;
    const double _ori_sample_scale;
    const int _ori_gridsize;
    double* _ori_hist;
};

}

#endif //__lfeat_keypointdescriptor_h

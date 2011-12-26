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
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __lfeat_keypointdescriptor_h
#define __lfeat_keypointdescriptor_h

#include "Image.h"
#include "KeyPoint.h"

namespace lfeat
{

/** Abstract base class for all keypoint descriptors */
class KeyPointDescriptor
{
public:
    virtual void makeDescriptor(KeyPoint& ioKeyPoint) const = 0;
    virtual int getDescriptorLength() const = 0;
    virtual int assignOrientation(KeyPoint& ioKeyPoint, double angles[4]) const = 0;

};

}

#endif //__lfeat_keypointdescriptor_h

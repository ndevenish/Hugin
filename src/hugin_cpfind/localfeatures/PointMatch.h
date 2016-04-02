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

#ifndef __lfeatPointMatch_h
#define __lfeatPointMatch_h

#include <memory>
#include <vector>

#include "KeyPoint.h"

namespace lfeat
{

struct PointMatch
{

    PointMatch(KeyPointPtr& aPM1, KeyPointPtr& aPM2) :
        _img1_x(aPM1->_x), _img1_y(aPM1->_y), _img2_x(aPM2->_x),  _img2_y(aPM2->_y),
        _img1_kp(aPM1), _img2_kp(aPM2) {};

    double _img1_x, _img1_y, _img2_x, _img2_y;

    // hold a reference to original keypoint
    KeyPointPtr		_img1_kp;
    KeyPointPtr		_img2_kp;

    //void print()
    //{
    //	std::cout << _img1_x << " " << _img2_x << " " << _img1_y << " " << _img2_y << std::endl;
    //}

};

typedef std::shared_ptr<PointMatch> PointMatchPtr;
typedef std::vector<PointMatchPtr> PointMatchVector_t;

class PointMatchPtrSort
{
public:
    inline bool operator() (const PointMatchPtr& a, const PointMatchPtr& b) const
    {
        if (a->_img1_kp->_score < b->_img1_kp->_score)
        {
            return true;
        }
        else if (a->_img1_kp->_score > b->_img1_kp->_score)
        {
            return false;
        }
        else
        {
            // same score, order by _x coordinate (this also removes duplicate matches)
            return (a->_img1_kp->_y < b->_img1_kp->_y);
        }
    }
};

}

#endif // __lfeatPointMatch_h

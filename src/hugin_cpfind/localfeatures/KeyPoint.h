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

#ifndef __lfeat_keypoint_h
#define __lfeat_keypoint_h

#include <hugin_shared.h>

#include <boost/shared_ptr.hpp>
#include <vector>

namespace lfeat
{


class KeyPoint
{
public:
    KeyPoint();
    KeyPoint(const KeyPoint& k);
    KeyPoint(double x, double y, double s, double score, int trace);
    KeyPoint& operator=(const KeyPoint& k) throw();

    ~KeyPoint();

    void allocVector(int iSize);

    double		_x, _y;
    double		_scale;
    double		_score;
    int			_trace;
    double		_ori;

    double*		_vec;

};

inline KeyPoint::KeyPoint() : _vec(0)
{

}

inline KeyPoint::KeyPoint(double x, double y, double s, double score, int trace) :
    _x(x), _y(y), _scale(s), _score(score), _trace(trace), _vec(0)
{

}

inline KeyPoint::KeyPoint(const KeyPoint& k) :
    _x(k._x), _y(k._y), _scale(k._scale), _score(k._score), _trace(k._trace), _vec(0)
{

}


inline KeyPoint& KeyPoint::operator=(const KeyPoint& k) throw()
{
    if (this == &k)
    {
        return *this;
    }
    _x = k._x;
    _y = k._y;
    _scale = k._scale;
    _score = k._score;
    _trace = k._trace;
    _vec = 0;
    return *this;
}

inline KeyPoint::~KeyPoint()
{
    if (_vec)
    {
        delete[] _vec;
    }
}

inline void KeyPoint::allocVector(int iSize)
{
    _vec = new double[iSize];
}


inline bool operator < (const KeyPoint& iA, const KeyPoint& iB)
{
    return (iA._score < iB._score);
}




typedef boost::shared_ptr<KeyPoint> 		KeyPointPtr;
typedef std::vector<KeyPointPtr>			KeyPointVect_t;
typedef std::vector<KeyPointPtr>::iterator	KeyPointVectIt_t;

class KeyPointPtrSort
{
public:
    inline bool operator() (const KeyPointPtr& a, const KeyPointPtr& b) const
    {
        if (a->_score < b->_score)
        {
            return true;
        }
        else if (a->_score > b->_score)
        {
            return false;
        }
        else
        {
            // same score, order by scale
            return (a->_scale < b->_scale);
        }
    }
};

}

#endif //__lfeat_keypoint_h


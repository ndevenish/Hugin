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

#ifndef __lfeat_ransacfiltering_h
#define __lfeat_ransacfiltering_h

#include <vector>
#include "Homography.h"

namespace lfeat
{

class LFIMPEX Ransac
{
public:
    Ransac() : _nIter(1000), _distanceThres(25) {};

    void filter(std::vector<PointMatchPtr>& ioMatches, std::vector<PointMatchPtr>& ioRemovedMatches);
    inline void setIterations(int iIters)
    {
        _nIter = iIters;
    }
    inline void setDistanceThreshold(int iDT)
    {
        _distanceThres = iDT;
    }

    Homography	_bestModel;


    void transform(double iX, double iY, double& oX, double& oY);

private:

    double calcError(Homography* aH, PointMatch& aM);

    int		_nIter;				// number of iterations
    int		_distanceThres;	// error distance threshold in pixels


};

}

#endif

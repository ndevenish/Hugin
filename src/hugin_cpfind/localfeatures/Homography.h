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

#ifndef __lsurf_homography_h
#define __lsurf_homography_h

#include <iostream>
using namespace std;

#include "PointMatch.h"
#include <vector>

namespace lfeat
{

class LFIMPEX Homography
{
public:
    Homography();
    ~Homography();

    void initMatchesNormalization(PointMatchVector_t& iMatches);

    bool estimate(PointMatchVector_t& iMatches);

    friend ostream& operator<< (ostream& o, const Homography& H);

    void transformPoint(double iX, double iY, double& oX, double& oY);


private:
    void initialize(void);

    void addMatch(int iIndex, PointMatch& iMatch);

    double** AA;
    double*	 B, * X, *r;

    static const int kNCols;


    void allocMemory(int iNPoints);
    void freeMemory();

    // the matrices for solving least squares
    double** _Amat;
    double* _Bvec;
    double* _Rvec;
    double* _Xvec;

public:
    double	_H[3][3];	// the homography matrix.
    int		_nMatches;	// number of matches to calc homography
    int		_currentPair; // the current pair to be set

    // values for vector normalization
    double _v1x, _v2x, _v1y, _v2y;


};

ostream& operator<< (ostream& o, const Homography& H);

}

#endif // __lsurf_homography_h


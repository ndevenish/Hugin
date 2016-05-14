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

#include <iostream>
#include <float.h>
#include <math.h>
//#include "Tracer.h"

#include "Homography.h"

namespace lfeat
{

const int Homography::kNCols = 8;

inline int fsign(double x)
{
    return (x > 0 ? 1 : (x < 0) ? -1 : 0);
}

bool Givens(double** C, double* d, double* x, double* r, int N, int n, int want_r);

Homography::Homography(void) : _nMatches(0)
{
    _Amat = NULL;
    _Bvec = NULL;
    _Rvec = NULL;
    _Xvec = NULL;
    _v1x = 0;
    _v2x = 0;
    _v1y = 0;
    _v2y = 0;
    for (size_t i = 0; i < 3; ++i)
    {
        for (size_t j = 0; j < 3; ++j)
        {
            _H[i][j] = 0;
        };
    };
}

void Homography::allocMemory(int iNMatches)
{
    int aNRows = iNMatches * 2;
    _Amat = new double*[aNRows];
    for(int aRowIter = 0; aRowIter < aNRows; ++aRowIter)
    {
        _Amat[aRowIter] = new double[kNCols];
    }

    _Bvec = new double[aNRows];
    _Rvec = new double[aNRows];
    _Xvec = new double[kNCols];

    _nMatches = iNMatches;
}

void Homography::freeMemory()
{
    // free memory
    delete[] _Bvec;
    delete[] _Rvec;
    delete[] _Xvec;
    for(int aRowIter = 0; aRowIter < 2 * _nMatches; ++aRowIter)
    {
        delete[] _Amat[aRowIter];
    }
    delete[] _Amat;

    // reset number of matches
    _nMatches = 0;

}

Homography::~Homography()
{
    if (_nMatches)
    {
        freeMemory();
    }
}

void Homography::initMatchesNormalization(PointMatchVector_t& iMatches)
{
    // for each set of points (img1, img2), find the vector
    // to apply to all points to have coordinates centered
    // on the barycenter.

    _v1x = 0;
    _v2x = 0;
    _v1y = 0;
    _v2y = 0;

    //estimate the center of gravity
    for (size_t i = 0; i < iMatches.size(); ++i)
    {
        PointMatchPtr& aMatchIt = iMatches[i];
        //aMatchIt->print();

        _v1x += aMatchIt->_img1_x;
        _v1y += aMatchIt->_img1_y;
        _v2x += aMatchIt->_img2_x;
        _v2y += aMatchIt->_img2_y;
    }

    _v1x /= (double)iMatches.size();
    _v1y /= (double)iMatches.size();
    _v2x /= (double)iMatches.size();
    _v2y /= (double)iMatches.size();
}


std::ostream& operator<< (std::ostream& o, const Homography& H)
{
    o << H._H[0][0] << "\t" << H._H[0][1] << "\t" << H._H[0][2] << std::endl
        << H._H[1][0] << "\t" << H._H[1][1] << "\t" << H._H[1][2] << std::endl
        << H._H[2][0] << "\t" << H._H[2][1] << "\t" << H._H[2][2] << std::endl;

    return o;
}

void Homography::addMatch(size_t iIndex, PointMatch& iMatch)
{
    size_t aRow = iIndex * 2;
    double aI1x = iMatch._img1_x - _v1x;
    double aI1y = iMatch._img1_y - _v1y;
    double aI2x = iMatch._img2_x - _v2x;
    double aI2y = iMatch._img2_y - _v2y;

    _Amat[aRow][0] = 0;
    _Amat[aRow][1] = 0;
    _Amat[aRow][2] = 0;
    _Amat[aRow][3] = - aI1x;
    _Amat[aRow][4] = - aI1y;
    _Amat[aRow][5] = -1;
    _Amat[aRow][6] = aI1x * aI2y;
    _Amat[aRow][7] = aI1y * aI2y;

    _Bvec[aRow] = aI2y;

    aRow++;

    _Amat[aRow][0] = aI1x;
    _Amat[aRow][1] = aI1y;
    _Amat[aRow][2] = 1;
    _Amat[aRow][3] = 0;
    _Amat[aRow][4] = 0;
    _Amat[aRow][5] = 0;
    _Amat[aRow][6] = - aI1x * aI2x;
    _Amat[aRow][7] = - aI1y * aI2x;

    _Bvec[aRow] = - aI2x;
}

void Homography::transformPoint(double iX, double iY, double& oX, double& oY)
{
    double aX = iX - _v1x;
    double aY = iY - _v1y;
    double aK = double(1. / (_H[2][0] * aX + _H[2][1] * aY + _H[2][2]));

    oX = aK * (_H[0][0] * aX + _H[0][1] * aY + _H[0][2]) + _v2x;
    oY = aK * (_H[1][0] * aX + _H[1][1] * aY + _H[1][2]) + _v2y;
}

bool Homography::estimate(PointMatchVector_t& iMatches)
{
    // check the number of matches we need at least 4.
    if (iMatches.size() < 4)
    {
        //TRACE_ERROR("At least 4 matches are needed for homography");
        return false;
    }

    // create matrices for least-squares solving

    // if there is a different size of matrices set, delete them
    if (_nMatches != (int)iMatches.size() && _nMatches != 0)
    {
        freeMemory();
    }

    // if there is no memory allocated, alloc memory
    if (_nMatches == 0)
    {
        allocMemory((int)iMatches.size());
    }

    // fill the matrices and vectors with points
    for (size_t aFillRow = 0; aFillRow < iMatches.size(); ++aFillRow)
    {
        addMatch(aFillRow, *(iMatches[aFillRow]));
    }

    // solve the system
    if (!Givens(_Amat, _Bvec, _Xvec, _Rvec, _nMatches*2, kNCols, 0))
    {
        //TRACE_ERROR("Failed to solve the linear system");
        return false;
    }

    // fill the homography matrix with the values.

    _H[0][0] = _Xvec[0];
    _H[0][1] = _Xvec[1];
    _H[0][2] = _Xvec[2];

    _H[1][0] = _Xvec[3];
    _H[1][1] = _Xvec[4];
    _H[1][2] = _Xvec[5];

    _H[2][0] = _Xvec[6];
    _H[2][1] = _Xvec[7];
    _H[2][2] = 1.0;

    return true;

}




/*****************************************************************

Solve least squares Problem C*x+d = r, |r| = min!, by Given rotations
(QR-decomposition). Direct implementation of the algorithm
presented in H.R.Schwarz: Numerische Mathematik, 'equation'
number (7.33)

If 'd == NULL', d is not accesed: the routine just computes the QR
decomposition of C and exits.

If 'want_r == 0', r is not rotated back (\hat{r} is returned
instead).

*****************************************************************/

bool Givens(double** C, double* d, double* x, double* r, int N, int n, int want_r)
{
    int i, j, k;
    double w, gamma, sigma, rho, temp;
    double epsilon = DBL_EPSILON;	/* FIXME (?) */

    /*
    * First, construct QR decomposition of C, by 'rotating away'
    * all elements of C below the diagonal. The rotations are
    * stored in place as Givens coefficients rho.
    * Vector d is also rotated in this same turn, if it exists
    */
    for (j = 0; j < n; j++)
    {
        for (i = j + 1; i < N; i++)
        {
            if (C[i][j])
            {
                if (fabs(C[j][j]) < epsilon * fabs(C[i][j]))
                {
                    /* find the rotation parameters */
                    w = -C[i][j];
                    gamma = 0;
                    sigma = 1;
                    rho = 1;
                }
                else
                {
                    w = fsign(C[j][j]) * sqrt(C[j][j] * C[j][j] + C[i][j] * C[i][j]);
                    if (w == 0)
                    {
                        return false;
                    }
                    gamma = C[j][j] / w;
                    sigma = -C[i][j] / w;
                    rho = (fabs(sigma) < gamma) ? sigma : fsign(sigma) / gamma;
                }
                C[j][j] = w;
                C[i][j] = rho;	/* store rho in place, for later use */
                for (k = j + 1; k < n; k++)
                {
                    /* rotation on index pair (i,j) */
                    temp = gamma * C[j][k] - sigma * C[i][k];
                    C[i][k] = sigma * C[j][k] + gamma * C[i][k];
                    C[j][k] = temp;

                }
                if (d)  	/* if no d vector given, don't use it */
                {
                    temp = gamma * d[j] - sigma * d[i];		/* rotate d */
                    d[i] = sigma * d[j] + gamma * d[i];
                    d[j] = temp;
                }
            }
        }
    }

    if (!d)			/* stop here if no d was specified */
    {
        return true;
    }

    /* solve R*x+d = 0, by backsubstitution */
    for (i = n - 1; i >= 0; i--)
    {
        double s = d[i];

        r[i] = 0;		/* ... and also set r[i] = 0 for i<n */
        for (k = i + 1; k < n; k++)
        {
            s += C[i][k] * x[k];
        }
        if (C[i][i] == 0)
        {
            return false;
        }
        x[i] = -s / C[i][i];
    }

    for (i = n; i < N; i++)
    {
        r[i] = d[i];    /* set the other r[i] to d[i] */
    }

    if (!want_r)		/* if r isn't needed, stop here */
    {
        return true;
    }

    /* rotate back the r vector */
    for (j = n - 1; j >= 0; j--)
    {
        for (i = N - 1; i >= 0; i--)
        {
            if ((rho = C[i][j]) == 1)  		/* reconstruct gamma, sigma from stored rho */
            {
                gamma = 0;
                sigma = 1;
            }
            else if (fabs(rho) < 1)
            {
                sigma = rho;
                gamma = sqrt(1 - sigma * sigma);
            }
            else
            {
                gamma = 1 / fabs(rho);
                sigma = fsign(rho) * sqrt(1 - gamma * gamma);
            }
            temp = gamma * r[j] + sigma * r[i];		/* rotate back indices (i,j) */
            r[i] = -sigma * r[j] + gamma * r[i];
            r[j] = temp;
        }
    }
    return true;
}

}

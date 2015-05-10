// -*- c-basic-offset: 4 -*-
/** @file FitPolynom.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _FITPOLYNOM_H
#define _FITPOLYNOM_H

#include "hugin_math/Matrix3.h"

namespace vigra_ext
{

/** calculate the determinat of
 *  a 3x3 matrix using the sarrus formula
 */
template <typename M>
double calcDeterminant3(const M &m)
{
    return    m[0][0] * m[1][1] * m[2][2]
            + m[0][1] * m[1][2] * m[2][0]
	    + m[0][2] * m[1][0] * m[2][1]
	    - m[2][0] * m[1][1] * m[0][2]
	    - m[2][1] * m[1][2] * m[0][0]
	    - m[2][2] * m[1][0] * m[0][1];
}

/** fit a second order polynom to a data set
 *
 *  y = a + b*x + c*x^2
 *
 *  uses a least mean square fit, give at least 3 points.
 *
 *  @TODO calculate quality of fit
 */
template <class T>
void FitPolynom(T x, T xend, T y, double & a, double & b, double & c)
    {
        size_t n = xend - x;
        // calculate various sums.
        double sx=0;
        double sx2=0;
        double sx3=0;
        double sx4=0;
        double sy=0;
        double sxy=0;
        double sx2y=0;
        T xi,yi;
        for (xi=x, yi=y; xi != xend; ++xi, ++yi) {
            double tx = *xi;
            double ty = *yi;
            double t = tx;
            sx += tx;
            sy += ty;
            sxy += tx * ty;
            tx = tx * t;
            sx2 += tx;
            sx2y += tx * ty;
            tx = tx * t;
            sx3 +=tx;
            sx4 += tx * t;
        }
        
        // X*A=Y
        Matrix3 X;
        X.m[0][0] = n;
        X.m[0][1] = sx;
        X.m[0][2] = sx2;
        X.m[1][0] = sx;
        X.m[1][1] = sx2;
        X.m[1][2] = sx3;
        X.m[2][0] = sx2;
        X.m[2][1] = sx3;
        X.m[2][2] = sx4;

        // calculate det(X)
        double D = X.Determinant(); //calcDeterminant3(X.m);

        // matrix to calculate a;
        X.m[0][0] = sy;
        X.m[0][1] = sx;
        X.m[0][2] = sx2;
        X.m[1][0] = sxy;
        X.m[1][1] = sx2;
        X.m[1][2] = sx3;
        X.m[2][0] = sx2y;
        X.m[2][1] = sx3;
        X.m[2][2] = sx4;
        double A = X.Determinant();
        a = A / D;

        // matrix to calculate b
        X.m[0][0] = n;
        X.m[0][1] = sy;
        X.m[0][2] = sx2;
        X.m[1][0] = sx;
        X.m[1][1] = sxy;
        X.m[1][2] = sx3;
        X.m[2][0] = sx2;
        X.m[2][1] = sx2y;
        X.m[2][2] = sx4;
        b = X.Determinant() / D;

        // matrix to calculate c
        X.m[0][0] = n;
        X.m[0][1] = sx;
        X.m[0][2] = sy;
        X.m[1][0] = sx;
        X.m[1][1] = sx2;
        X.m[1][2] = sxy;
        X.m[2][0] = sx2;
        X.m[2][1] = sx3;
        X.m[2][2] = sx2y;
        c = X.Determinant() / D;
    }

}

#endif // _FITPOLYNOM_H

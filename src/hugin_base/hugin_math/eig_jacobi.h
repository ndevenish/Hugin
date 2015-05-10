// -*- c-basic-offset: 4 -*-

/** @file eig_jacobi.h
 *
 *  @brief lu decomposition and linear LMS solver
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */


#ifndef _HUGIN_MATH_EIG_JACOBI_H
#define _HUGIN_MATH_EIG_JACOBI_H


namespace hugin_utils
{

    /** Implements jacobi eigenvalue/vector algorithm on a symmetric matrix
        stored as a 2 dimensional matrix a[n][n] and computes the eigenvectors
        in another globally allocated matrix v[n][n].
    intput:
        n        - size of matrix problem
    outputs:
        v        - eigenvector matrix
        d[MAX]   - a vector of unsorted eigenvalues
        ind[MAX] - a vector of indicies sorting d[] into descending order
        maxanil  - number of rotations applied
        inputs/outputs
        a        - input matrix (the input is changed)
        maxsweep - on input max number of sweeps
        - on output actual number of sweeps
        epsilon  - on input tolerance to consider offdiagonal elements as zero
        - on output sum of offdiagonal elements
    */
    void eig_jacobi( int n, double a[3][3], double v[3][3], double *d,int* ind,int* maxsweep,int* maxannil,double* epsilon);

}

#endif // _H




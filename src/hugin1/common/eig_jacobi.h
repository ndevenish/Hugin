// -*- c-basic-offset: 4 -*-

/** @file jacobi_eig.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#ifndef _Hgn1_COMMON_EIG_H
#define _Hgn1_COMMON_EIG_H

#include <hugin_math/eig_jacobi.h>

namespace utils
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
    using hugin_utils::eig_jacobi;
    
}

#endif
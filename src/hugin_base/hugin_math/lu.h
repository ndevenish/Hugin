// -*- c-basic-offset: 4 -*-

/** @file lu.h
 *
 *  @brief lu decomposition and linear LMS solver
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *  @author Antoine Lefebvre <antoine.lefebvre@polymtl.ca>
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


/* The LU decomposition function was taken from the rocketworkbench project:
 * 
 * lu.c  -  PA = LU factorisation with pivoting
 * $Id$
 * Copyright (C) 2000
 *    Antoine Lefebvre <antoine.lefebvre@polymtl.ca>
 *
 *
 * Licensed under the GPLv2
 */


#ifndef _HUGIN_MATH_LU_H
#define _HUGIN_MATH_LU_H


/* NOTE on matrix representation
 * -----------------------------
 * All matrix should be allocate the following way
 *
 * matrix = (double *) malloc (sizeof(double) * line * column)
 *
 * to access the value at line 2, column 4, you do it like that
 *
 * matrix[2 + 4*line]
 */


/* Find the solution of linear system of equation using the
 * LU factorisation method as explain in
 * 'Advanced engineering mathematics' bye Erwin Kreyszig.
 *
 * This algorithm will also do column permutation to find
 * the larger pivot.
 *
 * ARGUMENTS
 * ---------
 * matrix: the augmented matrix of coefficient in the system
 *         with right hand side value.
 *
 * solution: the solution vector
 *
 * neq: number of equation in the system
 *
 * Antoine Lefebvre
 *    february 6, 2000 Initial version
 *    october 20, 2000 revision of the permutation method
 */

extern "C"
{
int math_lu_solve(double *matrix, double *solution, int neq);
}


namespace hugin_utils
{

    /** Solve a linear least squares problem */
    class LMS_Solver
    {
    public:
        /** Create a new LMS solver.
         *   A*x = b, solve for x using the pseudoinverse and LU decomposition:
         *   A'A x = A' b.
         */
        LMS_Solver(unsigned nEq)
        {
            m_nEq = nEq;
            m_AtA = new double[nEq*(nEq+1)];
            for (unsigned i=0; i < nEq*(nEq+1); i++) m_AtA[i] = 0;
        }

        ~LMS_Solver()
        {
            delete[] m_AtA;
        }

        /** Add a single equation (row) to the solver */
        template <class Iter>
        void addRow(Iter Arow, double b)
        {
            for( unsigned i=0; i<m_nEq; ++i)
            {
                // calculate  Atb
                m_AtA[i + m_nEq*m_nEq]+=Arow[i]*b;
                for( unsigned j=0; j<m_nEq; ++j)
                {
                    m_AtA[i + j*m_nEq] += Arow[i]*Arow[j];
                }
            }
        }

        /** calculate LMS solution, returns false if no solution could be found */
        template <class Vector>
        bool solve(Vector & x)
        {
            double * solution = new double[m_nEq];
            bool ret = math_lu_solve(m_AtA, solution, m_nEq) != 0;
            for (unsigned i=0; i < m_nEq; i++) {
                x[i] = solution[i];
            }
            delete[] solution;
            return ret;
        }

    protected:
        unsigned m_nEq;
        // matrix that contains [AtA  Atb]
        // uses fortran array access conventions
        double * m_AtA;
    };

} // namespace

#endif // _H




// -*- c-basic-offset: 4 -*-

/** @file lu.h
 *
 *  @brief lu decomposition and linear LMS solver
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *  @author Antoine Lefebvre <antoine.lefebvre@polymtl.ca>
 *
 *  $Id: lu.h 1612 2006-06-01 19:04:35Z dangelo $
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
 * $Id: lu.h 1612 2006-06-01 19:04:35Z dangelo $
 * Copyright (C) 2000
 *    Antoine Lefebvre <antoine.lefebvre@polymtl.ca>
 *
 *
 * Licensed under the GPLv2
 */


#ifndef _Hgn1_COMMON_LU_H
#define _Hgn1_COMMON_LU_H

#include <hugin_math/lu.h>


//extern "C"
//{
//int math_lu_solve(double *matrix, double *solution, int neq);
//}


namespace utils
{

/** Solve a linear least squares problem */
using hugin_utils::LMS_Solver;
        
} // namespace



#endif




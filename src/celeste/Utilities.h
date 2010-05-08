/* Import from Gabor API

Copyright (c) 2002-3 Adriaan Tijsseling


                             All Rights Reserved

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

/*
	Author:			Adriaan Tijsseling (AGT)
	Copyright: 		(c) Copyright 2002-3 Adriaan Tijsseling. All rights reserved.
	Description:	Probably useful...
*/

#ifndef __UTILITIES__
#define __UTILITIES__

#include <hugin_shared.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#ifdef _WINDOWS
#include "direct.h"
#endif

using namespace std; 

enum
{
	kLeft = 0,
	kRight
};

IMPEX void		Permute( int* array, size_t size );
IMPEX int			cmp(const void *s1, const void *s2); 	// for qsort() function

IMPEX float		Heavyside( float a );

IMPEX float		Sigmoid( float act );
IMPEX float		Sigmoid( float beta, float a_pot );
IMPEX float		Sigmoid( float beta, float a_pot, float thresh );

IMPEX int			**CreateMatrix( int val, int row, int col );
IMPEX void		ResetMatrix( int ** matrix, int val, int row, int col );
IMPEX void		DisposeMatrix( int** matrix, int row );

IMPEX float		**CreateMatrix( float val, int row, int col );
IMPEX void		ResetMatrix( float ** matrix, float val, int row, int col );
IMPEX void		DisposeMatrix( float** matrix, int row );

IMPEX float 		ReturnDistance( float *pat1, float *pat2, int size );

IMPEX void		GetStreamDefaults( void );
IMPEX void 		AdjustStream( ostream &os, int precision, int width, int pos, bool trailers );
IMPEX void		SetStreamDefaults( ostream &os );

IMPEX void 		SkipComments( ifstream* infile );
IMPEX void 		FileCreateError( char* filename );
IMPEX void 		FileOpenError( char* filename );

IMPEX double		SafeAbs( double val1, double val2 );
IMPEX float		SafeAbs( float val1, float val2 );
IMPEX int			SafeAbs( int val1, int val2 );
IMPEX double		SafeAbs( double val );
IMPEX float		SafeAbs( float val );
IMPEX int			SafeAbs( int val );

#endif


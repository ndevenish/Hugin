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

#include <fstream>
#include <iostream>
#include <stdio.h>
#ifdef _WIN32
#include "direct.h"
#else
#include <unistd.h>
#endif

namespace celeste
{
enum
{
	kLeft = 0,
	kRight
};

void		Permute( int* array, size_t size );
int			cmp(const void *s1, const void *s2); 	// for qsort() function

float		Heavyside( float a );

float		Sigmoid( float act );
float		Sigmoid( float beta, float a_pot );
float		Sigmoid( float beta, float a_pot, float thresh );

int			**CreateMatrix( int val, int row, int col );
void		ResetMatrix( int ** matrix, int val, int row, int col );
void		DisposeMatrix( int** matrix, int row );

float		**CreateMatrix( float val, int row, int col );
void		ResetMatrix( float ** matrix, float val, int row, int col );
void		DisposeMatrix( float** matrix, int row );

float 		ReturnDistance( float *pat1, float *pat2, int size );

void		GetStreamDefaults( void );
void 		AdjustStream( std::ostream &os, int precision, int width, int pos, bool trailers );
void		SetStreamDefaults( std::ostream &os );

void 		SkipComments( std::ifstream* infile );
void 		FileCreateError( char* filename );
void 		FileOpenError( char* filename );

double		SafeAbs( double val1, double val2 );
float		SafeAbs( float val1, float val2 );
int			SafeAbs( int val1, int val2 );
double		SafeAbs( double val );
float		SafeAbs( float val );
int			SafeAbs( int val );
}; // namespace
#endif


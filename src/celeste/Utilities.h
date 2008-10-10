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
#include <Config.h>

using namespace std; 

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
void 		AdjustStream( ostream &os, int precision, int width, int pos, bool trailers );
void		SetStreamDefaults( ostream &os );

void 		SkipComments( ifstream* infile );
void 		FileCreateError( char* filename );
void 		FileOpenError( char* filename );

double		SafeAbs( double val1, double val2 );
float		SafeAbs( float val1, float val2 );
int			SafeAbs( int val1, int val2 );
double		SafeAbs( double val );
float		SafeAbs( float val );
int			SafeAbs( int val );

#endif


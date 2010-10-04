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
	Description:		class for log polar filter
	Original Author:	Takio Kurita
	Ported by:			Adriaan Tijsseling (AGT)
*/

#include "LogPolar.h"
#include "CelesteGlobals.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

namespace celeste
{
// construct class and apply filter
LogPolar::LogPolar( float** img, int height, int width, int minS, int ry, int rx )
{
	mImgHeight = height;
	mImgWidth = width;
	mHeight = ry;
	mWidth = rx;
	mMinHW = minS;
	
	// allocate output image
	mPolarized = new float*[mHeight];
	for ( int i = 0; i < mHeight; i++ )
	{
		mPolarized[i] = new float[mWidth];
		for ( int j = 0; j < mWidth; j++ )
			mPolarized[i][j] = 0.0;
	}
	// allocate coordinates image
	mCoords = new float*[mImgHeight];
	for ( int i = 0; i < mImgHeight; i++ )
	{
		mCoords[i] = new float[mImgWidth];
		for ( int j = 0; j < mImgWidth; j++ )
			mCoords[i][j] = 0.0;
	}
	// apply filter
	ApplyFilter( img, height, width );
}


// free memory
LogPolar::~LogPolar()
{
	if( mCoords != NULL )
	{
		for( int y = 0; y < mImgHeight; y++ )
			delete[] mCoords[y];
		delete[] mCoords;
	}
	if( mPolarized != NULL )
	{
		for( int y = 0; y < mHeight; y++ )
			delete[] mPolarized[y];
		delete[] mPolarized;
	}
}


// apply filter to image
void LogPolar::ApplyFilter( float** img, int height, int width )
{
	float	rho, theta, x, y, sum;
	int		i, j, k, l, f, g;

	for( k = 0; k < mHeight; k++ )
	{
		theta = (float)(2.0 * M_PI * (float)k / (float)mHeight);

		for( l = 0; l < mWidth; l++ )
		{
			rho = exp( log( (float)((float)mMinHW / 2.0) ) * (float)l / (float)mWidth );

			x = rho * cos( theta );   
			y = rho * sin( theta );

			if ( x >= 0.0 ) x += 0.5;
			else x -= 0.5;
			if ( y >= 0.0 ) y += 0.5;
			else y -= 0.5;

			f = (int)(y) + (int)height/2;  
			g = (int)(x) + (int)width/2;

			sum = 0.0;
			for( i = f-1; i <= f+1; i++ )
				for( j = g-1; j <= g+1; j++ )
					sum += img[i][j];
			
			mPolarized[k][l] = sum / 9.0f;
			mCoords[f][g] = 255.0f;
		}
	}
}


// write out contrast data to pgm file
void LogPolar::Save( void )
{
    PGMImage	pgmI;
	char		tmpname[256];
	
	strcpy( tmpname, mFile );
	strcat( tmpname, "-lp-hist.pgm" );
	pgmI.WriteScaled( tmpname, mPolarized, mHeight, mWidth );
	if ( kSaveFilter )
	{
		strcpy( tmpname, mFile );
		strcat( tmpname, "-lp-img.pgm" );
		pgmI.WriteScaled( tmpname, mCoords, mImgHeight, mImgWidth );
	}
}

}; // namespace
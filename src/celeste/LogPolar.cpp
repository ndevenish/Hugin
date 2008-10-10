/*
	Description:		class for log polar filter
	Original Author:	Takio Kurita
	Ported by:			Adriaan Tijsseling (AGT)
*/

#include "LogPolar.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

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
		theta = 2.0 * M_PI * (float)k / (float)mHeight;

		for( l = 0; l < mWidth; l++ )
		{
			rho = exp( log( (float)mMinHW / 2.0 ) * (float)l / (float)mWidth );

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
			
			mPolarized[k][l] = sum / 9.0;
			mCoords[f][g] = 255.0;
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



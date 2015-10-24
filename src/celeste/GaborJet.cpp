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
	Description:	Implementation for GaborJet class
	Author:			Adriaan Tijsseling (AGT)
	Copyright: 		(c) Copyright 2002-3 Adriaan Tijsseling. All rights reserved.
*/

#include "GaborJet.h"
#include "CelesteGlobals.h"

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

namespace celeste
{
// default constructor just sets everything to default
GaborJet::GaborJet()
{
	mHeight 	= 512;
	mWidth 		= 512;
	mX			= 128;
	mY			= 128;
	mFilters 	= NULL;
	mFiducials	= NULL;
    mAngles = 0;
    mFreqs = 0;
    mRadius = 0;
}

// destructor: free up memory
GaborJet::~GaborJet()
{
	if ( mFilters != NULL )
	{
		for ( int i = 0; i < mAngles; i++ ) delete[] mFilters[i];
		delete[] mFilters;
	}
	if ( mFiducials != NULL ) delete[] mFiducials;	
}


// set up the filter
void GaborJet::Initialize( int y, int x, int x0, int y0, int r, 
    float s, int f, float maxF, float minF, int a, char* file)
{
	int		i, j;
	float	freq;
	
// set internal variables
	mHeight 	= y;
	mWidth 		= x;
	mX			= x0;
	mY			= y0;
	float sigma	= (float)(s * M_PI * M_PI);
	mAngles 	= a;
	mFreqs 		= f;
	mRadius		= r;
	mFiducials = new float[mAngles * mFreqs];
	
// allocate memory for filters (angles * freqs = total filters)
	mFilters = new GaborFilter * [mAngles];
	for ( i = 0; i < mAngles; i++ )
	{
	// calculate angle
		float angle = (float)((float)i * M_PI / (float)mAngles);
		
	// allocate filters for this angle
		mFilters[i] = new GaborFilter[mFreqs];	
		
	// initialize each one	
		for ( j = 0; j < mFreqs; j++ )
		{
		// calculate frequency
			freq = minF + ( j * ( maxF - minF ) ) / (float)mFreqs;
			
		// initialize filter
			mFilters[i][j].Initialize( mRadius, angle, freq, sigma );
            if (file!=NULL && strlen(file)>0)
            {
                mFilters[i][j].Save(file, i, j);
            };
		}
	}	
}


// process an image
void GaborJet::Filter( float** image, int* len )
{	
	int			x, y;		// iterating over location
	int			gx, gy;		// iterating over filters
	int			a, f;		// iterating over angles and frequencies
	int			h, i, j;	// iterating over filter field
	float		sumI, sumR;	// sum of imaginary and of real parts
	
	if ( kVerbosity ) std::cerr << "convoluting..." << std::endl;

// convolve at center of filter location
	// collect responses over angles and frequencies
	h = 0;
	for ( a = 0; a < mAngles; a++ )
	{
		for ( f = 0; f < mFreqs; f++ )
		{
			sumR = 0.0;
			sumI = 0.0;

		// start from bottom-left corner of filter location
			y = mY - mRadius;
			for ( gy = y; gy < y + 2 * mRadius; gy++ )
			{
			// make sure we are not out of bounds
				if ( gy < 0 || gy >= mHeight ) break;
				
			// offset to local coordinates of filter
				i = gy - y;
				
				x = mX - mRadius;
				for ( gx = x; gx < x + 2 * mRadius; gx++ )
				{
				// make sure we are not out of bounds
					if ( gx < 0 || gx >= mWidth ) break;

				// offset to local coordinates of filter
					j = gx - x;

					sumR += image[gy][gx] * mFilters[a][f].GetReal(i,j);
					sumI += image[gy][gx] * mFilters[a][f].GetImaginary(i,j);
				}
			}
			mFiducials[h] = sqrt( sumR*sumR + sumI*sumI );
			h++;
		} // f
	} // a

	*len = mAngles * mFreqs;
}

}; // namespace
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
	Description:		Abstract class for contrast filter
	Original Author:	Yasunobu Honma
	Modifications by:	Adriaan Tijsseling (AGT)
*/

#include "ContrastFilter.h"

namespace celeste
{
float CONTRAST[9][9] = {
  {
    -0.00601522f,
    -0.00815698f,
    -0.00576532f,
    -0.000761649f,
    0.00105624f,
    -0.000761649f,
    -0.00576532f,
    -0.00815698f,
    -0.00601522f
  },

  {
    -0.00815698f,
    -0.00235211f,
    0.00300229f,
    -0.0157626f,
    -0.0304662f,
    -0.0157626f,
    0.00300229f,
    -0.00235211f,
    -0.00815698f
  },

  {
    -0.00576532f,
    0.00300229f,
    -0.0505102f,
    -0.115416f,
    -0.115769f,
    -0.115416f,
    -0.0505102f,
    0.00300229f,
    -0.00576532f
  },

  {
    -0.000761649f,
    -0.0157626f,
    -0.115416f,
    0.0361012f,
    0.273771f,
    0.0361012f,
    -0.115416f,
    -0.0157626f,
    -0.000761649f
  },

  {
    0.00105624f,
    -0.0304662f,
    -0.115769f,
    0.273771f,
    0.719623f,
    0.273771f,
    -0.115769f,
    -0.0304662f,
    0.00105624f
  },

  {
    -0.000761649f,
    -0.0157626f,
    -0.115416f,
    0.0361012f,
    0.273771f,
    0.0361012f,
    -0.115416f,
    -0.0157626f,
    -0.000761649f
  },

  {
    -0.00576532f,
    0.00300229f,
    -0.0505102f,
    -0.115416f,
    -0.115769f,
    -0.115416f,
    -0.0505102f,
    0.00300229f,
    -0.00576532f
  },

  {
    -0.00815698f,
    -0.00235211f,
    0.00300229f,
    -0.0157626f,
    -0.0304662f,
    -0.0157626f,
    0.00300229f,
    -0.00235211f,
    -0.00815698f
  },

  {
    -0.00601522f,
    -0.00815698f,
    -0.00576532f,
    -0.000761649f,
    0.00105624f,
    -0.000761649f,
    -0.00576532f,
    -0.00815698f,
    -0.00601522f
  }};

// construct class and apply filter
ContrastFilter::ContrastFilter( float **img, int height, int width )
{
	mHeight = height-8;
	mWidth = width-8;

	mContrast = new float*[mHeight];
	for ( int i = 0; i < mHeight; i++ )
	{
		mContrast[i] = new float[mWidth];
		for ( int j = 0; j < mWidth; j++ )
			mContrast[i][j] = 0.0;
	}

	ApplyFilter( img, height, width );
}


// free memory
ContrastFilter::~ContrastFilter()
{
	if( mContrast != NULL )
	{
		for( int y = 0; y < mHeight; y++ )
			delete[] mContrast[y];
		delete[] mContrast;
	}
}


// apply filter to image
void ContrastFilter::ApplyFilter( float** img, int height, int width )
{
	int 	x, y, i, j;//, k, l;
	float 	tmp;

	for( i = 0; i < height-8; i++ ) 
		for( j = 0; j < width-8; j++ )
		{
			tmp = 0.0;
			for( x = 0; x < 9; x++ )
				for( y = 0; y < 9; y++)
					tmp += CONTRAST[x][y] * img[i+x][j+y];
			mContrast[i][j] = tmp;
		}
}


// write out contrast data to pgm file
void ContrastFilter::Save( void )
{
    PGMImage	pgmI;
	char		tmpName[256];
	
	strcpy( tmpName, mFile );
	strcat( tmpName, "-contrast.pgm" );
	pgmI.WriteScaled( tmpName, mContrast, mHeight, mWidth );
}

}; // namespace
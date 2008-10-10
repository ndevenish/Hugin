/*
	Description:		Abstract class for contrast filter
	Original Author:	Yasunobu Honma
	Modifications by:	Adriaan Tijsseling (AGT)
*/

#include "ContrastFilter.h"

float CONTRAST[9][9] = {
  {
    -0.00601522,
    -0.00815698,
    -0.00576532,
    -0.000761649,
    0.00105624,
    -0.000761649,
    -0.00576532,
    -0.00815698,
    -0.00601522
  },

  {
    -0.00815698,
    -0.00235211,
    0.00300229,
    -0.0157626,
    -0.0304662,
    -0.0157626,
    0.00300229,
    -0.00235211,
    -0.00815698
  },

  {
    -0.00576532,
    0.00300229,
    -0.0505102,
    -0.115416,
    -0.115769,
    -0.115416,
    -0.0505102,
    0.00300229,
    -0.00576532
  },

  {
    -0.000761649,
    -0.0157626,
    -0.115416,
    0.0361012,
    0.273771,
    0.0361012,
    -0.115416,
    -0.0157626,
    -0.000761649
  },

  {
    0.00105624,
    -0.0304662,
    -0.115769,
    0.273771,
    0.719623,
    0.273771,
    -0.115769,
    -0.0304662,
    0.00105624
  },

  {
    -0.000761649,
    -0.0157626,
    -0.115416,
    0.0361012,
    0.273771,
    0.0361012,
    -0.115416,
    -0.0157626,
    -0.000761649
  },

  {
    -0.00576532,
    0.00300229,
    -0.0505102,
    -0.115416,
    -0.115769,
    -0.115416,
    -0.0505102,
    0.00300229,
    -0.00576532
  },

  {
    -0.00815698,
    -0.00235211,
    0.00300229,
    -0.0157626,
    -0.0304662,
    -0.0157626,
    0.00300229,
    -0.00235211,
    -0.00815698
  },

  {
    -0.00601522,
    -0.00815698,
    -0.00576532,
    -0.000761649,
    0.00105624,
    -0.000761649,
    -0.00576532,
    -0.00815698,
    -0.00601522
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



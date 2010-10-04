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
	Description:		Abstract class for reading and storing images
	Original Author:	Mickael Pic
	Modifications by:	Adriaan Tijsseling (AGT)
*/


#include "ImageFile.h"

namespace celeste
{
ImageFile::ImageFile()
{
    mPixels = NULL;
    mFloats = NULL;
    mRGB = NULL;
    mWidth = 0;
    mHeight = 0;
    mVerbosity = true;
}

ImageFile::~ImageFile()
{
    Deallocate();
}


// allocate pixel storage
void ImageFile::Allocate( int dataset )
{
	int i, j;

	if ( dataset & kChars )
	{	
		mPixels = new unsigned char*[mHeight];
		for ( i = 0; i < mHeight; i++ )
		{
			mPixels[i] = new unsigned char[mWidth];
			for ( j = 0; j < mWidth; j++ )
				mPixels[i][j] = 0;
		}
	}
	if ( dataset & kFloats )
	{
		mFloats = new float*[mHeight];
		for ( i = 0; i < mHeight; i++ )
		{
			mFloats[i] = new float[mWidth];
			for ( j = 0; j < mWidth; j++ )
				mFloats[i][j] = 0.0;
		}
	}
	if ( dataset & kRGB )
	{
		mRGB = new int**[3];
		for ( i = 0; i < 3; i++ )
		{
			mRGB[i] = new int*[mHeight];
			for ( j = 0; j < mHeight; j++ )
			{
				mRGB[i][j] = new int[mWidth];
				for ( int k = 0; k < mWidth; k++ )
					mRGB[i][j][k] = 255;
			}			
		}	
	}
}


// allocate pixel storage
void ImageFile::Deallocate()
{
	int i;
	
	if ( mPixels != NULL )
	{
    	for ( i = 0; i < mHeight; i++ )
    		delete[] mPixels[i];
		 delete[] mPixels; 
	}
	if ( mFloats != NULL )
	{
    	for ( i = 0; i < mHeight; i++ )
    		delete[] mFloats[i];
		 delete[] mFloats; 
	}
	if ( mRGB == NULL ) return;
	for ( i = 0; i < 3; i++ )
	{
		for ( int j = 0; j < mHeight; j++ )
			delete[] mRGB[i][j];
		delete[] mRGB[i];
	}	
	delete[] mRGB;
}


// get one single pixel
unsigned char ImageFile::GetPixel( int x, int y )
{
	if ( mPixels != NULL )
		return mPixels[x][y];
	else 
		return 0;
}


// Set the image from a table of float
void ImageFile::SetPixels( float** pixels )
{
	for ( int i = 0; i < mHeight; i++ )
		for ( int j = 0; j < mWidth; j++ )
			mPixels[i][j] = (unsigned char)pixels[i][j];
}


// return float cast of pixel storage
float** ImageFile::GetPixels( void )
{
	// allocate pixel storage
	Allocate( kFloats );

 	for ( int i = 0; i < mHeight; i++ )
		for ( int j = 0; j < mWidth; j++ )
			mFloats[i][j] = (float)mPixels[i][j];

	return mFloats;
}
}; // namespace
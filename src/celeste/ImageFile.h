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

#ifndef __IMAGE_FILE_CLASS__
#define __IMAGE_FILE_CLASS__

#include	<iostream>
#include 	<fstream>
#include	<string>
#include 	<math.h>
#include 	<stdio.h>
#include	<stdlib.h>

namespace celeste
{
enum
{
	kChars = 0x01,
	kFloats = 0x02,
	kRGB = 0x04
};

class ImageFile
{ 
public:

	ImageFile();
	virtual ~ImageFile();

	// set or get width of image
	inline void		SetWidth( int w ) { mWidth = w; }
	inline int		GetWidth() { return mWidth; }

	// set or get height of image
	inline void		SetHeight( int h ){ mHeight = h; }
	inline int		GetHeight(){ return mHeight; }

	// set or get one single pixel
	inline void 	SetPixel( int x, int y, unsigned char p ) { if ( mPixels != NULL ) mPixels[x][y] = p; }
	unsigned char	GetPixel( int x, int y );

	// set or get pixels
	inline int***	GetRGBPixels( void ) { return mRGB; }
	void 			SetPixels( float** );
	float**			GetPixels( void );

	// allocate pixelmap
	void			Allocate( int dataset );
	void			Deallocate();

	// read to or write image from file
	virtual int		Read( char* ) = 0;
	virtual void	Write( char* ) = 0;

protected:
	
	int***			mRGB;		// rgb pixels
	unsigned char**	mPixels;	// pixel storage
	float**			mFloats;	// converted to floats
	int 			mWidth;		// image width
	int 			mHeight;	// image height
	bool			mVerbosity;	// verbosity level

 };
}//namespace
#endif

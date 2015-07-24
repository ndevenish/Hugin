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
	Description:		class for reading and storing pgm images
	Author:				Adriaan Tijsseling ( AGT )
	Copyright: 			( c ) Copyright 2002-3 Adriaan Tijsseling. All rights reserved.
*/

#ifndef __PGM_IMAGE_CLASS__
#define __PGM_IMAGE_CLASS__

#include "ImageFile.h"
#include <cstring>

namespace celeste
{
class PGMImage : public ImageFile
{
public:

	PGMImage(){};
	explicit PGMImage( char* file ) { Read( file ); }
	virtual ~PGMImage(){};

	// Read a PGM image from a file
	int	Read( char* );

	// Write a PGM image in a file
	void	Write( char* );
	void	Write( char*, float**, int, int );
	void	Write( char*, float***, int, int );
	void	Write( char*, float**, int, int, int );
	void	WriteScaled( char* filename, float** output, int height, int width );

private:
	char	mMagicNumber[2];
	int  	mNumPixels;		// Total number of pixels (mHeight x mWidth)
	int		mNumLevels;
	int		mNumBits;
};
}; //namespace
#endif // __PGM_IMAGE_CLASS__

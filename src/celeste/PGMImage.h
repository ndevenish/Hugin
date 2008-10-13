/*
	Description:		class for reading and storing pgm images
	Author:				Adriaan Tijsseling ( AGT )
	Copyright: 			( c ) Copyright 2002-3 Adriaan Tijsseling. All rights reserved.
*/

#ifndef __PGM_IMAGE_CLASS__
#define __PGM_IMAGE_CLASS__

#include "ImageFile.h"
#include <cstring>

class PGMImage : public ImageFile
{
public:

	PGMImage(){};
	PGMImage( char* file ) { Read( file ); }
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

#endif // __PGM_IMAGE_CLASS__

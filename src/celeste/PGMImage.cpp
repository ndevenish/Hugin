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

#include <cstring>
#include "PGMImage.h"

using namespace std; 

namespace celeste
{
// read PGM image from file
int PGMImage::Read( char* file )
{
	int			i,j;
	char		buf[256];
	ifstream	imgFile( file );

	if ( !imgFile ) // Invalid FileName
	{
		cerr << "invalid filename: \"" << file << "\"" << endl;
		exit(1);
		return 0;
	}
	if ( mVerbosity ) cerr << "reading image from file \"" << file << "\"" << endl;

// get file type
	imgFile.getline( mMagicNumber, 256 );

// ignore comments
	imgFile.getline( buf, 256 );
	while ( buf[0] == '#' ) imgFile.getline( buf, 256 );
	
// get dimensions of image
	mWidth = atoi( buf );
	mHeight = atoi( strpbrk( buf, " \t" ) );
	mNumPixels = mWidth * mHeight;

// get color levels
	if ( mMagicNumber[1] == '1' || mMagicNumber[1] == '4' )
	{
		mNumLevels = 1;
	}
	else
	{
		imgFile.getline( buf, 256	 );
	 	mNumLevels = atoi( buf );
	}

// determine number of bits given image level
	mNumBits = (int)( log( (float)( mNumLevels+2 ) )/log( 2.0 ) );
	if ( mVerbosity ) cerr << "[" << mNumBits << "-bit ";

// read pixels
	if ( mMagicNumber[1] == '5' || mMagicNumber[1] == '2' )	  // GrayScale
	{
	// allocate pixel storage
		Allocate( kChars );

		if ( mMagicNumber[1] == '5' )		// RAWBITs
		{
			if ( mVerbosity ) cerr << "GrayScale RAWBITs PGM format]";
			for ( i = 0; i < mHeight; i++ )
				imgFile.read( (char *)mPixels[i], mWidth );
		}
		else								// ASCII
		{
			if ( mVerbosity ) cerr << "GrayScale ASCII PGM format]";
			for ( i = 0; i < mHeight; i++ )
			{
				for ( j = 0; j < mWidth; j++ )
				{
					int pix;
					imgFile >> pix;
					mPixels[i][j] = (unsigned char)( pix / pow( 2, (double)(mNumBits-8) ) );
				}
			}
		}
	}
	else if ( mMagicNumber[1] == '6' || mMagicNumber[1] == '3' ) // RGB
	{
		unsigned char rgb[3];
	// allocate rgb pixel storage
		Allocate( kRGB );
		
		if ( mMagicNumber[1] == '6' )	// RAWBITs
		{
			if ( mVerbosity ) cerr << "RGB RAWBITs PPM format]";
			for ( i = 0; i < mHeight; i++ )
				for ( j = 0; j < mWidth; j++ )
				{
					imgFile.read( (char *)rgb,3 );
					mRGB[0][i][j] = (int)rgb[0];
					mRGB[1][i][j] = (int)rgb[1];
					mRGB[2][i][j] = (int)rgb[2];
				}
		}
		else							// ASCII
		{
			if ( mVerbosity ) cerr << "RGB ASCII PPM format]";
			for ( i = 0; i < mHeight; i++ )
				for ( j = 0; j < mWidth; j++ )
				{
					imgFile >> mRGB[0][i][j];
					imgFile >> mRGB[1][i][j];
					imgFile >> mRGB[2][i][j];
				}
		}
	}
	else if ( mMagicNumber[1] == '4' || mMagicNumber[1] == '1' ) // Binary
	{
	// allocate pixel storage
		Allocate( kChars );

		if ( mMagicNumber[1] == '4' )		// RAWBITs
		{
			if ( mVerbosity ) cerr << "Binary RAWBITs PBM format]";
			for ( i = 0; i < mHeight; i++ )
				for ( j = 0; j < mWidth; j += 8 )
				{
					char pix[1];
					imgFile.read( pix, 1 );
					unsigned int x = (unsigned int)pix[0];
					for ( int k = 0; k < 8; k++ )
					{
						unsigned int y = x/( (unsigned int)pow( 2, (double)(7-k) ) );
						if ( y )
							mPixels[i][j+k] = 0;
						else
							mPixels[i][j+k] = 255;
						x -= y*( (unsigned int)pow( 2, (double)(7-k) ) );
					}
				}
		}
		else
		{
			if ( mVerbosity ) cerr << "Binary ASCII PBM format]";
			for ( i = 0; i < mHeight; i++ )
				for ( j = 0; j < mWidth; j++ )
				{
					int pix;
					imgFile >> pix;
					if ( pix == 0 )
						mPixels[i][j] = 0;
					else
						mPixels[i][j] = 255;
				}
		}
	}

	imgFile.close();

	if ( mVerbosity )
	{
		cerr << endl;
		cerr << "\twidth = " << mWidth << endl;
		cerr << "\theight = " << mHeight << endl;
		cerr << "\t#pixels = " << mNumPixels << endl;
	}
	mNumLevels = 255;
	mNumBits = 8;
	mMagicNumber[1] = '5';

	return 1;	// No error encountered....successful completion
}


// write PGM image to file
void PGMImage::Write( char* file )
{
	ofstream outfile( file );

	if ( mVerbosity )
	{
		//cerr << "writing " << mWidth << "x" << mHeight;
		//cerr << " image to file1 \"" << file << "\"...";
	}
	outfile << mMagicNumber[0] << mMagicNumber[1] << endl;  // 8-bit grayscale
	outfile << "# grayscale image" << endl;
	outfile << mWidth << " " << mHeight << endl;
	outfile << mNumLevels << endl;

	for ( int i = 0; i < mHeight; i++ )
		outfile.write( (char *)mPixels[i], mWidth );

	outfile.close();

	//if ( mVerbosity ) cerr << "done" << endl;
}


// Write a PGM image in a file from output as is
void PGMImage::Write( char* filename, float** output, int height, int width )
{
	int i, j;

	// clear old data first
	Deallocate();
	
	// set dimensions of pixelmap and allocate memory
    mWidth = width;
    mHeight = height;
    mMagicNumber[0] = 'P'; 
    mMagicNumber[1] = '5';
    mNumLevels = 255;
	Allocate( kChars );

	for( i = 0; i < mHeight; i++ )
		for( j = 0; j < mWidth; j++ )
			mPixels[i][j] = (unsigned char)output[i][j];

    Write( filename );
}


// Write a color PPM image in a file
void PGMImage::Write( char* filename, float*** pixels, int height, int width )
{
	ofstream		outfile( filename );
	unsigned char	rgb[3];
	
	// set dimensions of pixelmap and allocate memory
	//if ( mVerbosity ) cerr << "writing " << width << "x" << height;
	//if ( mVerbosity ) cerr << " image to file2 \"" << filename << "\"...";
	outfile << "P6" << endl;  // 8-bit color
	outfile << "# color image" << endl;
	outfile << width << " " << height << endl;
	outfile << 255 << endl;

	for ( int i = 0; i < height; i++ )
		for ( int j = 0; j < width; j++ )
		{
			rgb[0] = (unsigned char)(pixels[0][i][j]*255.0);
			rgb[1] = (unsigned char)(pixels[1][i][j]*255.0);
			rgb[2] = (unsigned char)(pixels[2][i][j]*255.0);
			outfile.write( (char *)rgb, 3 );
		}

	outfile.close();

	//if ( mVerbosity ) cerr << "done" << endl;
}


// Write a one-channel color PPM image in a file
void PGMImage::Write( char* filename, float** pixels, int height, int width, int channel )
{
	ofstream		outfile( filename );
	unsigned char	rgb[3];
    float			max, min, maxmin;
	int				i, j;
	
	// set dimensions of pixelmap and allocate memory
	//if ( mVerbosity ) cerr << "writing " << width << "x" << height;
	//if ( mVerbosity ) cerr << " image to file3 \"" << filename << "\"...";
	outfile << "P6" << endl;  // 8-bit color
	outfile << "# color image" << endl;
	outfile << width << " " << height << endl;
	outfile << 255 << endl;

	// original float values scaled to [0,255]
	max = min = pixels[0][0];
	for( i = 0; i < height; i++ )
		for( j = 0; j < width; j++ )
		{
			if( pixels[i][j] > max ) max = pixels[i][j];
			if( pixels[i][j] < min ) min = pixels[i][j];
		}
	maxmin = max - min;

	if ( channel == 0 )
		for ( i = 0; i < height; i++ )
		{
			for ( j = 0; j < width; j++ )
			{
				rgb[0] = (unsigned char)(255.0 * ( (pixels[i][j] - min ) / maxmin ));
				//(unsigned char)(pixels[i][j]*255.0);
				rgb[1] = 0;
				rgb[2] = 0;
				outfile.write( (char *)rgb, 3 );
			}
		}
	else if ( channel == 1 )
		for ( i = 0; i < height; i++ )
		{
			for ( j = 0; j < width; j++ )
			{
				rgb[0] = 0;
				rgb[1] = (unsigned char)(255.0 * ( (pixels[i][j] - min ) / maxmin ));
				rgb[2] = 0;
				outfile.write( (char *)rgb, 3 );
			}
		}
	else
		for ( i = 0; i < height; i++ )
		{
			for ( j = 0; j < width; j++ )
			{
				rgb[0] = 0;
				rgb[1] = 0;
				rgb[2] = (unsigned char)(255.0 * ( (pixels[i][j] - min ) / maxmin ));
				outfile.write( (char *)rgb, 3 );
			}
		}	

	outfile.close();

	//if ( mVerbosity ) cerr << "done" << endl;
}


// Write a PGM image in a file with output values scaled
void PGMImage::WriteScaled( char* filename, float** output, int height, int width )
{
    float	max, min, maxmin;
	int		i, j;

// clear old data first
	Deallocate();
	
// set dimensions of pixelmap and allocate memory
    mWidth = width;
    mHeight = height;
    mMagicNumber[0] = 'P';
    mMagicNumber[1] = '5';
    mNumLevels = 255;
	Allocate( kChars );

// original float values scaled to [0,255]
	max = min = output[0][0];
	for( i = 0; i < mHeight; i++ )
		for( j = 0; j < mWidth; j++ )
		{
			if( output[i][j] > max ) max = output[i][j];
			if( output[i][j] < min ) min = output[i][j];
		}

	maxmin = max - min;
	for( i = 0; i < mHeight; i++ )
		for( j = 0; j < mWidth; j++ )
			mPixels[i][j] = (unsigned char)(255.0 * ( (output[i][j] - min ) / maxmin ));

    Write( filename );
}

}; // namespace
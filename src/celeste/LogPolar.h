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
	Ported by:		Adriaan Tijsseling (AGT)
*/

#ifndef __LOGPOLAR_CLASS__
#define __LOGPOLAR_CLASS__

#include <cstring>
#include "GaborGlobal.h"
#include "PGMImage.h"

namespace celeste
{
class LogPolar
{
public:

    LogPolar(){ mCoords = NULL; mPolarized = NULL; }
    LogPolar( float** img, int height, int width, int minS, int ry = 30, int rx = 11 );
    ~LogPolar();

	void 		ApplyFilter( float** img, int height, int width );
	void 		Save( void );

	inline void		SetFileName( char* file ) { strcpy( mFile, file ); }
	inline float**	GetPolars( void ) { return mPolarized; }
	inline int		GetWidth() { return mWidth; }
	inline int		GetHeight(){ return mHeight; }

protected:

    float	**mCoords;		// logpolar coordinates img
    float	**mPolarized;	// result
    char	mFile[256];		// file name
    int		mMinHW;			// shortest size of image
    int		mHeight;		// height of filter
    int		mWidth;			// width of filter
    int		mImgHeight;		// height of output image
    int		mImgWidth;		// width of output image
};
}; //namespace
#endif

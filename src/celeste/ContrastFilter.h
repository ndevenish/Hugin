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

#ifndef __CONTRAST_FILTER_CLASS__
#define __CONTRAST_FILTER_CLASS__

#include <cstring>
#include "PGMImage.h"

namespace celeste
{

class ContrastFilter
{
public:

    ContrastFilter(){ mContrast = NULL; }
    ContrastFilter( float**, int, int );
    ~ContrastFilter();

	void 		ApplyFilter( float** img, int height, int width );
	void 		Save( void );

	inline void		SetFileName( char* file ) { strcpy( mFile, file ); }
	inline float**	GetContrast( void ) { return mContrast; }
	inline int		GetWidth() { return mWidth; }
	inline int		GetHeight(){ return mHeight; }

protected:

    float	**mContrast;	// applied contrast
    char	mFile[256];		// file name
    int		mHeight;		// height of filter
    int		mWidth;			// width of filter
};
}; // namespace
#endif

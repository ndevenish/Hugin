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
Description:	Class definition for a Gabor Jet
Author:		Adriaan Tijsseling (AGT)
Copyright: 	(c) Copyright 2002 Adriaan Tijsseling. All rights reserved.
Change History (most recent first):
18/04/2002 - AGT - initial version
*/

#ifndef __GABORJET__
#define __GABORJET__

#include <cstring>
#include "GaborGlobal.h"
#include "GaborFilter.h"

namespace celeste
{
class GaborJet
{
public:

	GaborJet();
	~GaborJet();
	
	void	Initialize( int y, int x, int x0, int y0, int r, float s = 2.0, int f = 2, 
						float maxF = 2, float minF = 1, int a = 8, char* file=NULL);

	void	Filter( float** image, int* len );
	float	GetResponse( int idx ) { return mFiducials[idx]; }

protected:

	int				mHeight;	// vertical size of image
	int				mWidth;		// horizontal size of image
	int				mX;			// origin of Gabor Jet
	int				mY;
	int				mAngles;	// number of orientations
	int				mFreqs;		// number of frequencies
	int				mRadius;	// radius of filter
	GaborFilter**	mFilters;	// set of filters in use
	float*			mFiducials;	// vector with Gabor responses at center
};
} //namespace
#endif


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
	Description:	Class definition for a single Gabor Filter
	Author:		Adriaan Tijsseling (AGT)
	Copyright: 	(c) Copyright 2002-3 Adriaan Tijsseling. All rights reserved.
*/


#ifndef __GABORFILTER__
#define __GABORFILTER__

#include "GaborGlobal.h"
#include "PGMImage.h"

namespace celeste
{
class GaborFilter
{
public:

	GaborFilter();
	~GaborFilter();
	
	void	Initialize( int radius, float a, float f, float s, float p = 0 );
	void	Save( char* file, int angle, int freq );

	inline float 	GetReal( int x, int y ) { return mReal[x][y]; }
	inline float 	GetImaginary( int x, int y ) { return mImaginary[x][y]; }
	
protected:

	int			mXYO;			// origin
	int			mRadius;		// radius of filter
	float		mSigma;			// curve of gaussian (sually set to PI)
	float		mAngle;			// orientation of filter (theta)
	float		mPhase;			// phase of filter (rho)
	float		mFrequency;		// wavelengths of filter (omega)
	float**		mReal;			// real part of filter
	float**		mImaginary;		// imaginary part of filter
};
} //namespace
#endif

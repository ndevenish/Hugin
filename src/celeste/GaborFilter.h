/*
	Description:	Class definition for a single Gabor Filter
	Author:		Adriaan Tijsseling (AGT)
	Copyright: 	(c) Copyright 2002-3 Adriaan Tijsseling. All rights reserved.
*/


#ifndef __GABORFILTER__
#define __GABORFILTER__

#include "GaborGlobal.h"
#include "PGMImage.h"

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

#endif

/*
Description:	Class definition for a Gabor Jet
Author:		Adriaan Tijsseling (AGT)
Copyright: 	(c) Copyright 2002 Adriaan Tijsseling. All rights reserved.
Change History (most recent first):
18/04/2002 - AGT - initial version
*/

#ifndef __GABORJET__
#define __GABORJET__

#include "GaborGlobal.h"
#include "GaborFilter.h"


class GaborJet
{
public:

	GaborJet();
	~GaborJet();
	
	void	Initialize( int y, int x, int x0, int y0, int r, float s = 2.0, int f = 2, 
						float maxF = 2, float minF = 1, int a = 8, bool save = false );
	void	Filter( float** image, int* len );
	float	GetResponse( int idx ) { return mFiducials[idx]; }

	inline void		SetFileName( char* file ) { strcpy( mFile, file ); }
	
protected:

	bool			mShowFilter;// indicates whether to save images of used filters
	int				mHeight;	// vertical size of image
	int				mWidth;		// horizontal size of image
	int				mX;			// origin of Gabor Jet
	int				mY;
	float			mSigma;		// modulator for standard deviation sigma
	int				mAngles;	// number of orientations
	int				mFreqs;		// number of frequencies
	int				mRadius;	// radius of filter
	float			mMinFreq;	// minimum frequency
	float			mMaxFreq;	// maximum frequency
	GaborFilter**	mFilters;	// set of filters in use
	float*			mFiducials;	// vector with Gabor responses at center
	char			mFile[256];	// filename
};

#endif


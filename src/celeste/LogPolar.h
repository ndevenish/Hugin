/*
	Description:		class for log polar filter
	Original Author:	Takio Kurita
	Ported by:		Adriaan Tijsseling (AGT)
*/

#ifndef __LOGPOLAR_CLASS__
#define __LOGPOLAR_CLASS__

#include "GaborGlobal.h"
#include "PGMImage.h"

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

#endif

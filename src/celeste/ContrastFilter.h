/*
	Description:		Abstract class for contrast filter
	Original Author:	Yasunobu Honma
	Modifications by:	Adriaan Tijsseling (AGT)
*/

#ifndef __CONTRAST_FILTER_CLASS__
#define __CONTRAST_FILTER_CLASS__

#include "PGMImage.h"

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

#endif

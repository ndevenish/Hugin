/* Panorama_Tools	-	Generate, Edit and Convert Panoramic Images
   Copyright (C) 1998,1999 - Helmut Dersch  der@fh-furtwangen.de
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

/*------------------------------------------------------------*/

#ifndef FILTER_H
#define FILTER_H


#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include "panorama.h"

#ifndef TRUE
	#define TRUE 1
#endif

#ifndef FALSE
	#define FALSE 0
#endif


//---------------------- Types ---------------------------------------------

#define UCHAR	unsigned char
#define USHORT  unsigned short
#define ULONG   unsigned long

enum{
	_UCHAR,
	_USHORT,
	_ULONG
	};

//---------------------- Some useful math defines --------------------------

#ifndef PI
	#define PI 3.14159265358979323846
#endif

// Normalize an angle to +/-180degrees

#define NORM_ANGLE( x )  while( x >180.0 ) x -= 360.0; while( x < -180.0 ) x += 360.0;

// Convert degree to radian

#define DEG_TO_RAD( x )		( (x) * 2.0 * PI / 360.0 )

// and reverse

#define RAD_TO_DEG( x )		( (x) * 360.0 / ( 2.0 * PI ) )

// Convert double x to unsigned char/short c



#define	DBL_TO_UC( c, x )	if(x>255.0) c=255U;								\
								else if(x<0.0) c=0;							\
								else c=(unsigned char)floor(x+0.5);

#define	DBL_TO_US( c, x )	if(x>65535.0) c=65535U;							\
								else if(x<0.0) c=0;							\
								else c=(unsigned short)floor(x+0.5);







// A large rectangle

typedef struct{
	long	top;
	long	bottom;
	long	left;
	long	right;
	}	PTRect;


struct PTPoint
{
	double x;
	double y;
};

typedef struct PTPoint PTPoint;

#define CopyPTPoint( to, from )       memcpy( &to, &from, sizeof( PTPoint ))
#define SamePTPoint( p, s )			  ((p).x == (s).x && (p).y == (s).y)

struct PTLine
{
	PTPoint v[2];
};

typedef struct PTLine PTLine;


struct PTTriangle
{
	PTPoint v[3];
};

typedef struct PTTriangle PTTriangle;




// Maximum number of controlpoints in a pair of images, which can be read
// via Barcodes

#define NUMPTS 21

// Randomization of feather in stitching tools

#define	BLEND_RANDOMIZE		0.1




//----------------------- Structures -------------------------------------------

struct remap_Prefs{								// Preferences Structure for remap
		long    		magic;					//  File validity check, must be 30
		int				from;					// Image format source image
		int				to;						// Image format destination image
		double			hfov;					// horizontal field of view /in degrees
		double			vfov;					// vertical field of view (usually ignored)
		} ;

typedef struct remap_Prefs rPrefs;

struct perspective_Prefs{						//  Preferences structure for tool perspective
		long			magic;					//  File validity check, must be 40
		int				format;					//  rectilinear or fisheye?
		double  		hfov;					//  Horizontal field of view (in degree)
		double			x_alpha;				//  New viewing direction (x coordinate or angle)
		double 			y_beta;					//  New viewing direction (y coordinate or angle)
		double			gamma;					//  Angle of rotation
		int				unit_is_cart;			//  true, if viewing direction is specified in coordinates
		int				width;					//  new width
		int				height;					//  new height
		} ;
		
typedef struct perspective_Prefs pPrefs;


struct optVars{									//  Indicate to optimizer which variables to optimize
		int hfov;								//  optimize hfov? 0-no 1-yes , etc
		int yaw;				
		int pitch;				
		int roll;				
		int a;
		int b;
		int c;					
		int d;
		int e;
		};
		
typedef struct optVars optVars;


enum{										// Enumerates for stBuf.seam
	_middle,								// seam is placed in the middle of the overlap
	_dest									// seam is places at the edge of the image to be inserted
	};

enum{										// Enumerates for colcorrect
	_colCorrectImage 	= 1,
	_colCorrectBuffer	= 2,
	_colCorrectBoth		= 3,
	};

struct stitchBuffer{						// Used describe how images should be merged
	char				srcName[256];		// Buffer should be merged to image; 0 if not.
	char				destName[256];		// Converted image (ie pano) should be saved to buffer; 0 if not
	int					feather;			// Width of feather
	int					colcorrect;			// Should the images be color corrected?
	int					seam;				// Where to put the seam (see above)
	};

typedef struct stitchBuffer stBuf;

struct panControls{							// Structure for realtime Panoeditor
		double panAngle;					// The amount by which yaw/pitch are changed per click
		double zoomFactor;					// The percentage for zoom in/out
		};
		
		
typedef struct panControls panControls;



enum{										// Enumerates for aPrefs.mode
		_readControlPoints,
		_runOptimizer,
		_insert,
		_extract,
		_useScript = 8,						// else use options
	};

struct adjust_Prefs{						//	Preferences structure for tool adjust
		long			magic;				//	File validity check, must be 50
		long			mode;				//  What to do: create Panorama etc?
		Image			im;					//  Image to be inserted/extracted
		Image			pano;				//  Panorama to be created/ used for extraction
		
		stBuf			sBuf;
		fullPath		scriptFile;	// On Mac: Cast to FSSpec; else: full path to scriptFile
		};
		
		
typedef struct adjust_Prefs aPrefs;
		


union panoPrefs{
		cPrefs	cP;
		pPrefs	pP;
		rPrefs	rP;
		aPrefs	aP;
		panControls pc;
		};
		
typedef union panoPrefs panoPrefs;


struct size_Prefs{								// Preferences structure for 'pref' dialog
		long			magic;					//  File validity check; must be 70
		int				displayPart;			// Display cropped/framed image ?
		int				saveFile;				// Save to tempfile? 0-no, 1-yes
		fullPath		sFile;					// Full path to file (short name)
		int				launchApp;				// Open sFile ?
		fullPath		lApp;					// the Application to launch
		int				interpolator;			// Which interpolator to use 
		double			gamma;					// Gamma correction value
		int				noAlpha;				// If new file is created: Don't save mask (Photoshop LE)
		int				optCreatePano;			// Optimizer creates panos? 0  no/ 1 yes
		} ;

typedef struct size_Prefs sPrefs;
		
		

#if 0
struct controlPoint{							// Control Points to adjust images
		int  num[2];							// Indices of Images 
		int	 x[2];								// x - Coordinates 
		int  y[2];								// y - Coordinates 
		int  type;								// What to optimize: 0-r, 1-x, 2-y
		} ;
#endif
struct controlPoint{							// Control Points to adjust images
		int  num[2];							// Indices of Images 
		double x[2];								// x - Coordinates 
		double y[2];								// y - Coordinates 
		int  type;								// What to optimize: 0-r, 1-x, 2-y
		} ;

typedef struct controlPoint controlPoint;

struct CoordInfo{								// Real World 3D coordinates
		int  num;								// auxilliary index
		double x[3];
		int  set[3];
		};
		
typedef struct CoordInfo CoordInfo;

// Some useful macros for vectors

#define SCALAR_PRODUCT( v1, v2 )	( (v1)->x[0]*(v2)->x[0] + (v1)->x[1]*(v2)->x[1] + (v1)->x[2]*(v2)->x[2] ) 
#define ABS_SQUARED( v )			SCALAR_PRODUCT( v, v )
#define ABS_VECTOR( v )				sqrt( ABS_SQUARED( v ) )
#define CROSS_PRODUCT( v1, v2, r )  { (r)->x[0] = (v1)->x[1] * (v2)->x[2] - (v1)->x[2]*(v2)->x[1];  \
									  (r)->x[1] = (v1)->x[2] * (v2)->x[0] - (v1)->x[0]*(v2)->x[2];	\
									  (r)->x[2] = (v1)->x[0] * (v2)->x[1] - (v1)->x[1]*(v2)->x[0]; }
#define DIFF_VECTOR( v1, v2, r )  	{ 	(r)->x[0] = (v1)->x[0] - (v2)->x[0];  \
									  	(r)->x[1] = (v1)->x[1] - (v2)->x[1];  \
									  	(r)->x[2] = (v1)->x[2] - (v2)->x[2]; }
#define DIST_VECTOR( v1, v2 )		sqrt( ((v1)->x[0] - (v2)->x[0]) * ((v1)->x[0] - (v2)->x[0]) + \
										  ((v1)->x[1] - (v2)->x[1]) * ((v1)->x[1] - (v2)->x[1]) + \
										  ((v1)->x[2] - (v2)->x[2]) * ((v1)->x[2] - (v2)->x[2]) )

struct transformCoord{							// 
		int nump;								// Number of p-coordinates
		CoordInfo  *p;							// Coordinates "as is"
		int numr;								// Number of r-coordinates
		CoordInfo  *r;							// Requested values for coordinates
		} ;
	
typedef struct transformCoord transformCoord;

struct  tMatrix{
		double alpha;
		double beta;
		double gamma;
		double x_shift[3];
		double scale;
		};
		
typedef struct tMatrix tMatrix;

		
		
	
	


struct MakeParams{								// Actual parameters used by Xform functions for pano-creation
	double 	scale[2];							// scaling factors for resize;
	double 	shear[2];							// shear values
	double  rot[2];								// horizontal rotation params
	void	*perspect[2];						// Parameters for perspective control functions
	double	rad[6];								// coefficients for polynomial correction (0,...3) and source width/2 (4) and correction radius (5)	
	double	mt[3][3];							// Matrix
	double  distance;
	double	horizontal;
	double	vertical;
	};

struct LMStruct{								// Parameters used by the Levenberg Marquardt-Solver
	int			m;								
	int			n;
	double 		*x;
	double 		*fvec;
	double 		ftol;
	double 		xtol;
	double 		gtol;
	int 		maxfev; 
	double 		epsfcn;
	double 		*diag;
	int 		mode;	
	double 		factor;
	int			nprint;
	int			info;
	int			nfev;
	double 		*fjac;
	int			ldfjac;
	int 		*ipvt;
	double 		*qtf;
	double 		*wa1;
	double 		*wa2;
	double 		*wa3;
	double 		*wa4;
	};

// function to minimize in Levenberg-Marquardt solver

typedef		int (*lmfunc)();	

struct triangle
{
	int vert[3];	// Three vertices from list
	int nIm;		// number of image for texture mapping
};

typedef struct triangle triangle;




struct AlignInfo{							// Global data structure used by alignment optimization
	Image 				*im;				// Array of Pointers to Image Structs
	optVars				*opt;				// Mark variables to optimize
	int					numIm;				// Number of images 
	controlPoint 		*cpt;				// List of Control points
	triangle			*t;					// List of triangular faces
	int					nt;					// Number of triangular faces
	int     			numPts;				// Number of Control Points
	int					numParam;			// Number of parameters to optimize
	Image				pano;				// Panoramic Image decription
	stBuf				st;					// Info on how to stitch the panorama
	void				*data;
	lmfunc				fcn;
	sPrefs				sP;	
	CoordInfo			*cim;				// Real World coordinates
	};  

typedef struct AlignInfo AlignInfo;

struct OptInfo{
	int numVars;					// Number of variables to fit
	int numData;					// Number of data to fit to
	int (*SetVarsToX)(double *x);	// Translate variables to x-values
	int (*SetXToVars)(double *x);	// and reverse
	lmfunc fcn;						// Levenberg Marquardt function measuring quality
	char message[256];				// info returned by LM-optimizer
	};
	
typedef struct OptInfo OptInfo;



struct VRPanoOptions
{
	int			width;
	int			height;
	double 		pan;
	double 		tilt;
	double 		fov;
	int 		codec;
	int 		cquality;
	int			progressive;
};

typedef struct VRPanoOptions VRPanoOptions;


struct MultiLayerImage
{
	Image	im;
	int		numLayers;
	Image	*Layer;
	PTRect	*LayerRect;
};

typedef struct MultiLayerImage MultiLayerImage;


	
	


// Transformation function type (we have only one...)

typedef		void (*trfn)( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );


// Function descriptor to be executed by exec_function
struct fDesc {
	trfn	func;			// The function to be called
	void	*param;			// The parameters to be used
	};		

typedef struct fDesc fDesc;

#define SetDesc(fD,f,p)		fD.func = f; fD.param = p

// Panorama tool type

typedef		void (*fnPtr)(TrformStr *TrPtr);


// Filter function type

typedef unsigned char (*flfn)( unsigned char srcPixel, int xc, int yc, void *params );


// Interpolating functions for resampler

typedef		void (*intFunc)( unsigned char *dst, 	unsigned char **rgb,
							register double Dx, 
							register double Dy,
							int color, int SamplesPerPixel);





// Gamma Correction

struct PTGamma{
	double *DeGamma;
	unsigned short *Gamma;
	int		ChannelSize;
	int 	ChannelStretch;
	int		GammaSize;
	};

typedef struct PTGamma PTGamma;

extern PTGamma glu;


// Some macros to find out more about images

#define GetBitsPerChannel( im, x )		switch( (im)->bitsPerPixel )	\
									{									\
										case 24:	x =  8; break;		\
										case 32: 	x =  8; break;		\
										case 48: 	x = 16; break;		\
										case 64: 	x = 16; break;		\
										default: 	x =  8; break;		\
									}												

#define GetChannels( im, x )		switch( (im)->bitsPerPixel )		\
									{									\
										case 24:	x =  3; break;		\
										case 32: 	x =  4; break;		\
										case 48: 	x =  3; break;		\
										case 64: 	x =  4; break;		\
										default: 	x =  3; break;		\
									}												

									

//---------------------------------- Functions identical in all platforms ------------------------


void 	dispatch 	(TrformStr *TrPtr, sPrefs *s);	   // Entry into platform independent code
void 	DoTransForm	(TrformStr *TrPtr, panoPrefs *p );

void setLibToResFile  ( void );			// MacOS: Get resources from shared lib
void unsetLibToResFile( void );			// MacOS: Don't get resources from shared lib

enum{					// Enumerates used by Progress and infoDlg
	_initProgress,   	// display message "argument"
	_setProgress,		// display progress (argument is percentage converted to string)
	_disposeProgress,	// dispose progress indicator
	_idleProgress		// do nothing; on Mac: call waitnextevent;
	};

int 	Progress( int command, char* argument );	// Progress Reporting 
int 	infoDlg ( int command, char* argument );	// Display info: same argumenmts as progress
void  	PrintError( char* fmt, ...);				// Error Reporting

int 	ccommand( char ***argvPtr);					// Shell for standalone programs


//  Panorama Tool functions


void 	perspective	(TrformStr *TrPtr, pPrefs *p); 	
void 	correct		(TrformStr *TrPtr, cPrefs *c);  
void 	remap		(TrformStr *TrPtr, rPrefs *r); 
void 	adjust		(TrformStr *TrPtr, aPrefs *a); 
void 	pan			(TrformStr *TrPtr, panControls *pc);




// Set Struct defaults

void    SetPrefDefaults			(panoPrefs *prPtr,  int selector);
void 	SetCorrectDefaults		( cPrefs *p );
void 	SetAdjustDefaults		( aPrefs *p );
void 	SetRemapDefaults		( rPrefs *p );
void 	SetPerspectiveDefaults	( pPrefs *p );
void 	SetImageDefaults		( Image *im);
void	SetOptDefaults			( optVars *opt );
void	SetPanDefaults			( panControls *pc);
void 	SetSizeDefaults			( sPrefs *pref);
void	SetStitchDefaults		( stBuf *sbuf);
void	SetVRPanoOptionsDefaults( VRPanoOptions *v);
void 	SettMatrixDefaults		( tMatrix *t );
void 	SetCoordDefaults		( CoordInfo *c, int num);

int		SetAlignParams			( double *x );
int 	SetLMParams				( double *x );
void 	SetGlobalPtr			( AlignInfo *p );



// Dialogs
int 	SetPrefs			( panoPrefs *p );
int		SetPanPrefs			( panControls *p );
int 	SetCorrectPrefs		( cPrefs *p );
int 	SetRadialOptions	( cPrefs *p );
int 	SetHorizontalOptions( cPrefs *p );
int 	SetVerticalOptions	( cPrefs *p );
int 	SetShearOptions		( cPrefs *p );
int 	SetScaleOptions		( cPrefs *p );
int 	SetLumOptions		( cPrefs *p );
int 	setSizePrefs		( sPrefs *p, int can_resize );
int 	SetRemapPrefs		( rPrefs *p );
int 	SetPerspectivePrefs	( pPrefs *p );
int 	SetAdjustPrefs		( aPrefs *p );
int 	SetInterpolator		( sPrefs *p );
int 	SetCreateOptions	( aPrefs *p );
int 	SetCutOptions		( cPrefs *p );
int 	SetFourierOptions	( cPrefs *p );



// File I/O

int 	readPrefs			(char* p, int selector );   			// Preferences, same selector as dispatch
void 	writePrefs			(char* p, int selector );   			// Preferences, same selector as dispatch

int		LoadBufImage		( Image *image, char *fname, int mode);
int		SaveBufImage		( Image *image, char *fname );
int		writeTIFF			( Image *im, fullPath* fname);			// On Mac: fname is FSSpec*				
void 	SaveOptions			( struct correct_Prefs * thePrefs );
int 	LoadOptions			( struct correct_Prefs * thePrefs );
void  	FindScript			( struct adjust_Prefs *thePrefs );
char* 	LoadScript			( fullPath* scriptFile  );
int 	WriteScript			( char* res, fullPath* scriptFile, int launch );
int 	writePSD			( Image *im, fullPath* fname);			// On Mac: fname is FSSpec*	
int 	readPSD				( Image *im, fullPath* fname, int mode);
int 	FindFile			( fullPath *fname );
int 	SaveFileAs			( fullPath *fname, char *prompt, char *name );
void 	ConvFileName		( fullPath *fname,char *string);
int 	writePSDwithLayer	( Image *im, fullPath *fname);
int 	addLayerToFile		( Image *im, fullPath* sfile, fullPath* dfile, stBuf *sB);
void 	showScript			( fullPath* scriptFile );
void 	MakeTempName		( fullPath *fspec, char *fname );
void 	makePathForResult	( fullPath *path );
int 	makePathToHost 		( fullPath *path );
void    open_selection		( fullPath *path );
int 	readPSDMultiLayerImage( MultiLayerImage *mim, fullPath* sfile);
int 	GetFullPath 		(fullPath *path, char *filename); // Somewhat confusing, for compatibility easons
int 	StringtoFullPath	(fullPath *path, char *filename);
int 	IsTextFile			( char* fname );
int 	readPositions		( char* script, transformCoord *tP );
int 	readImage			( Image *im, fullPath *sfile );
int 	writeImage			( Image *im, fullPath *sfile );
int 	writeJPEG			( Image *im, fullPath *sfile, 	int quality, int progressive );
int 	makeTempPath		( fullPath *path );
int 	writePNG			( Image *im, fullPath *sfile );
int 	readPNG				( Image *im, fullPath *sfile );

#define FullPathtoString( path, string ) 		GetFullPath( path, string)




// Image manipulation

void 	addAlpha			( Image *im ); 
void 	transForm			( TrformStr *TrPtr, fDesc *fD, int color);
void    filter				( TrformStr *TrPtr, flfn func, void* params, int color);		
void 	CopyImageData		( Image *dest, Image *src );
void 	laplace				( Image *im );
void 	blurr				( Image *im );
void 	MakePano			( TrformStr *TrPtr, aPrefs *aP, int nt, PTTriangle *ts,  PTTriangle *td);
void 	ExtractStill		( TrformStr *TrPtr , aPrefs *p );
int 	HaveEqualSize		( Image *im1, Image *im2 );
int 	merge				( Image *dst, Image *src, int feather, int showprogress, int seam );
void 	mergeAlpha			( Image *im, unsigned char *alpha, int feather, PTRect *theRect );
void 	SetEquColor			( cPrefs *p );
void 	CopyPosition		( Image *to, Image *from );
int  	isColorSpecific		( cPrefs *p );
void 	ThreeToFourBPP		( Image *im );
void 	FourToThreeBPP		( Image *im );
int 	SetUpGamma			( double pgamma, int psize);
int 	cutTheFrame			( Image *dest, Image *src, int width, int height, int showprogress );
int 	PositionCmp			( Image *im1, Image *im2 );
int 	MorphImage			( Image *src, Image *dst, PTTriangle *ts, PTTriangle *td, int nt );
int 	MorphImageFile		( fullPath *sfile, fullPath *dfile, AlignInfo *g,int nIm );
int 	blendImages			( fullPath *f0,  fullPath *f1, fullPath *result, double s );
int 	InterpolateImage	( Image *src, Image *dst, PTTriangle *ts, PTTriangle *td, int nt );
int 	InterpolateTrianglesPerspective( AlignInfo *g, int nIm, double s, PTTriangle** t  );
int 	InterpolateImageFile( fullPath *sfile, fullPath *dfile, AlignInfo *g,int nIm );
void 	OneToTwoByte		( Image *im );
void 	TwoToOneByte		( Image *im );
void 	SetMakeParams		( struct fDesc *stack, struct MakeParams *mp, Image *im , Image *pn, int color );
void 	SetInvMakeParams	( struct fDesc *stack, struct MakeParams *mp, Image *im , Image *pn, int color );
void 	GetControlPointCoordinates(int i, double *x, double *y, AlignInfo *gl );


// Script Reading/Parsing/Writing

int 	ParseScript			( char* script, AlignInfo *gl );
void 	WriteResults		( char* script, fullPath *sfile, AlignInfo *g, double ds( int i) , int launch);
int 	readAdjust			( aPrefs *p,  fullPath* sfile , int insert);
void 	readControlPoints	(char* script, controlPoint *c );
int		getVRPanoOptions	( VRPanoOptions *v, char *line );
void 	nextWord			( register char* word, char** ch );
void 	nextLine			( register char* line, char** ch );
int 	numLines			( char* script, char first );

// Memory

void 	DisposeAlignInfo	( AlignInfo *g );
void**  mymalloc			( long numBytes );					// Memory allocation, use Handles
void 	myfree				( void** Hdl );						// free Memory, use Handles
int 	SetDestImage		( TrformStr *TrPtr, int width, int height) ;
void	DisposeMultiLayerImage( MultiLayerImage *mim );


// Math

void 	RunLMOptimizer		( OptInfo	*g);
void 	RunBROptimizer		( OptInfo	*g, double minStepWidth);
void 	RunOverlapOptimizer ( AlignInfo	*g);

void 	SetMatrix			( double a, double b, double c , double m[3][3], int cl );
void 	matrix_mult			( double m[3][3], double vector[3] );
void 	matrix_inv_mult		( double m[3][3], double vector[3] );
double 	smallestRoot		( double *p );
void 	SetCorrectionRadius	( cPrefs *cP );
int		lmdif				();
void	fourier				( TrformStr *TrPtr, cPrefs *cP );
unsigned short 	gamma_correct( double pix );
int 	EqualCPrefs( cPrefs *c1, cPrefs *c2 );
double 	OverlapRMS			( MultiLayerImage *mim );
double 	distSquared			( int num ); 
int		fcnPano();
void 	doCoordinateTransform( CoordInfo *c, tMatrix *t );
void 	findOptimumtMatrix( transformCoord *tP, tMatrix *tM, lmfunc f);
int 	SolveLinearEquation2( double a[2][2], double b[2], double x[2] );
void 	SortControlPoints( AlignInfo *g , int nIm);
void 	noisefilter			( Image *dest, Image *src );	
void 	fwiener				( TrformStr *TrPtr, Image *nf, Image *psf, double gamma, double frame );


// Triangulation
int 	PointInTriangle( double x, double y, PTTriangle *T, double c[2] );
int 	SetSourceTriangles( AlignInfo *g, int nIm, PTTriangle** t  );
int 	SetDestTriangles( AlignInfo *g, int nIm, PTTriangle** t  );
int 	InterpolateTriangles( AlignInfo *g, int nIm, double s, PTTriangle** t  );
int 	DelaunayIteration( AlignInfo *g, int nIm );
int 	PointInCircumcircle( double x, double y, PTTriangle *tC );
int 	TriangulatePoints( AlignInfo *g, int nIm );
int 	AddTriangle( triangle *t, AlignInfo *g );
int 	RemoveTriangle( int nt, AlignInfo *g );
void 	OrderVerticesInTriangle( int nt, AlignInfo *g );
void 	SetTriangleCoordinates( triangle *t, PTTriangle *tC, AlignInfo *g );
int 	TrianglesOverlap( PTTriangle *t0, PTTriangle *t1 );
int 	LinesIntersect( PTLine *s0, PTLine *s1) ; 
double 	PTDistance( PTPoint *s0, PTPoint *s1 );
int 	PTPointInRectangle(  PTPoint *p, PTLine *r );
int 	PTElementOf(  double x, double a, double b );
int 	PTNormal( double *a, double *b, double *c, PTLine *s );
int 	PTGetLineCrossing( PTLine *s0, PTLine *s1, PTPoint *ps );
int 	ReduceTriangles( AlignInfo *g, int nIm );
double 	PTAreaOfTriangle( PTTriangle *t );
int 	normalToTriangle( CoordInfo *n, CoordInfo *v, triangle *t );




double GetBlendfactor( int d, int s, int feather );





void execute_stack		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	

void resize				( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );		
void shear				( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void horiz				( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void vert				( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void radial				( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	


void persp_sphere		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void persp_rect			( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	


void rect_pano			( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void pano_rect			( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void pano_erect			( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void erect_pano			( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void sphere_cp_erect	( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void sphere_tp_erect	( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void erect_sphere_cp	( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void rect_sphere_tp		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void sphere_tp_rect		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void sphere_cp_pano		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void rect_erect			( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void erect_rect			( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void erect_sphere_tp	( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void mirror_erect		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	 
void mirror_sphere_cp	( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void mirror_pano		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void sphere_cp_mirror	( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );	
void sphere_tp_pano		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );

void pano_sphere_tp		( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );

void rotate_erect		( double x_dest, double y_dest, double* x_src, double* y_src, void* params );
void inv_radial			( double x_dest, double y_dest, double* x_src, double* y_src, void* params );

void vertical			( double x_dest, double y_dest, double* x_src, double* y_src, void* params );
void inv_vertical		( double x_dest, double y_dest, double* x_src, double* y_src, void* params );
void deregister			( double x_dest, double y_dest, double* x_src, double* y_src, void* params );
void tmorph				( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );



unsigned char radlum	( unsigned char srcPixel, int xc, int yc, void *params );


extern TrformStr 		*gTrPtr;
extern sPrefs			*gsPrPtr;




// Endian stuff: Read and write numbers from and to memory (ptr)

#ifdef BIGENDIAN
	#define	LONGNUMBER( number, ptr )					*ptr++ = ((char*)(&number))[0];	\
														*ptr++ = ((char*)(&number))[1];	\
														*ptr++ = ((char*)(&number))[2];	\
														*ptr++ = ((char*)(&number))[3];	

	#define NUMBERLONG( number, ptr )					((char*)(&number))[0] = *ptr++;	\
														((char*)(&number))[1] = *ptr++;	\
														((char*)(&number))[2] = *ptr++;	\
														((char*)(&number))[3] = *ptr++;	

	#define	SHORTNUMBER( number, ptr )					*ptr++ = ((char*)(&number))[0];	\
														*ptr++ = ((char*)(&number))[1];	\

	#define NUMBERSHORT( number, ptr )					((char*)(&number))[0] = *ptr++;	\
														((char*)(&number))[1] = *ptr++;	\

#else
	#define	LONGNUMBER( number, ptr )					*ptr++ = ((char*)(&number))[3];	\
														*ptr++ = ((char*)(&number))[2];	\
														*ptr++ = ((char*)(&number))[1];	\
														*ptr++ = ((char*)(&number))[0];	

	#define NUMBERLONG( number, ptr )					((char*)(&number))[3] = *ptr++;	\
														((char*)(&number))[2] = *ptr++;	\
														((char*)(&number))[1] = *ptr++;	\
														((char*)(&number))[0] = *ptr++;	

	#define	SHORTNUMBER( number, ptr )					*ptr++ = ((char*)(&number))[1];	\
														*ptr++ = ((char*)(&number))[0];	\

	#define NUMBERSHORT( number, ptr )					((char*)(&number))[1] = *ptr++;	\
														((char*)(&number))[0] = *ptr++;	\



#endif // BIGENDIAN

// Cross platform file functions

#ifdef __Mac__

	#include <Files.h>
	#include "sys_mac.h"
	
	#define			file_spec							short
	#define			myopen( path, perm, fspec )			( FSpOpenDF( path, perm, &fspec ) != noErr )
	#define			mywrite( fspec, count, data )		FSWrite	(fspec, &count, data) 
	#define 		myread(  fspec, count, data )		FSRead  (fspec, &count, data) 
	#define         myclose( fspec )					FSClose (fspec )
	#define			mycreate( path, creator, type )		FSpCreate( path, creator, type,0)
	#define			mydelete( path )					FSpDelete( path )
	#define			myrename( path, newpath )			FSpRename (path, (newpath)->name)
	#define			write_text							fsWrPerm
	#define			write_bin							fsWrPerm
	#define			read_text							fsRdPerm
	#define			read_bin							fsRdPerm
	#define			read_write_text						fsRdWrPerm
			
#else // __Mac__, use ANSI-filefunctions
	#define			file_spec							FILE*
	#define			myopen( path, perm, fspec )			( (fspec = fopen( (path)->name, perm )) == NULL)
	#define			mywrite( fspec, count, data )		count = fwrite( data, 1, count, fspec)
	#define 		myread( fspec, count, data )		count = fread( data, 1, count, fspec ) 
	#define         myclose( fspec )					fclose (fspec )
	#define			mycreate( path, creator, type )		
	#define			mydelete( path )					remove((path)->name )
	#define			myrename( path, newpath )			rename ((path)->name, (newpath)->name)
	#define			write_text							"w"
	#define			write_bin							"wb"
	#define			read_text							"r"
	#define			read_bin							"rb"
	#define			read_write_text						"rw"
	#define			p2cstr( x )	
	#define			c2pstr( x )
															

#endif




#endif







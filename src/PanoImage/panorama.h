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



#ifndef PANORAMA_H
#define PANORAMA_H


#include "version.h"


// Determine which machine we are using. Macs are set to BIGENDIAN, all others not

// If you need BIGENDIAN, and don't use MacOS, define it here:
// #define BIGENDIAN			1


// Create a definition if we're on a Windows machine:
#ifndef __Win__
	#if (defined(MSDOS) || defined(WIN32) || defined(__INTEL__))
		#define __Win__			1
	#endif
#endif


// Create a definition if we're on a Macintosh:
#ifndef __Mac__
	#if (defined(macintosh) || defined(__MC68K__) || defined(__POWERPC__) || defined(__powerc))
		#define __Mac__			1
		#define BIGENDIAN		1
	#endif
#endif



// Use FSSpec on Macs as Path-specifyers, else strings
#define PATH_SEP							'/'

#ifdef __Mac__

	#include <Files.h>
	#define			fullPath							FSSpec
	#undef  PATH_SEP
	#define PATH_SEP									':'

#else // __Mac__, use ANSI-filefunctions

	#ifdef __Win__
		// #include <windows.h>; including this causes problems with libjpeg
		#define MAX_PATH_LENGTH		260
		// was MAX_PATH
		#undef  PATH_SEP
		#define PATH_SEP							'\\'
	#else
		#define MAX_PATH_LENGTH		512
	#endif

	typedef	struct{char name[MAX_PATH_LENGTH];} fullPath;

#endif





// Enumerates for TrFormStr.tool

enum{							// Panorama Tools
		_perspective,					
		_correct,
		_remap,
		_adjust,
		_interpolate,
		_sizep,					// dummy for size-preferences
		_version,				// dummy for version
		_panright,				// Pan Controls
		_panleft,
		_panup,
		_pandown,
		_zoomin,
		_zoomout,
		_apply,
		_getPano,
		_increment
	};

// Enumerates for TrFormStr.mode

enum{							// Modes
		_interactive,			// display dialogs and do Xform
		_useprefs,				// load saved prefs and do Xform/ no dialogs
		_setprefs,				// display dialogs and set preferences, no Xform	
		_usedata,				// use supplied data in TrFormStr.data, do Xform
		_honor_valid 	= 8,	// Use only pixels with alpha channel set
		_show_progress 	= 16,   // Interpolator displays progress bar
		_hostCanResize 	= 32,	// o-no; 1-yes (Photoshop: no; GraphicConverter: yes)
		_destSupplied 	= 64,	// Destination image allocated by plug-in host
		_wrapX			=128	// Wrap image horizontally (if HFOV==360 degrees)
	};
		

// Enumerates for Image.dataformat

enum{							
		_RGB,					
		_Lab,
		_Grey
	};

// Enumerates for TrFormStr.interpolator

enum{							// Interpolators
		_poly3		= 0,		// Third order polynomial fitting 16 nearest pixels
		_spline16	= 1,		// Cubic Spline fitting 16 nearest pixels
		_spline36	= 2,		// Cubic Spline fitting 36 nearest pixels
		_sinc256	= 3,		// Sinc windowed to 8 pixels
		_spline64,				// Cubic Spline fitting 64 nearest pixels
		_bilinear,				// Bilinear interpolation
		_nn	,					// Nearest neighbor
		_sinc1024
	};

// Corrections

struct  correct_Prefs{							//  Preferences structure for tool correct
		unsigned long 	magic;					//  File validity check, must be 20
		int 			radial;					//  Radial correction requested?
		double			radial_params[3][5];	//  3 colors x (4 coeffic. for 3rd order polys + correction radius)
		int 			vertical;				//  Vertical shift requested ?
		double			vertical_params[3];		//  3 colors x vertical shift value
		int				horizontal;				//  horizontal tilt ( in screenpoints)
		double			horizontal_params[3];	//  3 colours x horizontal shift value
		int				shear;					//  shear correction requested?
		double			shear_x;				//  horizontal shear values
		double			shear_y;				//  vertical shear values
		int 			resize;					//  scaling requested ?
		long			width;					//  new width
		long			height;					//  new height
		int				luminance;				//  correct luminance variation?
		double			lum_params[3];			//  parameters for luminance corrections
		int				correction_mode;		//  0 - radial correction;1 - vertical correction;2 - deregistration
		int				cutFrame;				//  remove frame? 0 - no; 1 - yes
		int			    fwidth;
		int 			fheight;
		int				frame;
		int				fourier;				//  Fourier filtering requested?
		int				fourier_mode;			//  _faddBlurr vs _fremoveBlurr
		fullPath		psf;					//  Point Spread Function, full path/fsspec to psd-file
		int				fourier_nf;				//  Noise filtering: _nf_internal vs _nf_custom
		fullPath		nff;					//  noise filtered file: full path/fsspec to psd-file
		double			filterfactor;			//  Hunt factor
		double			fourier_frame;			//  To correct edge errors
		} ;

typedef struct correct_Prefs cPrefs;

enum{
	correction_mode_radial 		= 0,
	correction_mode_vertical 	= 1,
	correction_mode_deregister 	= 2,
	correction_mode_morph		= 4
	};
	

enum{
	_faddBlurr,
	_fremoveBlurr,
	_nf_internal,
	_nf_custom,
	_fresize
	};
	

enum{				// Enumerates for Image.format
	_rectilinear 	= 0,
	_panorama 		= 1,
	_fisheye_circ	= 2,
	_fisheye_ff		= 3,
	_equirectangular= 4,
	_spherical_cp	= 5,
	_spherical_tp	= 6,
	_mirror			= 7,
	_orthographic 	= 8
	};



struct Image
	{
		// Pixel data
		long 			width;
		long 			height;
		long 			bytesPerLine;
		long 			bitsPerPixel;	// Must be 24 or 32
		long 			dataSize; 
		unsigned char** data;
		long			dataformat;		// rgb, Lab etc
		long			format;			// Projection: rectilinear etc
		double			hfov;
		double			yaw;
		double			pitch;
		double			roll;
		cPrefs			cP;				// How to correct the image
		char			name[256];
	};
		
typedef struct Image Image;


struct TrformStr 				// This structure holds all image information
	{
		Image *src;				// Source image, must be supplied on entry
		Image *dest;			// Destination image data, valid if success = 1 
		long success; 			// 0 - no, 1 - yes 


		long tool;				// Panorama Tool requested
		long mode;				// how to run transformation
		void *data;				// data for tool requested.
								// Required only if mode = _usedata; then it
								// must point to valid preferences structure
								// for requested tool (see filter.h).

		long interpolator;		// Select interpolator
		double gamma;			// Gamma value for internal gamma correction
	};

typedef struct TrformStr TrformStr;	


// Useful for looping through images

#define LOOP_IMAGE( image, action )		{ 	int x,y,bpp=(image)->bitsPerPixel/8;						\
											unsigned char *idata;										\
											for(y=0; y<(image)->height; y++){							\
												idata = *((image)->data) + y * (image)->bytesPerLine;	\
												for(x=0; x<(image)->width;x++, idata+=bpp){				\
													action;} } }								
											
											
											
											
											

void filter_main();


#endif // PANORAMA_H

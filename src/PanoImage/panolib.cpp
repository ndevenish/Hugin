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
#include "filter.h"

#include <stdio.h>

// Lookup Tables for Trig-functions and interpolator

#define NATAN 2048
#define NSQRT 2048

int *atan_LU;
int *sqrt_LU;
int *mweights[256];


extern int xhi, xlo, yhi, ylo;

void 	matrix_matrix_mult	( double m1[3][3],double m2[3][3],double result[3][3]);
void 	PV_transForm( TrformStr *TrPtr, int dist_r, int dist_e, int mt[3][3]);
int 	PV_atan2(int y, int x);
int 	PV_sqrt( int x1, int x2 );


#define PV_ATAN2(r,y,x) 	\
	if( x > 0 )	\
	{	\
		if( y > 0 ) r = atan_LU[(int)( NATAN * y / ( x + y ))];	\
			else r = -atan_LU[ (int)(NATAN * (-y) / ( x - y ))];	\
	} else if( x == 0 ) {	\
		if( y > 0 ) r= (int)(256*NATAN*PI / 2.0);	\
			else r = -(int)(256*NATAN*PI / 2.0);	\
	} else if( y < 0 ) {	\
		r = atan_LU[(int)( NATAN * y / ( x + y ))] - (int)(PI*256*NATAN);	\
	} else {	\
		r = -atan_LU[ (int)(NATAN * (-y) / ( x - y ))] + (int)(PI*256*NATAN);	\
	}	\



// Bilinear interpolator

static void bil( unsigned char *dst, unsigned char **rgb,  
		int dx,int dy)
{		
	int yr, yg, yb,weight;						
	int rd, gd, bd ;								
	register unsigned char *r;
	int *w1, *w2;
	
	w1 = mweights[dx]; w2 = mweights[255 - dx];

	r = rgb[0];		
	
	rd = w2[*r++];	gd = w2[*r++];	bd = w2[*r++];
	//weight = 255 - dx; rd = weight * *r++; gd = weight * *r++;	bd = weight * *r++;

	rd += w1[*r++]; gd += w1[*r++];	bd += w1[*r];
	//rd += dx * *r++; gd += dx * *r++;	bd += dx * *r;

	r = rgb[1];		
		
	yr = w2[*r++]; yg = w2[*r++];	yb = w2[*r++];
	//rd = weight * *r++; gd = weight * *r++;	bd = weight * *r++;

	yr += w1[*r++]; yg += w1[*r++];	yb += w1[*r];
	//rd += dx * *r++; gd += dx * *r++;	bd += dx * *r;
	
	weight = 255 - dy;																		
	rd = rd * weight + yr * dy;	
	gd = gd * weight + yg * dy;	
	bd = bd * weight + yb * dy;

	*dst++ = rd >> 16;
	*dst++ = gd >> 16;
	*dst   = bd >> 16;
}



// Extract image from pano in TrPtr->src 
// using parameters in prefs (ignore image parameters
// in TrPtr)

void PV_ExtractStill( TrformStr *TrPtr )
{
	double		a,b;							// field of view in rad
	double      p[2];
	double		mt[3][3];
	int 		mi[3][3],i,k;

	TrPtr->success = 1;

	a =	 DEG_TO_RAD( TrPtr->dest->hfov );	// field of view in rad		
	b =	 DEG_TO_RAD( TrPtr->src->hfov );

	SetMatrix( 	DEG_TO_RAD( TrPtr->dest->pitch ), 
				DEG_TO_RAD( TrPtr->dest->yaw ), 
				0.0 , 
				mt, 
				1 );


	p[0] = (double)TrPtr->dest->width/ (2.0 * tan( a / 2.0 ) );
	p[1] = (double)TrPtr->src->width/ b;
	
	for(i=0; i<3; i++){
		for(k=0; k<3; k++){
			mi[i][k] = int(256 * mt[i][k]);
		}
	}

	PV_transForm( TrPtr,  (int)(p[0]+.5), (int)(p[1]+.5), mi);
	return;
}

	
	


//    Main transformation function. Destination image is calculated using transformation
//    Function "func". Either all colors (color = 0) or one of rgb (color =1,2,3) are
//    determined. If successful, TrPtr->success = 1. Memory for destination image
//    must have been allocated and locked!

void PV_transForm( TrformStr *TrPtr, int dist_r, int dist_e, int mt[3][3])
{
	register int 	x, y;									// Loop through destination image
	unsigned char 	*dest,*src,*sry, *dst;			// Source and destination image data
	int				dx,dy;
	int 				xc;									// Cartesian Coordinates of point ("target") in Destination image
	int 				xs, ys;	
	unsigned char	*rgb[2], cdata[12];				// Image data handed to sampler
	int				mix = TrPtr->src->width - 1;	// maximum x-index src
	int				miy = TrPtr->src->height - 1;	// maximum y-index src

	// Variables used to convert screen coordinates to cartesian coordinates
	int 				w2  = TrPtr->dest->width  / 2 ;  
	int 				h2  = TrPtr->dest->height / 2 ;
	int 				sw2 = TrPtr->src->width   / 2 ;
	int 				sh2 = TrPtr->src->height  / 2 ;
	
	int				BytesPerLine = TrPtr->src->bytesPerLine;
	int 				v[3];
	int				x_min, x_max, y_min, y_max;
	int				dr1, dr2, dr3;

	int a, b, c, d, e, f, i;
	unsigned char *ps;

	dr1 = mt[2][0] * dist_r;
	dr2 = mt[2][1] * dist_r;
	dr3 = mt[2][2] * dist_r;
	
	dest = *TrPtr->dest->data;
	src  = *TrPtr->src->data; // is locked

	x_min = -w2; x_max = TrPtr->dest->width - w2;
	y_min = -h2; y_max = TrPtr->dest->height - h2;

	d = mt[0][0];
	e = mt[0][1];
	f = mt[0][2];
	dst = dest;

	switch ( TrPtr->interpolator )
	{
		case _bilinear:
		{
			for(y = y_min; y < y_max; y++)
			{
				a = mt[1][0] * y + dr1;
				b = mt[1][1] * y + dr2;
				c = mt[1][2] * y + dr3;
				for(x = x_min; x < x_max; x++, dst+=3)
				{
					v[0] = (d * x + a) >> 8;
					v[1] = e * x + b;
					v[2] = (f * x + c) >> 8;

					PV_ATAN2(xs, v[0],v[2]);
					xs = dist_e * xs / NATAN ;
					ys = PV_sqrt( abs(v[2]), abs(v[0]) );
					PV_ATAN2(ys, v[1], ys );
					ys = dist_e * ys / NATAN ;

					dx = xs & 255; dy = ys & 255; // fraction
						
					xs = (xs >> 8) + sw2;
					ys = (ys >> 8) + sh2;

					// if all interpolation pixels inside image (true in most cases)
					if( ys >= 0 && ys < miy && xs >= 0 && xs < mix )
					{
						rgb[0] = src + ys * BytesPerLine + xs * 3;
						rgb[1] = rgb[0] + BytesPerLine;
					}
					else // edge pixels
					{
						xc = xs;

						rgb[0] = cdata;
						rgb[1] = cdata+6;

						if( ys < 0 ) sry = src;
							else if( ys > miy ) sry = src + miy * BytesPerLine;
								else sry = src + ys  * BytesPerLine;
				
						if( xs < 0 )  xs = mix;
						if( xs > mix) xs = 0;
						for(i = 0; i < 3; i++ ) cdata[i] = *(sry + xs*3 + i);

						xs = xc+1;
						if( xs < 0 )  xs = mix;
						if( xs > mix) xs = 0;
						for(i = 3; i < 6; i++ ) cdata[i] = *(sry + xs*3 + i);

						ys+=1;
						if( ys < 0 ) sry = src;
							else if( ys > miy ) sry = src + miy * BytesPerLine;
								else sry = src + ys  * BytesPerLine;

						xs = xc;
						if( xs < 0 )  xs = mix;
						if( xs > mix) xs = 0;
						for(i = 6; i < 9; i++ ) cdata[i] = *(sry + xs*3 + i);

						xs = xc+1;
						if( xs < 0 )  xs = mix;
						if( xs > mix) xs = 0;
						for(i = 9; i < 12; i++ ) cdata[i] = *(sry + xs*3 + i);
					}
					bil( dst, rgb, dx, dy ); 
				}
			}
			break;
		}
		case _nn:
		{
			d = mt[0][0];
			e = mt[0][1];
			f = mt[0][2];
			dst = dest;
		
			for(y = y_min; y < y_max; y++)
			{
				a = mt[1][0] * y + dr1;
				b = mt[1][1] * y + dr2;
				c = mt[1][2] * y + dr3;
				for(x = x_min; x < x_max; x++, dst+=3)
				{
					v[0] = (d * x + a) >> 8;
					v[1] = e * x + b;
					v[2] = (f * x + c) >> 8;
			
					PV_ATAN2(xs, v[0], v[2]);
					xs 	= sw2 + (dist_e * xs >> 19);
					ys = PV_sqrt( abs(v[2]), abs(v[0]) );
					PV_ATAN2(ys, v[1], ys );
					ys 	= sh2 + (dist_e * ys >> 19);

					if( xs < 0 ) 	xs = 0;
					if( xs > mix ) 	xs = mix;
					if( ys < 0) 	ys = 0;
					if( ys > miy ) 	ys = miy;

					ps = src + ys * BytesPerLine + xs * 3;
					for(i = 0; i < 3; i++)
						*(dst+i) = *(ps + i);
				}
			}
			break;
		}
	}

	TrPtr->success = 1;

	return;
}


void matrix_inv_mult( double m[3][3], double vector[3] )
{
	register int i;
	register double v0 = vector[0];
	register double v1 = vector[1];
	register double v2 = vector[2];
	
	for(i=0; i<3; i++)
	{
		vector[i] = m[0][i] * v0 + m[1][i] * v1 + m[2][i] * v2;
	}
}

// Set matrix elements based on Euler angles a, b, c

void SetMatrix( double a, double b, double c , double m[3][3], int cl )
{
	double mx[3][3], my[3][3], mz[3][3], dummy[3][3];
	

	// Calculate Matrices;

	mx[0][0] = 1.0 ; 				mx[0][1] = 0.0 ; 				mx[0][2] = 0.0;
	mx[1][0] = 0.0 ; 				mx[1][1] = cos(a) ; 			mx[1][2] = sin(a);
	mx[2][0] = 0.0 ;				mx[2][1] =-mx[1][2] ;			mx[2][2] = mx[1][1];
	
	my[0][0] = cos(b); 				my[0][1] = 0.0 ; 				my[0][2] =-sin(b);
	my[1][0] = 0.0 ; 				my[1][1] = 1.0 ; 				my[1][2] = 0.0;
	my[2][0] = -my[0][2];			my[2][1] = 0.0 ;				my[2][2] = my[0][0];
	
	mz[0][0] = cos(c) ; 			mz[0][1] = sin(c) ; 			mz[0][2] = 0.0;
	mz[1][0] =-mz[0][1] ; 			mz[1][1] = mz[0][0] ; 			mz[1][2] = 0.0;
	mz[2][0] = 0.0 ;				mz[2][1] = 0.0 ;				mz[2][2] = 1.0;

	if( cl )
		matrix_matrix_mult( mz,	mx,	dummy);
	else
		matrix_matrix_mult( mx,	mz,	dummy);
	matrix_matrix_mult( dummy, my, m);
}

void matrix_matrix_mult( double m1[3][3],double m2[3][3],double result[3][3])
{
	register int i,k;
	
	for(i=0;i<3;i++)
	{
		for(k=0; k<3; k++)
		{
			result[i][k] = m1[i][0] * m2[0][k] + m1[i][1] * m2[1][k] + m1[i][2] * m2[2][k];
		}
	}
}



inline int PV_atan2(int y, int x)
{
	// return atan2(y,x) * 256*NATAN;
	if( x > 0 )
	{
		if( y > 0 )
		{
			return  atan_LU[(int)( NATAN * y / ( x + y ))];
		}
		else
		{
			return -atan_LU[ (int)(NATAN * (-y) / ( x - y ))];
		}
	}

	if( x == 0 )
	{
		if( y > 0 )
			return  (int)(256*NATAN*PI / 2.0);
		else
			return  -(int)(256*NATAN*PI / 2.0);
	}
	
	if( y < 0 )
	{
		return  atan_LU[(int)( NATAN * y / ( x + y ))] - (int)(PI*256*NATAN);
	}
	else
	{
		return -atan_LU[ (int)(NATAN * (-y) / ( x - y ))] + (int)(PI*256*NATAN);
	}
	
}



int SetUpAtan()
{
	int i;
	double dz = 1.0 / (double)NATAN;
	double z = 0.0;
	
	atan_LU = (int*) malloc( (NATAN+1) * sizeof( int ));
	
	if( atan_LU == NULL )
		return -1;
		
	for( i=0; i< NATAN; i++, z+=dz )
		atan_LU[i] = int ( atan( z / (1.0 - z ) ) * NATAN * 256 );
		
	atan_LU[NATAN] = int ( PI/4.0 * NATAN * 256 );
	
	// Print a test
#if 0	
	for(i = -10; i< 10; i++)
	{
		int k;
		for(k=-10; k<10; k++)
		{
			printf("i =  %d  k = %d   atan2(i,k) = %g    LUatan(i,k) = %g diff = %g\n", i,k,atan2(i,k), 
				(double)PV_atan2(i,k) / (256*NATAN) , atan2(i,k) - (double)PV_atan2(i,k) / (256*NATAN));
		}
	}
	exit(0);
#endif	
	return 0;
}

int SetUpSqrt()
{
	int i;
	double dz = 1.0 / (double)NSQRT;
	double z = 0.0;
	
	sqrt_LU = (int*) malloc( (NSQRT+1) * sizeof( int ));
	
	if( sqrt_LU == NULL )
		return -1;
		
	for( i=0; i< NSQRT; i++, z+=dz )
		sqrt_LU[i] = int ( sqrt( 1.0 + z*z ) * 256 * NSQRT );
		
	sqrt_LU[NSQRT] = int ( sqrt(2.0) * 256 * NSQRT );
	
	return 0;
}

int SetUpMweights()
{
	int i,k;
	
	for(i=0; i<256; i++)
	{
		mweights[i] = (int*)malloc( 256 * sizeof(int) );
		if( mweights[i] == NULL ) return -1;
	}
	for(i=0; i<256; i++)
	{
		for(k=0; k<256; k++)
		{
			mweights[i][k] = i*k;
		}
	}
	
	return 0;
}


int PV_sqrt( int x1, int x2 )
{
	if( x1 > x2 )
	{
		return  x1 * sqrt_LU[ NSQRT * x2 /  x1 ] / NSQRT;
	}
	else
	{
		if( x2 == 0 ) return 0;
		return x2 * sqrt_LU[ NSQRT * x1 /  x2 ] / NSQRT;
	}
}


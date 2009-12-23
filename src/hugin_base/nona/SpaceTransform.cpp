// -*- c-basic-offset: 4 -*-

/** @file SpaceTransform.cpp
 *
 *  @implementation of Space Transformation
 *
 *  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "SpaceTransform.h"


using namespace std;
using namespace vigra;
using namespace hugin_utils;


namespace HuginBase {
namespace Nona {
        

/// ctor
SpaceTransform::SpaceTransform()
{
	m_Initialized = false;
}

/// dtor
SpaceTransform::~SpaceTransform()
{
}

void SpaceTransform::AddTransform( trfn function_name, double var0, double var1, double var2, double var3, double var4, double var5, double var6, double var7 )
{
	fDescription fD;
	fD.param.var0	= var0;
	fD.param.var1	= var1;
	fD.param.var2	= var2;
	fD.param.var3	= var3;
	fD.param.var4	= var4;
	fD.param.var5	= var5;
        fD.param.var6   = var6;
        fD.param.var7   = var7;
	fD.func			= function_name;
	m_Stack.push_back( fD );
}

void SpaceTransform::AddTransform( trfn function_name, Matrix3 m, double var0, double var1, double var2, double var3)
{
	fDescription fD;
	fD.param.distance	= var0;
	fD.param.var1	= var1;
	fD.param.var2	= var2;
	fD.param.var3	= var3;
	fD.param.mt			= m;
	fD.func				= function_name;
	m_Stack.push_back( fD );
}




//==============================================================================
// Pano12.dll (math.c) functions, helmut dersch

#define MAXITER 100
#define R_EPS 1.0e-6

// Rotate equirectangular image
void rotate_erect( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams &params)
{
	// params: double 180degree_turn(screenpoints), double turn(screenpoints);
	*x_src = x_dest + params.var1;
	while( *x_src < -params.var0 )
		*x_src += 2 * params.var0;
	while( *x_src >  params.var0 )
		*x_src -= 2 * params.var0;
	*y_src = y_dest;
}

// Calculate inverse 4th order polynomial correction using Newton
// Don't use on large image (slow)!
void inv_radial( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams &params)
{
	// params: double coefficients[5]
	register double rs, rd, f, scale;
	int iter = 0;

	rd	= (sqrt( x_dest*x_dest + y_dest*y_dest )) / params.var4; // Normalized

	rs	= rd;				
	f 	= (((params.var3 * rs + params.var2) * rs + params.var1) * rs + params.var0) * rs;

	while( abs(f - rd) > R_EPS && iter++ < MAXITER )
	{
		rs = rs - (f - rd) / ((( 4 * params.var3 * rs + 3 * params.var2) * rs  +
						  2 * params.var1) * rs + params.var0);

		f 	= (((params.var3 * rs + params.var2) * rs +
				params.var1) * rs + params.var0) * rs;
	}

	scale = rs / rd;
//	printf("scale = %lg iter = %d\n", scale,iter);	
	
	*x_src = x_dest * scale  ;
	*y_src = y_dest * scale  ;
}

/*
static void inv_vertical( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams &params)
{
	// params: double coefficients[4]
	register double rs, rd, f, scale;
	int iter = 0;

	rd 	= abs( y_dest ) / params.var4; // Normalized
	rs	= rd;				
	f 	= (((params.var3 * rs + params.var2) * rs + params.var1) * rs + params.var0) * rs;

	while( abs(f - rd) > R_EPS && iter++ < MAXITER )
	{
		rs = rs - (f - rd) / ((( 4 * params.var3 * rs + 3 * params.var2) * rs  +
						  2 * params.var1) * rs + params.var0);

		f 	= (((params.var3 * rs + params.var2) * rs + params.var1) * rs + params.var0) * rs;
	}

	scale = rs / rd;
//	printf("scale = %lg iter = %d\n", scale,iter);	
	
	*x_src = x_dest  ;
	*y_src = y_dest * scale  ;
}
*/

// scale
void resize( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double scale_horizontal, double scale_vertical;
	*x_src = x_dest * params.var0;
	*y_src = y_dest * params.var1;
}

/*
// shear
static void shear( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double shear_horizontal, double shear_vertical;
	*x_src  = x_dest + params.var0 * y_dest;
	*y_src  = y_dest + params.var1 * x_dest;
}
*/

// horiz shift
void horiz( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double horizontal params.shift
	*x_src	= x_dest + params.shift;	
	*y_src  = y_dest;
}

// vertical shift
void vert( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double vertical params.shift
	*x_src	= x_dest;	
	*y_src  = y_dest + params.shift;
}

// radial
void radial( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double coefficients[4], scale, correction_radius
	register double r, scale;

	r = (sqrt( x_dest*x_dest + y_dest*y_dest )) / params.var4;
	if( r < params.var5 )
	{
		scale = ((params.var3 * r + params.var2) * r + params.var1) * r + params.var0;
	}
	else
		scale = 1000.0;
	
	*x_src = x_dest * scale  ;
	*y_src = y_dest * scale  ;
}

/*
//
static void vertical( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double coefficients[4]
	register double r, scale;

	r = y_dest / params.var4;

	if( r < 0.0 ) r = -r;

	scale = ((params.var3 * r + params.var2) * r + params.var1) * r + params.var0;
	
	*x_src = x_dest;
	*y_src = y_dest * scale  ;
}
*/

/*
//
static void deregister( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double coefficients[4]
	register double r, scale;

	r = y_dest / params.var4;

	if( r < 0.0 ) r = -r;

	scale 	= (params.var3 * r + params.var2) * r + params.var1 ;
	
	*x_src = x_dest + abs( y_dest ) * scale;
	*y_src = y_dest ;
}
*/
// perspective
void persp_sphere( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params :  double Matrix[3][3], double params.distance
	register double theta,s,r;
	Vector3 v, v2;

	r = sqrt( x_dest * x_dest + y_dest * y_dest );
	theta 	= r / params.distance;
	if( r == 0.0 )
		s = 0.0;
	else
		s = sin( theta ) / r;

	v.x =  s * x_dest ;
	v.y =  s * y_dest ;
	v.z =  cos( theta );

	v2 = params.mt.TransformVector( v );

	r = sqrt( v2.x*v2.x + v2.y*v2.y );
	if( r == 0.0 )
		theta = 0.0;
	else
		theta 	= params.distance * atan2( r, v2.z ) / r;
	*x_src 	= theta * v2.x;
	*y_src 	= theta * v2.y;
}	


// perspective rect
void persp_rect( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params :  double Matrix[3][3], double params.distance, double x-offset, double y-offset
	Vector3 v;
	v.x = x_dest + params.var2;
	v.y = y_dest + params.var3;
	v.z = params.var1;
	v = params.mt.TransformVector( v );
	*x_src = v.x * params.var1 / v.z;
	*y_src = v.y * params.var1 / v.z;
}

/*
//
static void rect_pano( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{									
	*x_src = params.distance * tan( x_dest / params.distance ) ;
	*y_src = y_dest / cos( x_dest / params.distance );
}
*/

/*
//
static void pano_rect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{	
	*x_src = params.distance * atan ( x_dest / params.distance );
	*y_src = y_dest * cos( *x_src / params.distance );
}
*/

//
void rect_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{	
	// params: double params.distance
	register double  phi, theta;

	phi 	= x_dest / params.distance;
	theta 	=  - y_dest / params.distance  + PI / 2.0;
	if(theta < 0)
	{
		theta = - theta;
		phi += PI;
	}
	if(theta > PI)
	{
		theta = PI - (theta - PI);
		phi += PI;
	}

	*x_src = params.distance * tan(phi);
	*y_src = params.distance / (tan( theta ) * cos(phi));
}

//
void pano_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{	
	// params: double params.distance
	*x_src = x_dest;
	*y_src = params.distance * tan( y_dest / params.distance);
}

//
void erect_pano( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{	
	// params: double params.distance
	*x_src = x_dest;
	*y_src = params.distance * atan( y_dest / params.distance);
}

// FIXME: implement!
void transpano_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{	
    // params: double params.distance
    *x_src = x_dest;
    *y_src = params.distance * tan( y_dest / params.distance);
}

// FIXME: implement!
void erect_transpano( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{	
	// params: double params.distance
    *x_src = x_dest;
    *y_src = params.distance * atan( y_dest / params.distance);
}

/*
//
static void sphere_cp_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance, double b
	register double phi, theta;
	phi 	= - x_dest /  ( params.var0 * PI / 2.0);
	theta 	=  - ( y_dest + params.var1 ) / ( PI / 2.0) ;
	*x_src =  theta * cos( phi );
	*y_src =  theta * sin( phi );
}
*/

//
void sphere_tp_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance
	register double phi, theta, r,s;
	double v[3];
	phi 	= x_dest / params.distance;
	theta 	=  - y_dest / params.distance  + PI / 2;
	if(theta < 0)
	{
		theta = - theta;
		phi += PI;
	}
	if(theta > PI)
	{
		theta = PI - (theta - PI);
		phi += PI;
	}
	s = sin( theta );
	v[0] =  s * sin( phi );	//  y' -> x
	v[1] =  cos( theta );				//  z' -> y
	r = sqrt( v[1]*v[1] + v[0]*v[0]);	
	theta = params.distance * atan2( r , s * cos( phi ) );

	*x_src =  theta * v[0] / r;
	*y_src =  theta * v[1] / r;
}

/*
//
static void erect_sphere_cp( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance, double b
	register double phi, theta;
	theta = sqrt( x_dest * x_dest + y_dest * y_dest ) ;
	phi   = atan2( y_dest , -x_dest );
	*x_src = params.var0 * phi;
	*y_src = theta - params.var1;
}
*/

//
void rect_sphere_tp( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance
	register double rho, theta,r;
	r = sqrt( x_dest*x_dest + y_dest*y_dest );
	theta 	= r / params.distance;

    if( theta >= PI /2.0 )
    	rho = 1.6e16 ;
    else if( theta == 0.0 )
		rho = 1.0;
	else
		rho =  tan( theta ) / theta;
	*x_src = rho * x_dest ;
	*y_src = rho * y_dest ;
}

//
void sphere_tp_rect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{	
	// params: double params.distance
	register double  theta, r;
	r = sqrt(x_dest*x_dest + y_dest*y_dest) / params.distance;
	if( r== 0.0 )
		theta = 1.0;
	else
		theta 	= atan( r ) / r;
	*x_src =  theta * x_dest ;
	*y_src =  theta * y_dest ;
}

//
void sphere_tp_pano( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance
	register double r, s, Phi, theta;
	Phi = x_dest / params.distance;
	s =  params.distance * sin( Phi ) ;	//  y' -> x
	r = sqrt( s*s + y_dest*y_dest );
	theta = params.distance * atan2( r , (params.distance * cos( Phi )) ) / r;
	*x_src =  theta * s ;
	*y_src =  theta * y_dest ;
}

//
void pano_sphere_tp( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance
	register double r,s, theta;
	double v[3];
	r = sqrt( x_dest * x_dest + y_dest * y_dest );
	theta = r / params.distance;
	if( theta == 0.0 )
		s = 1.0 / params.distance;
	else
		s = sin( theta ) /r;
	v[1] =  s * x_dest ;   //  x' -> y
	v[0] =  cos( theta );				//  z' -> x
	*x_src = params.distance * atan2( v[1], v[0] );
	*y_src = params.distance * s * y_dest / sqrt( v[0]*v[0] + v[1]*v[1] );
}

/*
//
static void sphere_cp_pano( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance
	register double phi, theta;
	phi 	= -x_dest / (params.distance * PI / 2.0) ;
	theta	= PI /2.0 + atan( y_dest / (params.distance * PI/2.0) );
	*x_src = params.distance * theta * cos( phi );
	*y_src = params.distance * theta * sin( phi );
}
*/

//
void erect_rect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance
	*x_src = params.distance * atan2( x_dest, params.distance );
	*y_src = params.distance * atan2(  y_dest, sqrt( params.distance*params.distance + x_dest*x_dest ) );
}

//
void erect_sphere_tp( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance
	register double  theta,r,s;
	double	v[3];
	r = sqrt( x_dest * x_dest + y_dest * y_dest );
	theta = r / params.distance;
	if(theta == 0.0)
		s = 1.0 / params.distance;
	else
		s = sin( theta) / r;
	
	v[1] =  s * x_dest;
	v[0] =  cos( theta );				
	
	*x_src = params.distance * atan2( v[1], v[0] );
	*y_src = params.distance * atan( s * y_dest /sqrt( v[0]*v[0] + v[1]*v[1] ) );
}

/** convert from erect to mercator */
void mercator_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance
    *x_src = x_dest;
    *y_src = params.distance*log(tan(y_dest/params.distance)+1/cos(y_dest/params.distance));
}

/** convert from mercator to erect */
void erect_mercator( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance
    *x_src = x_dest;
    *y_src = params.distance*atan(sinh(y_dest/params.distance));
}


/** convert from erect to transverse mercator */
void transmercator_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance
    x_dest /= params.distance;
    y_dest /= params.distance;
    double B = cos(y_dest)*sin(x_dest);
    *x_src = params.distance / tanh(B);
    *y_src = params.distance * atan(tan(y_dest)/cos(x_dest));
}

/** convert from erect to transverse mercator */
void erect_transmercator( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance
    x_dest /= params.distance;
    y_dest /= params.distance;
    *x_src = params.distance * atan(sinh(x_dest)/cos(y_dest));
    *y_src = params.distance * asin(sin(y_dest)/cosh(x_dest));
}

/** convert from erect to sinusoidal */
void sinusoidal_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance

    *x_src = params.distance * (x_dest/params.distance*cos(y_dest/params.distance));
    *y_src = y_dest;
}

/** convert from sinusoidal to erect */
void erect_sinusoidal( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance

    *y_src = y_dest;
    *x_src = x_dest/cos(y_dest/params.distance);
}

/** convert from erect to stereographic */
void stereographic_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance
    double lon = x_dest / params.distance;
    double lat = y_dest / params.distance;

    // use: R = 1
    double k=2.0/(1+cos(lat)*cos(lon));
    *x_src = params.distance * k*cos(lat)*sin(lon);
    *y_src = params.distance * k*sin(lat);
}

/** convert from stereographic to erect */
void erect_stereographic( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: distance

    // use: R = 1
    double p=sqrt(x_dest*x_dest + y_dest*y_dest) / params.distance;
    double c= 2.0*atan(p/2.0);

    *x_src = params.distance * atan2(x_dest/params.distance*sin(c),(p*cos(c)));
    *y_src = params.distance * asin(y_dest/params.distance*sin(c)/p);
}


/*
//
static void mirror_sphere_cp( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance, double b
	register double rho, phi, theta;
	theta = sqrt( x_dest * x_dest + y_dest * y_dest ) / params.var0;
	phi   = atan2( y_dest , x_dest );
	rho = params.var1 * sin( theta / 2.0 );
	*x_src = - rho * cos( phi );
	*y_src = rho * sin( phi );
}
*/

/*
//
static void mirror_erect( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance, double b, double b2
	register double phi, theta, rho;
	phi 	=  x_dest / ( params.var0 * PI/2.0) ;
	theta 	=  - ( y_dest + params.var2 ) / (params.var0 * PI/2.0)  ;
	rho = params.var1 * sin( theta / 2.0 );
	*x_src = - rho * cos( phi );
	*y_src = rho * sin( phi );
}
*/

/*
//
static void mirror_pano( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance, double b
	register double phi, theta, rho;
	phi 	= -x_dest / (params.var0 * PI/2.0) ;
	theta	= PI /2.0 + atan( y_dest / (params.var0 * PI/2.0) );
	rho = params.var1 * sin( theta / 2.0 );
	*x_src = rho * cos( phi );
	*y_src = rho * sin( phi );
}
*/

/*
//
static void sphere_cp_mirror( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.distance, double b
	register double phi, theta, rho;
	rho = sqrt( x_dest*x_dest + y_dest*y_dest );
	theta = 2 * asin( rho/params.var1 );
	phi   = atan2( y_dest , x_dest );
	*x_src = params.var0 * theta * cos( phi );
	*y_src = params.var0 * theta * sin( phi );
}
*/

/*
//
static void shift_scale_rotate( double x_dest,double  y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
	// params: double params.shift_x, params.shift_y, scale, cos_phi, sin_phi
	register double x = x_dest - params.var0;
	register double y = y_dest - params.var1;
	*x_src = (x * params.var3 - y * params.var4) * params.var2;
	*y_src = (x * params.var4 + y * params.var3) * params.var2;
}
*/

/*

// Correct radial luminance change using parabel
unsigned char radlum( unsigned char srcPixel, int xc, int yc, void *params )
{
	// params: second and zero order polynomial coeff
	register double result;

	result = (xc * xc + yc * yc) * params.var0 + params.var1;
	result = ((double)srcPixel) - result;

	if(result < 0.0) return 0;
	if(result > 255.0) return 255;

	return( (unsigned char)(result+0.5) );
}


// Get smallest positive (non-zero) root of polynomial with degree deg and
// (n+1) real coefficients p[i]. Return it, or 1000.0 if none exists or error occured
// Changed to only allow degree 3
#if 0
double smallestRoot( double *p )
{
	doublecomplex 		root[3], poly[4];
	doublereal 			radius[3], apoly[4], apolyr[4];
	logical 			myErr[3];
	double 				sRoot = 1000.0;
	doublereal 			theEps, theBig, theSmall;
	integer 			nitmax;
	integer 			iter;
	integer 			n,i;
	
	n 		= 3;

	
	for( i=0; i< n+1; i++)
	{
		poly[i].r = p[i];
		poly[i].i = 0.0;
	}
	
	theEps   = DBL_EPSILON;  		// machine precision
	theSmall = DBL_MIN ; 			// smallest positive real*8
	theBig   = DBL_MAX ; 			// largest real*8

	nitmax 	= 100;

    polzeros_(&n, poly, &theEps, &theBig, &theSmall, &nitmax, root, radius, myErr, &iter, apoly, apolyr);

	for( i = 0; i < n; i++ )
	{
//		PrintError("No %d : Real %g, Imag %g, radius %g, myErr %ld", i, root[i].r, root[i].i, radius[i], myErr[i]);
		if( (root[i].r > 0.0) && (dabs( root[i].i ) <= radius[i]) && (root[i].r < sRoot) )
			sRoot = root[i].r;
	}

	return sRoot;
}
#endif

void cubeZero( double *a, int *n, double *root );
void squareZero( double *a, int *n, double *root );
double cubeRoot( double x );


void cubeZero( double *a, int *n, double *root ){
	if( a[3] == 0.0 ){ // second order polynomial
		squareZero( a, n, root );
	}else{
		double p = ((-1.0/3.0) * (a[2]/a[3]) * (a[2]/a[3]) + a[1]/a[3]) / 3.0;
		double q = ((2.0/27.0) * (a[2]/a[3]) * (a[2]/a[3]) * (a[2]/a[3]) - (1.0/3.0) * (a[2]/a[3]) * (a[1]/a[3]) + a[0]/a[3]) / 2.0;
		
		if( q*q + p*p*p >= 0.0 ){
			*n = 1;
			root[0] = cubeRoot(-q + sqrt(q*q + p*p*p)) + cubeRoot(-q - sqrt(q*q + p*p*p)) - a[2] / (3.0 * a[3]);
		}else{
			double phi = acos( -q / sqrt(-p*p*p) );
			*n = 3;
			root[0] =  2.0 * sqrt(-p) * cos(phi/3.0) - a[2] / (3.0 * a[3]);
			root[1] = -2.0 * sqrt(-p) * cos(phi/3.0 + PI/3.0) - a[2] / (3.0 * a[3]);
			root[2] = -2.0 * sqrt(-p) * cos(phi/3.0 - PI/3.0) - a[2] / (3.0 * a[3]);
		}
	}
	// PrintError("%lg, %lg, %lg, %lg root = %lg", a[3], a[2], a[1], a[0], root[0]);
}

void squareZero( double *a, int *n, double *root ){
	if( a[2] == 0.0 ){ // linear equation
		if( a[1] == 0.0 ){ // constant
			if( a[0] == 0.0 ){
				*n = 1; root[0] = 0.0;
			}else{
				*n = 0;
			}
		}else{
			*n = 1; root[0] = - a[0] / a[1];
		}
	}else{
		if( 4.0 * a[2] * a[0] > a[1] * a[1] ){
			*n = 0;
		}else{
			*n = 2;
			root[0] = (- a[1] + sqrt( a[1] * a[1] - 4.0 * a[2] * a[0] )) / (2.0 * a[2]);
			root[1] = (- a[1] - sqrt( a[1] * a[1] - 4.0 * a[2] * a[0] )) / (2.0 * a[2]);
		}
	}

}

double cubeRoot( double x ){
	if( x == 0.0 )
		return 0.0;
	else if( x > 0.0 )
		return pow(x, 1.0/3.0);
	else
		return - pow(-x, 1.0/3.0);
}

double smallestRoot( double *p ){
	int n,i;
	double root[3], sroot = 1000.0;
	
	cubeZero( p, &n, root );
	
	for( i=0; i<n; i++){
		// PrintError("Root %d = %lg", i,root[i]);
		if(root[i] > 0.0 && root[i] < sroot)
			sroot = root[i];
	}
	
	// PrintError("Smallest Root  = %lg", sroot);
	return sroot;
}

*/



// really strange. the pano12.dll for windows doesn't seem to
// contain the SetCorrectionRadius function, so it is included here

static void cubeZero_copy( double *a, int *n, double *root );
static void squareZero_copy( double *a, int *n, double *root );
static double cubeRoot_copy( double x );


static void cubeZero_copy( double *a, int *n, double *root ){
	if( a[3] == 0.0 ){ // second order polynomial
		squareZero_copy( a, n, root );
	}else{
		double p = ((-1.0/3.0) * (a[2]/a[3]) * (a[2]/a[3]) + a[1]/a[3]) / 3.0;
		double q = ((2.0/27.0) * (a[2]/a[3]) * (a[2]/a[3]) * (a[2]/a[3]) - (1.0/3.0) * (a[2]/a[3]) * (a[1]/a[3]) + a[0]/a[3]) / 2.0;
		
		if( q*q + p*p*p >= 0.0 ){
			*n = 1;
			root[0] = cubeRoot_copy(-q + sqrt(q*q + p*p*p)) + cubeRoot_copy(-q - sqrt(q*q + p*p*p)) - a[2] / (3.0 * a[3]);
		}else{
			double phi = acos( -q / sqrt(-p*p*p) );
			*n = 3;
			root[0] =  2.0 * sqrt(-p) * cos(phi/3.0) - a[2] / (3.0 * a[3]);
			root[1] = -2.0 * sqrt(-p) * cos(phi/3.0 + PI/3.0) - a[2] / (3.0 * a[3]);
			root[2] = -2.0 * sqrt(-p) * cos(phi/3.0 - PI/3.0) - a[2] / (3.0 * a[3]);
		}
	}
	// PrintError("%lg, %lg, %lg, %lg root = %lg", a[3], a[2], a[1], a[0], root[0]);
}

static void squareZero_copy( double *a, int *n, double *root ){
	if( a[2] == 0.0 ){ // linear equation
		if( a[1] == 0.0 ){ // constant
			if( a[0] == 0.0 ){
				*n = 1; root[0] = 0.0;
			}else{
				*n = 0;
			}
		}else{
			*n = 1; root[0] = - a[0] / a[1];
		}
	}else{
		if( 4.0 * a[2] * a[0] > a[1] * a[1] ){
			*n = 0;
		}else{
			*n = 2;
			root[0] = (- a[1] + sqrt( a[1] * a[1] - 4.0 * a[2] * a[0] )) / (2.0 * a[2]);
			root[1] = (- a[1] - sqrt( a[1] * a[1] - 4.0 * a[2] * a[0] )) / (2.0 * a[2]);
		}
	}

}

static double cubeRoot_copy( double x ){
	if( x == 0.0 )
		return 0.0;
	else if( x > 0.0 )
		return pow(x, 1.0/3.0);
	else
		return - pow(-x, 1.0/3.0);
}

static double smallestRoot_copy( double *p ){
	int n,i;
	double root[3], sroot = 1000.0;
	
	cubeZero_copy( p, &n, root );
	
	for( i=0; i<n; i++){
		// PrintError("Root %d = %lg", i,root[i]);
		if(root[i] > 0.0 && root[i] < sroot)
			sroot = root[i];
	}
	
	// PrintError("Smallest Root  = %lg", sroot);
	return sroot;
}


// Restrict radial correction to monotonous interval
static double CalcCorrectionRadius_copy(double *coeff )
{
    double a[4];
    int k;
	
    for( k=0; k<4; k++ )
    {
        a[k] = 0.0;//1.0e-10;
        if( coeff[k] != 0.0 )
        {
            a[k] = (k+1) * coeff[k];
        }
    }
    return smallestRoot_copy( a );
}

// radial and shift
static void radial_shift( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams & params)
{
    // params: double coefficients[4], scale, correction_radius, shift_x, shift_y
    register double r, scale;

    r = (sqrt( x_dest*x_dest + y_dest*y_dest )) / params.var4;
    if( r < params.var5 )
    {
        scale = ((params.var3 * r + params.var2) * r + params.var1) * r + params.var0;
    }
    else
        scale = 1000.0;
	
    *x_src = x_dest * scale  + params.var6;
    *y_src = y_dest * scale  + params.var7;
}


//==============================================================================


Matrix3 SetMatrix( double a, double b, double c, int cl )
{
    Matrix3 mx, my, mz;
    //    Matrix3 dummy;
	
    // Calculate Matrices;
    mx.SetRotationX( a );
    my.SetRotationY( b );
    mz.SetRotationZ( c );
	
    if (cl)
        return ( (mz * mx) * my );
    else
        return ( (mx * mz) * my );
	
	//if( cl )
	//		matrix_matrix_mult( mz,	mx,	dummy);
	//else
	//	matrix_matrix_mult( mx,	mz,	dummy);
	//matrix_matrix_mult( dummy, my, m);
}


double estScaleFactorForFullFrame(const SrcPanoImage & src)
{
    SpaceTransform transf;
    transf.InitInvRadialCorrect(src, 1);
    vigra::Rect2D inside;
    vigra::Rect2D insideTemp;
    vigra::Rect2D boundingBox;
    traceImageOutline(src.getSize(), transf, inside, boundingBox);
    if (src.getCorrectTCA()) {
        transf.InitInvRadialCorrect(src, 0);
        traceImageOutline(src.getSize(), transf, insideTemp, boundingBox);
        inside &= insideTemp;
        transf.InitInvRadialCorrect(src, 2);
        traceImageOutline(src.getSize(), transf, insideTemp, boundingBox);
        inside &= insideTemp;
    }
    double width2 = src.getSize().x/2.0;
    double height2 = src.getSize().y/2.0;
    double sx = std::max(width2/(width2-inside.left()), width2/(inside.right()-width2));
    double sy = std::max(height2/(height2-inside.top()), height2/(inside.bottom()-height2));
    return 1/std::max(sx,sy);
}


double estRadialScaleCrop(const vector<double> &coeff, int width, int height)
{
    double r_test[4];
    double p, r;
    int test_points, i;
    double a, b, c, d;

    // truncate black edges
    // algorithm courtesy of Paul Wilkinson, paul.wilkinson@ntlworld.com
    // modified by dangelo to just calculate the scaling factor -> better results

    a = coeff[0];
    b = coeff[1];
    c = coeff[2];
    d = coeff[3];

    if (width > height)
        p = (double)(width) / (double)(height);
    else
        p = (double)(height) / (double)(width);

    //***************************************************
    //* Set the test point for the far corner.          *
    //***************************************************
    r_test[0] = sqrt(1 + p * p);
    test_points = 1;

    //***************************************************
    //* For non-zero values of a, there are two other   *
    //* possible test points. (local extrema)           *
    //***************************************************
    //

    if (a != 0.0)
    {
        r = (-b + sqrt(b * b - 3 * a * c)) / (3 * a);
        if (r >= 1 && r <= r_test[0])
        {
            r_test[test_points] = r;
            test_points = test_points + 1;
        }
        r = (-b - sqrt(b * b - 3 * a * c)) / (3 * a);
        if (r >= 1 && r <= r_test[0])
        {
            r_test[test_points] = r;
            test_points = test_points + 1;
        }
    }

    //***************************************************
    //* For zero a and non-zero b, there is one other   *
    //* possible test point.                            *
    //***************************************************
    if (a == 0.0 && b != 0.0)
    {
        r = -c / (2 * b);
        if (r >= 1 && r <= r_test[0])
        {
            r_test[test_points] = r;
            test_points = test_points + 1;
        }
    }

    // check the scaling factor at the test points.
    // start with a very high scaling factor.
    double scalefactor = 0.1;
    for (i = 0; i <= test_points - 1; i++)
    {
        r = r_test[i];
        double scale = d + r * (c + r * (b + r * a));
        if ( scalefactor < scale)
            scalefactor = scale;
    }
    return scalefactor;
}



//==============================================================================


/** Create a transform stack for radial distortion correction only */
void SpaceTransform::InitRadialCorrect(const Size2D & sz, const vector<double> & radDist, 
                                 const FDiff2D & centerShift)
{
    double mprad[6];

//    double  imwidth = src.getSize().x;
//    double  imheight= src.getSize().y;

    m_Stack.clear();
    m_srcTX = sz.x/2.0;
    m_srcTY = sz.y/2.0;
    m_destTX = sz.x/2.0;
    m_destTY = sz.y/2.0;

    // green channel, always correct
    for (int i=0; i < 4; i++) {
        mprad[3-i] = radDist[i];
    }
    mprad[4] = ( sz.x < sz.y ? sz.x: sz.y)  / 2.0;
    // calculate the correction radius.
    mprad[5] = CalcCorrectionRadius_copy(mprad);

    // radial correction if nonzero radial coefficients
    if ( mprad[0] != 1.0 || mprad[1] != 0.0 || mprad[2] != 0.0 || mprad[3] != 0.0) {
        AddTransform (&radial_shift, mprad[0], mprad[1], mprad[2], mprad[3], mprad[4], mprad[5],
                     centerShift.x, centerShift.y);
    }
}

/** Create a transform stack for distortion & TCA correction only */
void SpaceTransform::InitInvRadialCorrect(const SrcPanoImage & src, int channel)
{
    double mprad[6];

//    double  imwidth = src.getSize().x;
//    double  imheight= src.getSize().y;

    m_Stack.clear();
    m_srcTX = src.getSize().x/2.0;
    m_srcTY = src.getSize().y/2.0;
    m_destTX = src.getSize().x/2.0;
    m_destTY = src.getSize().y/2.0;

    if (src.getRadialDistortionCenterShift().x != 0.0) {
        AddTransform(&horiz, -src.getRadialDistortionCenterShift().x);
    }

    // shift optical center if needed
    if (src.getRadialDistortionCenterShift().y != 0.0) {
        AddTransform(&vert, -src.getRadialDistortionCenterShift().y);
    }

    if (src.getCorrectTCA() && (channel == 0 || channel == 2)) {
        for (int i=0; i < 4; i++) {
            if (channel == 0) {
                // correct red channel (TCA)
                mprad[3-i] = src.getRadialDistortionRed()[i];
            } else {
                // correct blue channel (TCA)
                mprad[3-i] = src.getRadialDistortionBlue()[i];
            }
        }
        mprad[4] = ( src.getSize().x < src.getSize().y ? src.getSize().x: src.getSize().y)  / 2.0;
        // calculate the correction radius.
        mprad[5] = CalcCorrectionRadius_copy(mprad);

        // radial correction if nonzero radial coefficients
        if ( mprad[0] != 1.0 || mprad[1] != 0.0 || mprad[2] != 0.0 || mprad[3] != 0.0) {
            AddTransform (&inv_radial, mprad[0], mprad[1], mprad[2], mprad[3], mprad[4], mprad[5]);
        }
    }

    // green channel, always correct
    for (int i=0; i < 4; i++) {
        mprad[3-i] = src.getRadialDistortion()[i];
    }
    mprad[4] = ( src.getSize().x < src.getSize().y ? src.getSize().x: src.getSize().y)  / 2.0;
    // calculate the correction radius.
    mprad[5] = CalcCorrectionRadius_copy(mprad);

    // radial correction if nonzero radial coefficients
    if ( mprad[0] != 1.0 || mprad[1] != 0.0 || mprad[2] != 0.0 || mprad[3] != 0.0) {
        AddTransform (&inv_radial, mprad[0], mprad[1], mprad[2], mprad[3], mprad[4], mprad[5]);
    }
}


/** Create a transform stack for distortion & TCA correction only */
void SpaceTransform::InitRadialCorrect(const SrcPanoImage & src, int channel)
{
    double mprad[6];

//    double  imwidth = src.getSize().x;
//    double  imheight= src.getSize().y;

    m_Stack.clear();
    m_srcTX = src.getSize().x/2.0;
    m_srcTY = src.getSize().y/2.0;
    m_destTX = src.getSize().x/2.0;
    m_destTY = src.getSize().y/2.0;

    // green channel, always correct
    for (int i=0; i < 4; i++) {
        mprad[3-i] = src.getRadialDistortion()[i];
    }
    mprad[4] = ( src.getSize().x < src.getSize().y ? src.getSize().x: src.getSize().y)  / 2.0;
    // calculate the correction radius.
    mprad[5] = CalcCorrectionRadius_copy(mprad);

    // radial correction if nonzero radial coefficients
    if ( mprad[0] != 1.0 || mprad[1] != 0.0 || mprad[2] != 0.0 || mprad[3] != 0.0) {
        AddTransform (&radial, mprad[0], mprad[1], mprad[2], mprad[3], mprad[4], mprad[5]);
        DEBUG_DEBUG("Init Radial (green): " 
                << "g: " << mprad[0] << " " << mprad[1] << " " << mprad[2] 
                << " " << mprad[3] << " " << mprad[4] << " " << mprad[5]);
    }

    if (src.getCorrectTCA() && (channel == 0 || channel == 2)) {
        for (int i=0; i < 4; i++) {
            if (channel == 0) {
                // correct red channel (TCA)
                mprad[3-i] = src.getRadialDistortionRed()[i];
            } else {
                // correct blue channel (TCA)
                mprad[3-i] = src.getRadialDistortionBlue()[i];
            }
        }
        mprad[4] = ( src.getSize().x < src.getSize().y ? src.getSize().x: src.getSize().y)  / 2.0;
        // calculate the correction radius.
        mprad[5] = CalcCorrectionRadius_copy(mprad);

        // radial correction if nonzero radial coefficients
        if ( mprad[0] != 1.0 || mprad[1] != 0.0 || mprad[2] != 0.0 || mprad[3] != 0.0) {
            AddTransform (&radial, mprad[0], mprad[1], mprad[2], mprad[3], mprad[4], mprad[5]);
            DEBUG_DEBUG("Init Radial (channel " << channel << "): " 
                    << "g: " << mprad[0] << " " << mprad[1] << " " << mprad[2] 
                    << " " << mprad[3] << " " << mprad[4] << " " << mprad[5]);
        }
    }

    // shift optical center if needed
    if (src.getRadialDistortionCenterShift().y != 0.0) {
        AddTransform(&vert, src.getRadialDistortionCenterShift().y);
    }
    if (src.getRadialDistortionCenterShift().x != 0.0) {
        AddTransform(&horiz, src.getRadialDistortionCenterShift().x);
    }
}

/** Creates the stacks of matrices and flatten them
  *
  */
void SpaceTransform::Init(
    const SrcPanoImage & image,
    const Diff2D &destSize,
    PanoramaOptions::ProjectionFormat destProj,
    double destHFOV )
{
    int 	i;
    double	a, b;
    Matrix3 mpmt;
    double  mpdistance, mpscale[2], mpshear[2], mprot[2], mprad[6];
    // double mpperspect[2];
    double  mphorizontal, mpvertical;

    double  imhfov  = image.getHFOV();
    vigra::Size2D srcSize = image.getSize();
    double  imwidth = srcSize.x;
    double  imheight= srcSize.y;
    double  imyaw   = image.getYaw();
    double  impitch = image.getPitch();
    double  imroll  = image.getRoll();
    double  ima = image.getRadialDistortion()[0];
    double  imb = image.getRadialDistortion()[1];
    double  imc = image.getRadialDistortion()[2];
    double  imd = image.getRadialDistortionCenterShift().x;
    double  ime = image.getRadialDistortionCenterShift().y;
    double  img = image.getShear().x;
    double  imt = image.getShear().y;
    double  pnhfov  = destHFOV;
    double  pnwidth = destSize.x;
    SrcPanoImage::Projection srcProj = image.getProjection();
//    double  pnheight= destSize.y;

    m_Stack.clear();
    m_srcTX = destSize.x/2.0;
    m_srcTY = destSize.y/2.0;    
    m_destTX = srcSize.x/2.0;
    m_destTY = srcSize.y/2.0;

    a = DEG_TO_RAD( imhfov );	// field of view in rad		
    b = DEG_TO_RAD( pnhfov );

    mpmt = SetMatrix(	-DEG_TO_RAD( impitch ),
                        0.0,
                        - DEG_TO_RAD( imroll ),
                        0 );

    if (destProj == PanoramaOptions::RECTILINEAR)	// rectilinear panorama
    {
        mpdistance = pnwidth / (2.0 * tan(b/2.0));
        if (srcProj == SrcPanoImage::RECTILINEAR)	// rectilinear image
        {
            mpscale[0] = (pnhfov / imhfov) * (a /(2.0 * tan(a/2.0))) * (imwidth/pnwidth) * 2.0 * tan(b/2.0) / b;

        }
        else //  pamoramic or fisheye image
        {
            mpscale[0] = (pnhfov / imhfov) * (imwidth/pnwidth) * 2.0 * tan(b/2.0) / b;
        }
    }
    else // equirectangular or panoramic or fisheye or other.
    {
        mpdistance = pnwidth / b;
        if(srcProj == SrcPanoImage::RECTILINEAR)	// rectilinear image
        {
            mpscale[0] = (pnhfov / imhfov) * (a /(2.0 * tan(a/2.0)))*( imwidth / pnwidth );
        }
        else //  pamoramic or fisheye image
        {
            mpscale[0] = (pnhfov / imhfov) * ( imwidth / pnwidth );
        }
    }
    mpscale[1]		= mpscale[0];
    mpshear[0]		= img / imheight; // TODO : im->cP.shear_x / imheight;
    mpshear[1]		= imt / imwidth; // TODO : im->cP.shear_y / imwidth;
    mprot[0]		= mpdistance * PI;								// 180 in screenpoints
    mprot[1]		= -imyaw *  mpdistance * PI / 180.0; 			//    rotation angle in screenpoints

    // add radial correction
    mprad[3] = ima;
    mprad[2] = imb;
    mprad[1] = imc;
    mprad[0] = 1.0 - (ima+imb+imc);
    mprad[4] = ( imwidth < imheight ? imwidth : imheight)  / 2.0;
    // calculate the correction radius.
    mprad[5] = CalcCorrectionRadius_copy(mprad);

	/*for(i=0; i<4; i++)
		mprad[i] 	= im->cP.radial_params[color][i];
	mprad[5] = im->cP.radial_params[color][4];

	if( (im->cP.correction_mode & 3) == correction_mode_radial )
		mp->rad[4] 	= ( (double)( im->width < im->height ? im->width : im->height) ) / 2.0;
	else
		mp->rad[4] 	= ((double) im->height) / 2.0;*/
		
    mphorizontal = imd;
    mpvertical = ime;
	//mp->horizontal 	= im->cP.horizontal_params[color];
	//mp->vertical 	= im->cP.vertical_params[color];
	
    i = 0;

    switch (destProj)
    {
    case PanoramaOptions::RECTILINEAR :
        // Convert rectilinear to equirect
        AddTransform( &erect_rect, mpdistance );
        break;

    case PanoramaOptions::CYLINDRICAL:
        // Convert panoramic to equirect
        AddTransform( &erect_pano, mpdistance );
        break;
    case PanoramaOptions::EQUIRECTANGULAR:
        // do nothing... coordinates are already equirect
        break;
    //case PanoramaOptions::FULL_FRAME_FISHEYE:
    case PanoramaOptions::FULL_FRAME_FISHEYE:
        // Convert panoramic to sphere
        AddTransform( &erect_sphere_tp, mpdistance );
        break;
    case PanoramaOptions::STEREOGRAPHIC:
        AddTransform( &erect_stereographic, mpdistance );
        break;
    case PanoramaOptions::MERCATOR:
        AddTransform( &erect_mercator, mpdistance );
        break;
    case PanoramaOptions::TRANSVERSE_MERCATOR:
        AddTransform( &erect_transmercator, mpdistance );
        break;
//    case PanoramaOptions::TRANSVERSE_CYLINDRICAL:
//        AddTransform( &erect_transpano, mpdistance );
//        break;
    case PanoramaOptions::SINUSOIDAL:
        AddTransform( &erect_sinusoidal, mpdistance );
        break;
    default:
        DEBUG_FATAL("Fatal error: Unknown projection " << destProj);
        break;
    }

    // Rotate equirect. image horizontally
    AddTransform( &rotate_erect, mprot[0], mprot[1] );
	
    // Convert spherical image to equirect.
    AddTransform( &sphere_tp_erect, mpdistance );
	
    // Perspective Control spherical Image
    AddTransform( &persp_sphere, mpmt, mpdistance );

    switch (srcProj)
    {
    case SrcPanoImage::RECTILINEAR :
        // Convert rectilinear to spherical
        AddTransform( &rect_sphere_tp, mpdistance);
        break;
    case SrcPanoImage::PANORAMIC :
        // Convert panoramic to spherical
        AddTransform( &pano_sphere_tp, mpdistance );
        break;
    case SrcPanoImage::EQUIRECTANGULAR:
        // Convert PSphere to spherical
        AddTransform( &erect_sphere_tp, mpdistance );
        break;
    default:
        // do nothing for CIRCULAR & FULL_FRAME_FISHEYE
        break;
    }

    // Scale Image
    AddTransform( &resize, mpscale[0], mpscale[1] );

    // radial correction if nonzero radial coefficients
    if ( mprad[1] != 0.0 || mprad[2] != 0.0 || mprad[3] != 0.0) {
        AddTransform (&radial, mprad[0], mprad[1], mprad[2], mprad[3], mprad[4], mprad[5]);
    }

    // shift optical center if needed
    if (mpvertical != 0.0) {
        AddTransform(&vert, mpvertical);
    }
    if (mphorizontal != 0.0) {
        AddTransform(&horiz, mphorizontal);
    }

    /*if( im->cP.radial )
      {
		switch( im->cP.correction_mode & 3 )
		{
			case correction_mode_radial:    SetDesc(stack[i],radial,mp->rad); 	  i++; break;
			case correction_mode_vertical:  SetDesc(stack[i],vertical,mp->rad);   i++; break;
			case correction_mode_deregister:SetDesc(stack[i],deregister,mp->rad); i++; break;
		}
	}
	if (  im->cP.vertical)
	{
		SetDesc(stack[i],vert,				&(mp->vertical)); 	i++;
	}
	if ( im->cP.horizontal )
	{
		SetDesc(stack[i],horiz,				&(mp->horizontal)); i++;
	}
	if( im->cP.shear )
	{
		SetDesc( stack[i],shear,			mp->shear		); i++;
	}

	stack[i].func  = (trfn)NULL;*/

}

void SpaceTransform::InitInv(
    const SrcPanoImage & image,
    const Diff2D &destSize,
    PanoramaOptions::ProjectionFormat destProj,
    double destHFOV )
{
    double	a, b;
    Matrix3 mpmt;
    double  mpdistance, mpscale[2], mpshear[2], mprot[2], mprad[6];
//    double  mpperspect[2];
    double mphorizontal, mpvertical;

    double  imhfov  = image.getHFOV();
    vigra::Size2D srcSize = image.getSize();
    double  imwidth = srcSize.x;
    double  imheight= srcSize.y;
    double  imyaw   = image.getYaw();
    double  impitch = image.getPitch();
    double  imroll  = image.getRoll();
    double  ima = image.getRadialDistortion()[0];
    double  imb = image.getRadialDistortion()[1];
    double  imc = image.getRadialDistortion()[2];
    double  imd = image.getRadialDistortionCenterShift().x;
    double  ime = image.getRadialDistortionCenterShift().y;
    SrcPanoImage::Projection srcProj = image.getProjection();
    
    double  pnhfov  = destHFOV;
    double  pnwidth = destSize.x;
//    double  pnheight= destSize.y;

    m_Stack.clear();
    m_srcTX = destSize.x/2.0;
    m_srcTY = destSize.y/2.0;
    m_destTX = srcSize.x/2.0;
    m_destTY = srcSize.y/2.0;


    a =	 DEG_TO_RAD( imhfov );	// field of view in rad		
    b =	 DEG_TO_RAD( pnhfov );

    mpmt = SetMatrix( 	DEG_TO_RAD( impitch ),
                        0.0,
                        DEG_TO_RAD( imroll ),
                        1 );

    if(destProj == PanoramaOptions::RECTILINEAR)	// rectilinear panorama
    {
        mpdistance 	= pnwidth / (2.0 * tan(b/2.0));
        if(srcProj == SrcPanoImage::RECTILINEAR)	// rectilinear image
        {
            mpscale[0] = ( pnhfov/imhfov ) * (a /(2.0 * tan(a/2.0))) * ( imwidth/pnwidth ) * 2.0 * tan(b/2.0) / b;
        }
        else //  pamoramic or fisheye image
        {
            mpscale[0] = ( pnhfov/imhfov ) * ( imwidth/pnwidth ) * 2.0 * tan(b/2.0) / b;
        }
    }
    else // equirectangular or panoramic
    {
        mpdistance 	= pnwidth / b;
        if(srcProj == SrcPanoImage::RECTILINEAR ) // rectilinear image
        {
            mpscale[0] = ( pnhfov/imhfov ) * (a /(2.0 * tan(a/2.0))) * ( imwidth/pnwidth );
        }
        else //  pamoramic or fisheye image
        {
            mpscale[0] = ( pnhfov/imhfov ) * ( imwidth/pnwidth );
        }
    }
    mpshear[0] 	= 0.0f; // TODO -im->cP.shear_x / im->height;
    mpshear[1] 	= 0.0f; // -im->cP.shear_y / im->width;
	
    mpscale[0] = 1.0 / mpscale[0];
    mpscale[1] = mpscale[0];

    // principal point shift
    mphorizontal = - imd;
    mpvertical = - ime;

    // radial correction parameters
    mprad[3] = ima;
    mprad[2] = imb;
    mprad[1] = imc;
    mprad[0] = 1.0 - (ima+imb+imc);
    mprad[4] = ( imwidth < imheight ? imwidth : imheight)  / 2.0;
    // calculate the correction radius.
    mprad[5] = CalcCorrectionRadius_copy(mprad);


    /*mp->horizontal 	= -im->cP.horizontal_params[color];
	mp->vertical 	= -im->cP.vertical_params[color];
	for(i=0; i<4; i++)
		mp->rad[i] 	= im->cP.radial_params[color][i];
	mp->rad[5] = im->cP.radial_params[color][4];
	
	switch( im->cP.correction_mode & 3 )
	{
		case correction_mode_radial: mp->rad[4] = ((double)(im->width < im->height ? im->width : im->height) ) / 2.0;break;
		case correction_mode_vertical:
		case correction_mode_deregister: mp->rad[4] = ((double) im->height) / 2.0;break;
	}
	*/

    mprot[0]	= mpdistance * PI;								// 180 in screenpoints
    mprot[1]	= imyaw *  mpdistance * PI / 180.0; 			//    rotation angle in screenpoints

    //mp->perspect[0] = (void*)(mp->mt);
    //mp->perspect[1] = (void*)&(mp->distance);

    // Perform radial correction
    //if( im->cP.shear )
    //{
    //	SetDesc( stack[i],shear,			mp->shear		); i++;
    //}
    //	

    // principal point shift
    if (mphorizontal != 0.0) {
        AddTransform(&horiz, mphorizontal);
    }
    if (mpvertical != 0.0) {
        AddTransform(&vert, mpvertical);
    }

    // radial distortion
    if ( mprad[1] != 0.0 || mprad[2] != 0.0 || mprad[3] != 0.0) {
        AddTransform (&inv_radial, mprad[0], mprad[1], mprad[2], mprad[3], mprad[4], mprad[5]);
    }
	
    // Scale image
    AddTransform( &resize, mpscale[0], mpscale[1] );
		
    switch (srcProj)
    {
    case SrcPanoImage::RECTILINEAR :
        // rectilinear image
        AddTransform( &sphere_tp_rect, mpdistance );
        break;
    case SrcPanoImage::PANORAMIC :
        // Convert panoramic to spherical
        AddTransform( &sphere_tp_pano, mpdistance );
        break;
    case SrcPanoImage::EQUIRECTANGULAR:
        // Convert PSphere to spherical
        AddTransform( &sphere_tp_erect, mpdistance );
        break;
    default:
        // do nothing for fisheye lenses
        break;
    }

    // Perspective Control spherical Image
    AddTransform( &persp_sphere, mpmt, mpdistance );
    // Convert spherical image to equirect.
    AddTransform( &erect_sphere_tp, mpdistance );
    // Rotate equirect. image horizontally
    AddTransform( &rotate_erect, mprot[0], mprot[1] );

    switch (destProj)
    {
    case PanoramaOptions::RECTILINEAR :
        // Convert rectilinear to spherical
        AddTransform( &rect_erect, mpdistance);
        break;
    case PanoramaOptions::CYLINDRICAL :
        // Convert panoramic to spherical
        AddTransform( &pano_erect, mpdistance );
        break;
    case PanoramaOptions::EQUIRECTANGULAR:
        break;
    case PanoramaOptions::FULL_FRAME_FISHEYE:
        // Convert PSphere to spherical
        AddTransform( &sphere_tp_erect, mpdistance );
        break;
    case PanoramaOptions::STEREOGRAPHIC:
        // Convert PSphere to spherical
        AddTransform( &stereographic_erect, mpdistance );
        break;
    case PanoramaOptions::MERCATOR:
        AddTransform( &mercator_erect, mpdistance );
        break;
    case PanoramaOptions::TRANSVERSE_MERCATOR:
        AddTransform( &transmercator_erect, mpdistance );
        break;
//    case PanoramaOptions::TRANSVERSE_CYLINDRICAL:
//        AddTransform( &transpano_erect, mpdistance );
//        break;
    case PanoramaOptions::SINUSOIDAL:
        AddTransform( &transpano_erect, mpdistance );
        break;
    default:
        DEBUG_FATAL("Fatal error: Unknown projection " << destProj);
        break;
    }
}

//
void SpaceTransform::createTransform(const SrcPanoImage & src, const PanoramaOptions & dest)
{
    Init(src,
         vigra::Size2D(dest.getWidth(), dest.getHeight()),
         dest.getProjection(),
         dest.getHFOV());
}

//
void SpaceTransform::createInvTransform(const SrcPanoImage & src, const PanoramaOptions & dest)
{
    InitInv(src,
            vigra::Size2D(dest.getWidth(), dest.getHeight()),
            dest.getProjection(),
            dest.getHFOV());
}

void SpaceTransform::createTransform(const PanoramaData & pano, unsigned int imgNr,
                     const PanoramaOptions & dest,
                     vigra::Diff2D srcSize)
{
    const SrcPanoImage & img = pano.getImage(imgNr);
    if (srcSize.x == 0 && srcSize.y == 0)
    {
        srcSize = img.getSize();
    }
    Init(pano.getImage(imgNr),
         vigra::Diff2D(dest.getWidth(), dest.getHeight()),
         dest.getProjection(), dest.getHFOV() );
}


// create image->pano transformation
void SpaceTransform::createInvTransform(const PanoramaData & pano, unsigned int imgNr,
                        const PanoramaOptions & dest,
                        vigra::Diff2D srcSize)
{
    const SrcPanoImage & img = pano.getImage(imgNr);
    if (srcSize.x == 0 && srcSize.y == 0)
    {
        srcSize = img.getSize();
    }
    InitInv(pano.getImage(imgNr),
            vigra::Diff2D(dest.getWidth(), dest.getHeight()),
            dest.getProjection(), dest.getHFOV() );
}

/// @todo remove this obsolete function. Callers should use a SrcPanoImg instead
void SpaceTransform::createTransform(const vigra::Diff2D & srcSize,
                     const VariableMap & srcVars,
                     Lens::LensProjectionFormat srcProj,
                     const vigra::Diff2D &destSize,
                     PanoramaOptions::ProjectionFormat destProj,
                     double destHFOV)
{
    SrcPanoImage src_image;
    src_image.setSize(vigra::Size2D(srcSize.x, srcSize.y));
    src_image.setProjection((SrcPanoImage::Projection)srcProj);
    for (VariableMap::const_iterator i = srcVars.begin(); i != srcVars.end(); i++)
    {
        src_image.setVar((*i).first, (*i).second.getValue());
    }
    Init(src_image, destSize, destProj, destHFOV);
}

// create image->pano transformation
/// @todo remove this obsolete function. Callers should use a SrcPanoImg instead
void SpaceTransform::createInvTransform(const vigra::Diff2D & srcSize,
                        const VariableMap & srcVars,
                        Lens::LensProjectionFormat srcProj,
                        const vigra::Diff2D & destSize,
                        PanoramaOptions::ProjectionFormat destProj,
                        double destHFOV)
{
    // make a SrcPanoImage with the necessary data.
    SrcPanoImage src_image;
    src_image.setSize(vigra::Size2D(srcSize.x, srcSize.y));
    src_image.setProjection((SrcPanoImage::Projection)srcProj);
    for (VariableMap::const_iterator i = srcVars.begin(); i != srcVars.end(); i++)
    {
        src_image.setVar((*i).first, (*i).second.getValue());
    }
    InitInv(src_image, destSize, destProj, destHFOV);
}


//
bool SpaceTransform::transform(FDiff2D& dest, const FDiff2D & src) const
{
	double xd = src.x, yd = src.y;
	vector<fDescription>::const_iterator tI;
	
    dest.x = xd;
    dest.y = yd;
    for (tI = m_Stack.begin(); tI != m_Stack.end(); tI++)
    {
        (tI->func)( xd, yd, &dest.x, &dest.y, tI->param );
        xd = dest.x;	
        yd = dest.y;
    }
    return true;
}

//
bool SpaceTransform::transformImgCoord(double & x_dest, double & y_dest, double x_src, double y_src) const
{
	FDiff2D dest, src;
	src.x = x_src - m_srcTX + 0.5;
	src.y = y_src - m_srcTY + 0.5;
	transform( dest, src );
	x_dest = dest.x + m_destTX - 0.5;
	y_dest = dest.y + m_destTY - 0.5;
        return true;
}


} // namespace
} // namespace

/***************************************************************************
 *   Copyright (C) 2009 by Tim Nugent                                      *
 *   timnugent@gmail.com                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/***
  major mods 03Aug2009 TKSharpless
  to integrate the lensFunc universal coordiante mapping facility
  small supporting changesalso in
    Main.c
	Globals.h, .cpp
	ProcessImage.cpp
***/

#include <map>
#include <iostream>
#include <stdio.h>
#include <math.h>
#include <cstdlib>
#include <levmar/lm.h>
#include "Globals.h"

#include "lensFunc.h"
#include <assert.h>

using namespace std;

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

class XYZPoint {
public:
	double x, y, z;
	XYZPoint( double X, double Y, double Z )
		: x(X), y(Y), z(Z) {}
	XYZPoint() : x(0), y(0), z(0) {}
	XYZPoint( double *p ) 
		: x(p[0]), y(p[1]), z(p[2]) {}
	double normalize(){
		register double r = sqrt(x*x + y*y + z*z );
		register double s = r == 0 ? 0 : 1 / r;
		x *= s;
		y *= s;
		z *= s;
		return r;
	}
};

XYZPoint XYZdifference( XYZPoint & a, XYZPoint & b ){
	return XYZPoint( b.x - a.x, b.y - a.y, b.z - a.z );
}

double XYZdistance( XYZPoint & a, XYZPoint & b ){
	XYZPoint d = XYZdifference( a, b );
	return sqrt( d.x * d.x + d.y * d.y + d.z * d.z );
}

XYZPoint CrossProduct (XYZPoint & v1, XYZPoint & v2){
	XYZPoint cp;
// (verified with x=yzzy rule)
	cp.x = v1.y * v2.z - v1.z * v2.y;
	cp.y = v1.z * v2.x - v1.x * v2.z;
	cp.z = v1.x * v2.y - v1.y * v2.x;
	return(cp);
}

double DotProduct (XYZPoint & v1, XYZPoint & v2){
	return((v1.x*v2.x + v1.y*v2.y + v1.z*v2.z));
}

double PointPointDist(double x1, double y1, double x2, double y2){
        double d1 = x1 - x2;
        double d2 = y1 - y2;
        return sqrt(d1*d1+d2*d2);
}

/* The fn called by levmar to compute model values.
   In this case they are just the sqare roots of the 
   error sums for the lines.
*/
static int param_count = 0; // DEBUG CHECK

void Mapto3d (double *p, double *x, int m, int n, void *data){

  // get ref to the lensFunc
	lensFunc & LF = *(lensFunc *)data;

  // copy in the updated parameters
	int pct = LF.setOptParams( p );
	assert( pct == param_count );

	double x_cen = 0.5 * original_width,
		   y_cen = 0.5 * original_height;
	double invert_size_factor = 1.0/sizefactor;

  // loop over lines
	for (unsigned int l = 0; l < lines.size(); l++){	
		
		int npnts = lines[l].size();
		XYZPoint * mapped_point = new XYZPoint[npnts];

		// Map points to 3D and fill the above array with XYZPoints
  		for(unsigned int i = 0; i < npnts; i++){

			double x_point = invert_size_factor * lines[l][i]->x;
			double y_point = invert_size_factor * lines[l][i]->y;

		// Move coordinates so 0,0 is in the centre of the image
			x_point -= x_cen;
			y_point -= y_cen;						
 			y_point *= -1;


		// map image coords to spherical cartesian
			double xyz[3];
			LF.cart3d_photo( x_point, y_point, xyz );
			XYZPoint Point(xyz);
		    if( Point.normalize() == 0 )
				i = i;
		
			mapped_point[i] = Point;
			
		}
	  // compute straightness error for this line
		// Vector cross product of the end points - this is the normal to the estimated plane
		XYZPoint & first = mapped_point[0],
			     & last = mapped_point[npnts - 1];
		XYZPoint norm = CrossProduct( first, last);
		if( norm.normalize() == 0 )
			npnts = lines[l].size();

		// Compute straigthness error
		double ssq = 0;
		for(unsigned int i = 1; i < npnts - 1; i++){			
			// Add square of dot product of that point with norm
			double d = DotProduct(norm,mapped_point[i]);
        		ssq += d * d;			
		}
		// add to optimizer's error vector
		x[l] = sqrt(ssq) ; 
		line_errors[l] = sqrt(ssq / (npnts - 2));  // save rms error 
		
		delete [] mapped_point;
	}
}


// Progress reporter to be called by levmar
int visf (double *p, double *hx, int m, int n, int iter, double p_eL2, void *adata){
	return(1);
}

/* Optimization main line 
  sets up the lens model, runs levmar and reports results.
*/

void map_points(){

	static char * LMreason[] = {
	  " 1 - stopped by small gradient J^T e ",
      " 2 - stopped by small Dp ",
      " 3 - stopped by itmax ",
      " 4 - singular matrix. Restart from current p with increased mu ",
	  " 5 - no further error reduction is possible. Restart with increased mu ",
      " 6 - stopped by small ||e||_2 ",
      " 7 - stopped by user "
	};
	static char * infodesc[] = {
		"start ||e||_2 ",
		"final ||e||_2 ",
		"||J^T e||_inf ",
		"||Dp||_2      ",
		"mu/max[J^T...]",
		"iterations    ",
		"stop reason   ",
		"function evals",
		"Jacobian evals"
	};
  /* create a lensFunc with default initial parameters
  */ 
	lensFunc LF( lensFunc::digicam, 
		         original_width, original_height, // should be sensor dims 
				 pixel_density, pixel_density,
				 lensFunc::lens_kind( lens_type ),
				 focal_length, 180 );

	printf("Camera %s h %d (%.1f/mm) v %d (%.1f/mm)\n",
		    LF.camDesc(), int(original_width), pixel_density,
				int(original_height), pixel_density );
    printf("Lens %s FL %.1f mm (%.1f pixels)\n",
			 LF.lensDesc(), LF.getFL_mm(), LF.avgFL_pix() );
	const char * perm = LF.errMsg();
	if( perm ){
		printf("ERROR lensFunc: %s\n", perm );
		return;
	}

	/* select the parameters to be optimized
	*/

	int m; // number of parameters to be optimized
	m = LF.setOptPurpose( lensFunc::calibrate, 
						  optimise_radius,
						  optimise_centre 
						);
	param_count = m;  //DEBUG
  // copy out initial parameter values
	double * p = new double[m];	
	m = LF.getOptParams( p ); 
	assert( m == param_count );

	int iterations = 3000;
	double info[LM_INFO_SZ];

	// Set up measurement vector
	int n = lines.size();	

	double * x = new double[n];
	for (unsigned int i = 0; i < n; i++){
		x[i] = 0;
	}	

	int ret;
	ret = dlevmar_dif(&Mapto3d, NULL, p, x, m, n, iterations, NULL, info, NULL, NULL, (void *)&LF);
	
	delete [] p;
	delete [] x;

	printf("\nLevenberg-Marquardt returned in %g iterations\nreason %s\n", 
		    info[5], LMreason[int(info[6])-1]);
	// Print levmar info
	for (int i = 0; i <=7; i++){
		printf("    %s:\t%g\n", infodesc[i], info[i] );
	}
	printf("\n");
	printf("Fitted parameters...\n");
	if( lens_type > lensFunc::orthographic ){
	    double sina, tana, tanf;
		LF.get_w_proj_params( sina, tana, tanf );
		printf("  Sine 'a' %g  tangent 'a' %g  tangent fraction %g\n",
					sina, tana, tanf );
	}
	if( optimise_centre ){ 
		double d, e;
		LF.get_w_c_shift( d, e );
		printf("  Centre shifts: %.2f  %.2f pixels\n", d, e );
	}
	if( optimise_radius ){
		double rp[7];	
		LF.get_w_rad_params( rp );
		double rms = 0;
		for(int i = 0; i < 7; i++){
			register double d = rp[i];
			rms += d * d;
		}
		rms = sqrt( rms / 7);
		printf("  Radial mean sq. dev. %.4f  factors: \n", rms );
		for( int i = 0; i < 7; i++ ) printf("%8.4f", rp[i] );
		printf("\n");
	}	

	if(verbose){
		cout << endl;
		for (unsigned int i = 0; i < n; i++){
			cout << "Line " << i+1 << " rms:\t" << line_errors[i] << endl;
		}
	}
	printf("\n");
		 
 }

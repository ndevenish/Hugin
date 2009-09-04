/** lensFunc.cpp  for calibrate_lens  28Jul09 TKS  **/

/***************************************************************************
 *   Copyright (C) 2009 Thomas K Sharpless  <tksharpless@gmail.com>        *
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
#include <assert.h>

#include "lensFunc.h"
#include "HermiteSpline.h"
#include <math.h>

/* the C API visible to libpano contains these glue routines, that
   are declared in lensFunc_glue.h.  One parameter is a handle 
   (pointer) to an instance of lensFunc.

*/
// PT remapping stack functions called from fDescs
// can also be called directly with params = handle
extern "C" 
int LF_photo_sphere ( double x_dest, double  y_dest, double* x_src, double* y_src, void* params ){
	return ((lensFunc *)params)->photo_sphere( x_dest, y_dest, x_src, y_src );
}

extern "C" 
int LF_sphere_photo ( double x_dest,double  y_dest, double* x_src, double* y_src, void* params ){
	return ((lensFunc *)params)->sphere_photo( x_dest, y_dest, x_src, y_src );
}

// Optimization parameter funcs called via handle
extern "C" 
int LF_setOptPurpose( void * handle, int purpose ){
	return ((lensFunc *)handle)->setOptPurpose( lensFunc::opt_purpose(purpose) );
}

extern "C" 
int LF_getOptParams( void * handle, double * ppa ){	
	return ((lensFunc *)handle)->getOptParams( ppa );
}

extern "C" 
int LF_setOptParams( void * handle, double * ppa ){
	return ((lensFunc *)handle)->setOptParams( ppa );
}

/* local definitions
*/ 
#ifndef Pi
  #define Pi 3.1415926535897932384626433832795
  #define DEG2RAD( x ) ((x) * Pi/180.0 )
  #define RAD2DEG( x ) ((x) * 180.0 / Pi )
#endif

/* map a finite parameter range to infinite optimizer
   range via the tangent function
*/
static double to_inf_opt( double pv, double lwr, double upr ){
	register double  s = Pi / (upr - lwr),
					 m = 0.5 * (upr + lwr);
	return tan( s * ( pv - m ));
}

static double from_inf_opt( double ov, double lwr, double upr ){
	register double  s = (upr - lwr) / Pi,
					 m = 0.5 * (upr + lwr);
	return m + s * atan( ov );
}

/*  lensFunc member functions
*/
lensFunc::lensFunc() 
  : model_valid (0), spline_valid (0), color(1),
    errmsg(0)
{}

lensFunc::lensFunc( camera_kind c_type,
			  int hPixels, int vPixels,
		      double hPixmm, double vPixmm,
			  lens_kind l_type, double FLmm,
			  double cropRadPix )
{
	errmsg = setCamLens( c_type, hPixels, vPixels,
				hPixmm, vPixmm, 
				l_type, FLmm, cropRadPix );

}

#define fail( s ) {errmsg = s; return errmsg;}

const char * lensFunc::setCamLens( camera_kind c_type,
			  int hPixels, int vPixels, double hPixmm, double vPixmm,
			  lens_kind l_type, double FLmm, double cropRadPix )
{
	model_valid = spline_valid = false;
	color = 1;	// default green, tca factor == 1
	opt_rad_k = 0;
	opt_center = opt_scale = opt_tca = false;
	camera = no_cam;
	lens = no_lens;
	set_f_scale( 1, 1 );
	set_c_shift( 0, 0 );
	set_tca_params( 1, 1 );
	set_rad_params( 0 );
  // validate arguments
	if( c_type < digicam || c_type > scanner )
		fail("invalid camera type" );
	if( hPixels < 1 || vPixels < 1 ) 
		fail("invalid image size");
	if( hPixmm < 1 || vPixmm < 1 ) 
		fail("invalid pixel spacing");
	if( l_type < rectilinear || l_type > universal_model ) 
		fail("invalid lens type");
	if( FLmm < 0.01 ) 
		fail("invalid focal length");

  // post fixed params
	camera = c_type;
	hPix_num = hPixels; vPix_num = vPixels;
	hPix_mm = hPixmm; vPix_mm = vPixmm;
	lens = l_type;
	FL_mm = FLmm;

/* sanity check model... */
  // set basic derived params
	hc_s_lim = 0.5 * double(hPix_num + 1);
	vc_s_lim = 0.5 * double(vPix_num + 1);
	hFLpix = FL_mm * hPix_mm;
	vFLpix = FL_mm * vPix_mm;
	double dx = hc_s_lim / hFLpix,
		   dy = vc_s_lim / vFLpix;
	Rmaxrad = sqrt( dx*dx + dy*dy );  // crop radius / fl
  // 
	double trad = cropRadPix / vFLpix;
	if( trad == 0 || trad > Rmaxrad ) trad = Rmaxrad;

	double tfov = 0;
	switch( l_type ){
	case rectilinear: 
		tfov = atan( trad );
		break;
	case equal_angle:
		tfov = trad;
		break;
	case equal_area:
		if( trad <= 2 )
			tfov = 2 * asin( 0.5 * trad );
		if( tfov == 0 )
			fail( "equal-area FOV too large" );
		break;
	case stereographic:
		tfov = 2 * atan( 0.5 * trad );
		break;
	case orthographic:
		if( trad <= 1 )
			tfov = asin( trad );
		if( tfov == 0 )
			fail( "orthographic FOV too large" );
		break;
	case fisheye_model:
	case universal_model:
		tfov = 0.5 * Pi;
		break;
	}
	
  // post default projection params 
	model_valid = true;  // enables setSpline
	set_proj_params( 2, 2, 0.5 );
	min_tana = 1;
	w_proj_u = 0.5;
	w_proj_v = 0.5;

	if( !spline_valid )
		fail( "non-monotonic model" );
	
	fail( 0 );

}

// set primary and working parameter values
bool lensFunc::set_f_scale( double h_scl, double v_scl ){
	f_scale[0] = w_f_scale[0] = h_scl;
	f_scale[1] = w_f_scale[1] = v_scl;
	return setSpline();
}

bool lensFunc::set_c_shift( double h_shfmm, double v_shfmm ){
	c_shift[0] = w_c_shift[0] = h_shfmm;
	c_shift[1] = w_c_shift[1] = v_shfmm;
	return setSpline();
}

bool lensFunc::set_proj_params( double sina, double tana, double tanf ){
	proj_sina = w_proj_sina = sina;
	proj_tana = w_proj_tana = tana;
	proj_tanf = w_proj_tanf = tanf;
	return setSpline();
}

// special case: pp == 0 clears to zero
bool lensFunc::set_rad_params( double pp[7] ){
	for(int i = 0; i < 7; i++ ){
		register double v = pp ? pp[i] : 0;
		rad_params[i] = w_rad_params[i] = v;
	}
	return setSpline();
}

bool lensFunc::set_tca_params( double RperG, double BperG ){
	tca_params[0] = w_tca_params[0] = RperG;
	tca_params[1] = w_tca_params[1] = BperG;
	return setSpline();
}

  // get primary parameter values
void lensFunc::get_f_scale( double& h_scl, double& v_scl ){
	h_scl = f_scale[0]; v_scl = f_scale[1];
}

void lensFunc::get_c_shift( double& h_shfmm, double& v_shfmm ){
	h_shfmm = c_shift[0]; v_shfmm = c_shift[1];
}

void lensFunc::get_proj_params( double & sina, double & tana, double & tanf ){
	sina = proj_sina;
	tana = proj_tana;
	tanf = proj_tanf;
}

void lensFunc::get_tca_params( double& RperG, double& BperG ){
	RperG = tca_params[0]; BperG = tca_params[1];
}

void lensFunc::get_rad_params( double pp[7] ){
	for(int i = 0; i < 7; i++ ){
		pp[i] = rad_params[i];
	}
}


  // get working (optimizable) parameter vlaues
void lensFunc::get_w_f_scale( double& h_scl, double& v_scl ){
	h_scl = w_f_scale[0]; v_scl = w_f_scale[1];
}

void lensFunc::get_w_c_shift( double& h_shfmm, double& v_shfmm ){
	h_shfmm = w_c_shift[0]; v_shfmm = w_c_shift[1];
}

void lensFunc::get_w_proj_params( double & sina, double & tana, double & tanf ){
	sina = w_proj_sina;
	tana = w_proj_tana;
	tanf = w_proj_tanf;
}

void lensFunc::get_w_tca_params( double& RperG, double& BperG ){
	RperG = w_tca_params[0]; BperG = w_tca_params[1];
}

void lensFunc::get_w_rad_params( double pp[7] ){
	for(int i = 0; i < 7; i++ ){
		pp[i] = w_rad_params[i];
	}
}

/* coordinate mapping API */

bool lensFunc::setColorChannel( int rgb ){
	if( rgb < 0 || rgb > 2 ) return false;
	color = rgb;
	return setSpline();
}

// build libpano fDesc's
int lensFunc::fD_photo_sphere( fDesc * pfd ){
	pfd->func = LF_photo_sphere;
	pfd->param = (void *) this;
	return 1;
}

int lensFunc::fD_sphere_photo( fDesc * pfd ){
	pfd->func = LF_sphere_photo;
	pfd->param = (void *) this;
	return 1;
}

/* compute the image <=> equal angle fisheye mappings */
// centered image coords to spherical fisheye
int lensFunc::sphere_photo( double x_phot, double  y_phot, double* x_sphr, double* y_sphr ){
	*x_sphr = *y_sphr = 0;
	if( !spline_valid ) return 0;
	double R;
	register double x = (x_phot - w_c_shift[0]) / w_hFLpix,
					y = (y_phot - w_c_shift[1]) / w_vFLpix,
					r = sqrt( x * x + y * y );
	if( !spline_hermite_val( NSpts, r_val, Cr2a, r, &R, 0 )) return 0;
	if( r > 0 ) R /= r;
	*x_sphr = x * R;
	*y_sphr = y * R;
	return 1;
}

// map spherical fisheye to centered image coordinates
int lensFunc::photo_sphere( double x_sphr, double  y_sphr, double* x_phot, double* y_phot ){
	*x_phot = *y_phot = 0;
	if( !spline_valid ) return 0;
	double r;
	double R = sqrt( x_sphr * x_sphr + y_sphr * y_sphr );
	if( !spline_hermite_val( NSpts, a_val, Ca2r, R, &r, 0 )) return 0;
	if( R > 0 ) r /= R;
	*x_phot = x_sphr * r * w_hFLpix + w_c_shift[0];
	*y_phot = y_sphr * r * w_vFLpix + w_c_shift[1];
	return 1;
}
  
/* compute image <=> Cartesian unit sphere mappings */
// map image x, y to Cartesian unit sphere 
int lensFunc::cart3d_photo( double x_phot, double  y_phot, double xyz_sphr[3] ){
	xyz_sphr[0] = xyz_sphr[1] = xyz_sphr[2] = 0;
	if( !spline_valid ) return 0;
	double R;
	register double x = (x_phot - w_c_shift[0]) / w_hFLpix,
					y = (y_phot - w_c_shift[1]) / w_vFLpix,
					r = sqrt( x * x + y * y );
	if( !spline_hermite_val( NSpts, r_val, Cr2a, r, &R, 0 )) return 0;
	xyz_sphr[2] = cos( R );
	register double s = sin( R );
	if( r > 0 ) s /= r;
	else
		s = 0;
	xyz_sphr[0] = x * s;
	xyz_sphr[1] = y * s;
	return 1;
}

// map Cartesian unit sphere to image x, y 
int lensFunc::photo_cart3d( double xyz_sphr[3], double* x_phot, double* y_phot ){
	*x_phot = *y_phot = 0;
	if( !spline_valid ) return 0;
	double r;
	double R = acos( xyz_sphr[2] );
	if( !spline_hermite_val( NSpts, a_val, Ca2r, R, &r, 0 )) return 0;
	double s = sin( R );
	if( s > 0 ) r /= s;
	*x_phot = xyz_sphr[0] * r * w_hFLpix + w_c_shift[0];
	*y_phot = xyz_sphr[1] * r * w_vFLpix + w_c_shift[1];
	return 1;
}

/* round-trip tests for verifying internal computations
   return inv(fwd(arg)) - arg or fwd(inv(arg)) - arg.
*/

// currently valid max arguments (or 0)

double lensFunc::test_Tr_max(){
	if( !model_valid ) return 0;
	return w_maxrad;
}

double lensFunc::test_Ta_max(){
	if( !model_valid ) return 0;
	return AofR( w_maxrad );
}

double lensFunc::test_Sr_max(){
	if( !spline_valid ) return 0;
	return r_val[NSpts - 1];
}

double lensFunc::test_Sa_max(){
	if( !spline_valid ) return 0;
	return a_val[NSpts - 1];
}

// round trip tests of trig fns

double lensFunc::test_Trar( double v ){
	return RofA(AofR(v)) - v;
}

double lensFunc::test_Tara( double v ){
	return AofR(RofA(v)) - v;
}
// round trip tests of spline tables

double lensFunc::test_Srar( double v ){
	double t1 = 0, t2 = 0;
	if( spline_hermite_val( NSpts, r_val, Cr2a, v, &t1, 0 )){
		if( spline_hermite_val( NSpts, a_val, Ca2r, t1, &t2, 0 )){
			return t2 - v;
		}
	}
	return -v;
}

double lensFunc::test_Sara( double v ){
	double t1 = 0, t2 = 0;
	if( spline_hermite_val( NSpts, a_val, Ca2r, v, &t1, 0 )){
		if( spline_hermite_val( NSpts, r_val, Cr2a, t1, &t2, 0 )){
			return t2 - v;
		}
	}
	return -v;
}

/* parameter optimization API 
*/
int lensFunc::setOptPurpose( opt_purpose p,
							 int optRad,
							 bool optCen,
							 bool optScl,
							 bool optTca ){
	if( !model_valid ) return 0;
	int cnt = 0;

	if( optRad < 0 ) optRad = 0;
	else if( optRad > 4 ) optRad = 4;
	if( optRad ) cnt += radfn.getParamCount( optRad );
	opt_rad_k = optRad;

	opt_center = optCen;
	if( opt_center ) cnt += 2;

	opt_scale = optScl;
	if( opt_scale ) cnt += 2;

	opt_tca = optTca;
	if( opt_tca && color != 1 ) ++cnt;

	if( lens > orthographic ) {
		++cnt;
		if( lens == universal_model ) ++cnt;
	}
	return cnt;
}

// export optimizer parameter vector
int lensFunc::getOptParams( double * ppa ){
	if( !spline_valid ) return 0;
	int n = 0;
  // proj params first
 	if( lens > orthographic ) {
		if( lens == fisheye_model ){
			ppa[n++] = to_inf_opt( w_proj_u, 0, 1 );
		} else if( lens == universal_model ){
			ppa[n++] = to_inf_opt( w_proj_u, 0, 1 );
			ppa[n++] = to_inf_opt( w_proj_v, 0, 1 );
		}
	}

	if( opt_rad_k ){
	// note use nonlinear interface
		n += radfn.getInfParams( ppa, opt_rad_k );
	}

	if( opt_center ){
		ppa[n++] = to_inf_opt( w_c_shift[0], -hc_s_lim, hc_s_lim );
		ppa[n++] = to_inf_opt( w_c_shift[1], -vc_s_lim, vc_s_lim );
	}

	if( opt_scale ){
		ppa[n++] = w_f_scale[0];
		ppa[n++] = w_f_scale[1];
	}

	if( opt_tca ){
		if( color == 0 ) ppa[n++] = w_tca_params[0];
		if( color == 2 ) ppa[n++] = w_tca_params[1];
	}

	return n;
}

// import optimizer parameter vector
int lensFunc::setOptParams( double * ppa ){
	if( !spline_valid ) return 0;
	int n = 0;

  // proj params first
 	if( lens > orthographic ) {
		if( lens == fisheye_model ){
			w_proj_u = from_inf_opt( ppa[n++], 0, 1 );
		} else if( lens == universal_model ){
			w_proj_u = from_inf_opt( ppa[n++], 0, 1 );
			w_proj_v = from_inf_opt( ppa[n++], 0, 1 );
		}
		setup_proj();
	}

	if( opt_rad_k ){
	// load subset via the nonlinear interface
		n += radfn.setInfParams( ppa, opt_rad_k );
	// copy all via the linear interface
		radfn.getParams( w_rad_params, 3 );
	}

	if( opt_center ){
		w_c_shift[0] = from_inf_opt( ppa[n++], -hc_s_lim, hc_s_lim );
		w_c_shift[1] = from_inf_opt( ppa[n++], -vc_s_lim, vc_s_lim );
	}

	if( opt_scale ){
		w_f_scale[0] = ppa[n++];
		w_f_scale[1] = ppa[n++];
	}

	if( opt_tca ){
		if( color == 0 ) w_tca_params[0] = ppa[n++];
		if( color == 2 ) w_tca_params[1] = ppa[n++];
	}

	if( setSpline() ) return n;
	return 0;		// setSpline() failed!
}

// set projection parameters from w_proj_u, wproj_v
void lensFunc::setup_proj(){
	w_proj_sina = w_maxrad / w_proj_v;
	w_proj_tana = w_maxrad / w_proj_v;
	w_proj_tanf = 0.5 + w_proj_v * ( w_proj_u - 0.5 ); 
}

/* check the model; if OK tabulate radial mapping.

   false if model not valid or mapping not monotonic.
   model_valid must be true at call, and is not changed

   Posts w_hFLpix, w_vFLpix, w_maxrad, min_sina (& maybe
   sina), spline_valid.

 */

bool lensFunc::setSpline(){
	spline_valid = false;
	if( !model_valid ) 
		return false;
/* calculate some derived parameters here, as the F.L.
   scale factors may have changed. Must validate
   sin 'a' factor too.
 */
	w_hFLpix = hFLpix * w_f_scale[0];
	w_vFLpix = vFLpix * w_f_scale[1];
	double dx = 0.5 * double(hPix_num + 1) / w_hFLpix,
		   dy = 0.5 * double(vPix_num + 1) / w_vFLpix;
	w_maxrad = sqrt( dx*dx + dy*dy );  // crop radius

// post new min_sina, adjust sina if necessary
	min_sina = w_maxrad > 1 ? w_maxrad : 1;
	if( w_proj_sina < min_sina ) 
		w_proj_sina = w_proj_tana = min_sina;

// set up the radius correction function
// note use the linear interface
	radfn.setParams( w_rad_params, 3 );

// fill the tables
    int i;
    double * x = r_val, 
		   * y = a_val, 
		   * t = new double[ NSpts ];
// tabulate angle vs radius/fl
	double dr1 = 1.0 / (NSpts - 1);
	for( i = 0; i < NSpts; i++ ){
		register double r = i * dr1;		// radius 0:1
		x[i] = r * w_maxrad;	// radius 0:maxrad
		register double R = radfn.yofx( r );	// corrected 0:1
		R *= w_maxrad;
		y[i] = AofR( R );	// to angle
	}

// set up spline table
  // tangents for radius=>angle
	i = spline_tangents_set( NSpts, x, y, t );
  // fail if y is not monotonic
	if( i != 1 ){
		return false;
	}
  // coefficients for radius=>angle
	spline_hermite_set ( Cr2a, NSpts, x, y, t );
  // tangents for angle=>radius
	i = spline_tangents_set( NSpts, y, x, t );
  // coefficients for angle=>radius
	spline_hermite_set ( Ca2r, NSpts, y, x, t );

	delete [] t;
	spline_valid = true;
	return true;
  }


/* partial radial mappings according to model 
*/

double lensFunc::AofR( double R ){
	switch( lens ){
	case rectilinear:
		return atan( R );
		break;
	case stereographic:
		return 2 * atan( 0.5 * R );
		break;
	case equal_angle:
		return  R ;
		break;
	case equal_area:
		return 2 * asin( 0.5 * R );
		break;
	case orthographic:
		return asin( R );
		break;
	case fisheye_model:
	case universal_model:
		double at = w_proj_tana * atan( R / w_proj_tana );
		double as = w_proj_sina * asin( R / w_proj_sina );
		return w_proj_tanf * at + (1 - w_proj_tanf) * as;
		break;
	}
	return 0;
}

double lensFunc::RofA( double A ){
	switch( lens ){
	case rectilinear:
		return tan( A );
		break;
	case stereographic:
		return 2 * tan( 0.5 * A );
		break;
	case equal_angle:
		return  A ;
		break;
	case equal_area:
		return 2 * sin( 0.5 * A );
		break;
	case orthographic:
		return sin( A );
		break;
	case fisheye_model:
	case universal_model:
	/* Can't invert the weighted average,
	   so use Newton's method to find R
	*/
		double R = A;	// guess
		int n = 200;
		for(;;) {
			double a = AofR( R ) - A;
			if( fabs( a ) < 3.e-11 ) 
				return R;	// normal exit
			if( --n == 0 )	// iteration limit (shd not occur)
				return R;
			double ct = cos(R/w_proj_tana),
				   cs = cos(R/w_proj_sina);
			if( ct <= 0 )	// infeasible (shd not occur)
				return R;
			double q = (cs + 1 / (ct * ct )); // da / dR
			R -= a / q;
		}
		break;
	}
	return 0;
}

/* Printable descriptions of current model
*/
const char * lensFunc::camDesc(){
	static char * cd[] = {
		"no camera",
		"digicam",
		"slit cam",
		"film scanner"
	};
	int c = int(camera);
	if( c < 0 || c > sizeof( cd ) / sizeof(char *) ) c = 0;
	return cd[c];
}

const char * lensFunc::lensDesc(){
	static char * ld[] = {
		"no lens",
		"rectilinear",
		"equal_angle",
		"equal_area",
		"stereographic",
		"orthographic",
		"fisheye_model",
		"universal_model"
	};
	int c = int(lens);
	if( c < 0 || c > sizeof( ld ) / sizeof(char *) ) c = 0;
	return ld[c];
}

/* ssifn --
*/

ssifn::ssifn( int k ){
	if( k < 0 ) k = 0;
	else if( k > 16 ) k = 16;
	kmax = k;
	npts = (1 << k) + 1;
	para = new double[ 11 * npts ];
	xval = &para[npts];
	yval = &para[2 * npts];
	Cxy = &para[3 * npts];
	Cyx = &para[7 * npts];
  // initialize to straight line
	for( int i = 0; i < npts; i++ ) para[i] = 0;
	setSpline();
}

ssifn::~ssifn(){
	delete[] para;
}

int ssifn::getParamCount( int k ){
	if( k <= 0 || k >= kmax ) k = kmax;
	return ( 1 << k ) - 1;
}

int ssifn::getParams( double * p, int k ){
	if( k <= 0 || k >= kmax ) k = kmax;
	int j = 1 << (kmax - k);
	int n = 0;
	for( int i = j; i < npts - 1; i += j ){
		*p++ = para[i];
		++n;
	}
	return n;
}

int ssifn::setParams( double * p, int k ){
	if( k <= 0 || k >= kmax ) k = kmax;
	int j = 1 << (kmax - k);
	int n = 0;
	for( int i = j; i < npts - 1; i += j ){
		register double t = *p++;
		if( t < -0.5 ) t = -0.5;
		else if( t > 0.5 ) t = 0.5;
		para[i] = t;
	}
	if( !setSpline() ) n = 0;
	return n;
}

int ssifn::getInfParams( double * p, int k ){
	if( k <= 0 || k >= kmax ) k = kmax;
	int j = 1 << (kmax - k);
	int n = 0;
	for( int i = j; i < npts - 1; i += j ){
		register double t = para[i];
		*p++ = tan( t * Pi );
		++n;
	}
	return n;
}

int ssifn::setInfParams( double * p, int k ){
	if( k <= 0 || k >= kmax ) k = kmax;
	int j = 1 << (kmax - k);
	int n = 0;
	for( int i = j; i < npts - 1; i += j ){
		register double t = *p++;
		para[i] = atan( t ) * (1/Pi);
		++n;
	}
	if( !setSpline() ) n = 0;
	return n;
}

double ssifn::yofx( double x ){
	double y = 0;
	spline_hermite_val( npts, xval, Cxy, x, &y, 0 );
	return y;
}

double ssifn::xofy( double y ){
	double x = 0;
	spline_hermite_val( npts, yval, Cyx, y, &x, 0 );
	return x;
}

// recursion to fill xval, yval
void ssifn::sssub( int l, int r ){
	int m = (l+r)/2;
	if(m == l) return;	// break recursion
	register double dx = xval[r] - xval[l],
					dy = yval[r] - yval[l],
					p = para[m];
	xval[m] = xval[l] + dx * (0.5 - p);
	yval[m] = yval[l] + dy * (0.5 + p);
	sssub( l, m );	// do left subtree
	sssub( m, r );	// do right subtree
}

// call after posting new params
int ssifn::setSpline(){
	int i = npts - 1;
	xval[0] = yval[0] = 0;
	xval[i] = yval[i] = 1;
 // fill xval, yval arrays
	sssub( 0, i );
 // build spline tables
	double * t = new double[npts];
	i = spline_tangents_set( npts, xval, yval, t );
  // fail if y is not monotonic
	if( i != 1 ) 
		return 0;
  // coefficients for forward spline
	spline_hermite_set ( Cxy, npts, xval, yval, t );
  // tangents for inverse spline
	i = spline_tangents_set( npts, xval, yval, t );
  // coefficients for inverse spline
	spline_hermite_set ( Cyx, npts, yval, xval, t );
	delete [] t;

	return 1;
}

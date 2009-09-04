/* lensFunc.h		for calibrate_lens, 28July09 TKS

  This is a C++ header, not to be used in libpano source.
  In that context, use "lensFunc_glue.h"

  lensFunc encapsulates calibrated lens projection functions 
  in a form compatible with the PanoTools remapping function 
  stack and parameter optimizer.

  By providing what is essentially a single remapping function 
  for all lenses, and only for lenses, lensFunc can support a 
  more comprehensive set of lens models and a practical lens 
  calibration facility, both of which are sorely needed.  

  Remapping with lensFunc is also faster than with existing
  stack functions because it tabulates the radial part of the
  mapping as a cubic spine.  There are no evaluations of trig 
  functions or a radial polynomial during remapping, and
  forward and inverse mappings are equally fast.

  There are also functions to map image coordinates to/from
  points on the unit Cartesian sphere. The projection center 
  is the center of the sphere, and the image center is tangent 
  to the sphere at (0,0,1).  The sphere coordinates are right
  handed if image x runs right and y runs up -- caution: in
  most image formats, y runs down, which gives left handed
  3D coordinates.  These functions use the radial spline, but 
  do have to evaluate sin & cos, or arccos, as well.
 
  The C interface to libpano is very thin: two new remapping
  stack functions; two to copy lensFunc parameters to/from 
  an array of optimizer parameters, and one to select which set 
  of parameters is copied; one new format code; and a new void * 
  parameter table entry as a handle to an instance of lensFunc.

  The lensFunc parameters are expected to come from calibration
  procedures, via new API's separate from those used for the 
  traditional PT corrections.  It is intended that those APIs
  be developed, not inside libpano, but in the C++ contexts of 
  Hugin and other programs that use libpano.  
  
  The API for parameter optimization is compatible with libpano's
  but a bit more flexible.  The lensFunc basically decides what 
  parameters to make available to the optimizer, based on the 
  declared purpose of the optimization (lens calibration, 
  pano alignment, ...).  Two functions then copy that set of 
  values to and from an external double array during optimization.  

  All lens parameters and the center shifts can be adjusted via the 
  optimizer API.  Calibrated parameters are protected during routine 
  panorama alignment; however apparent fov and distortion can be 
  optimized via secondary parameters.

  During optimization, certain parameters need to be kept within
  limited ranges.  There is a flavor of levmar that can do that, 
  however it runs less than half as fast as the unconstrained
  version.  So lensFunc presents unrestricted versions of those 
  parameters to the optimizer, using the tangent function to map
  the limited range to [-infinity:infinity], and you can call the
  normal levmar-diff().
   
  The traditional radius correction polynomial causes all kinds
  of trouble for optimization, so lensFunc does not use one.  
  Instead it uses an adjustable spline, that is guaranteed to be 
  monotonic no matter what parameter values the optimizer chooses.
  The number of radius correction points can be 0, 1, 3, 7, or 15; 
  each is one optimizer parameter. Unlike polynomial coefficients 
  these parameters are stable under optimization.  Using more of 
  them improves detail but does not encourage wild excursions.

  The radius function is normalized to the maximum possible pixel
  radius, which is half the diagonal (PT's constant radius is half 
  the larger image dimension).

  lensFunc implements five "ideal" lens models: rectilinear, 
  stereographic, equal-angle, equal-area, orthographic; and two
  "generic" models: fisheye and universal.  
  
  The ideal models are not adjustable, and are only valid for 
  certain combinations of focal length, image diagonal, and field
  of view.  The generic models have adjustable scale limits, that 
  are set such that no illegal trig trig function arguments can be 
  generated. Those limits can change due to optimization of focal 
  length scale, otherwise they are fixed by the design parameters 
  given to setCamLens().

  The only optional setup parameter, the crop radius in pixels,
  defaults to half the diagonal of the image.  It needs to be
  specified when the true image circle is smaller than the 
  sensor diagonal, to ensure that the full angular field of the 
  lens can be mapped.  CAUTION: image coordinates outside the
  crop circle, or sphere coordinates outside the corresponding
  view cone, will likely cause numerical errors and must never
  be passed in for remapping.  Note in case of non-square
  pixels, give crop radius in vertical pixels.

  The generic fisheye model is a weighted average of two fixed 
  functions, nominally the stereographic and equal-area projections, 
  but with the angle scale set initially according to the ratio of 
  image diagonal to focal length.  Its parameter [0:1] adjusts the 
  fraction of the stereographic function in the mixture.

  The universal model is the same as the generic fisheye model
  but with the angle scale factor also adjustable.  It can thus
  approximate the rectilinear, orthographic and equal-angle as well 
  as the generic fisheye projections.  
  
  Other features of the calibration API: 

  Physical parameters are kept in standard physical units: mm for 
  lengths, degrees for angles.

  Wherever possible parameters are defined so that the value zero
  is either a reasonable default, or means "ignore this parameter".

  Lens and camera parameters are specified separately.  The basic
  lens parameters are focal length (mm), nominal projection
  function, and radial correction factors including TCA.  Basic 
  camera parameters are sensor size and resolution (pixels per mm, 
  each way) and center point shifts (mm).  This separation reflects 
  the reality that one lens might be used on several cameras, or that 
  an image might have been digitized from film.  We also cater for 
  cameras having non-square pixels, and slit-cameras having a lens 
  projection only on the vertical axis.

  Following the libpano termninology, we call the projection
  in which coordinates are given the "destination" and the one
  whose coordinates are to be computed the "source"; and name
  transformation functions <destination>_<source>.  But function 
  arguments are named after the projection they belong to.
   

Notes on the Derivation of a Universal Lens Model

	Photographic lenses have projection functions that lie between
	r = tan( a ) (rectilinear) and r = sin( a ) (orthographic). See
	http://www.bobatkins.com/photography/technical/field_of_view.html

	So we should be able to compose a universal lens model out of
	those two functions.  The simplest such model is a weighted
	average: r = p * tan(a) + (1-p) * sin(a).  But this is limited
	to a < 90 degrees, while some lenses have fov > 180 degrees.  To
	increase the fov we can scale the angle by a factor 1/k:
	  r = p * k * tan(a/k) + (1-p) * k * sin(a/k)
    Multiplying the radius by k normalizes the slope to 1 at a = 0
	which effectively holds the focal length constant.  When k = 2, 
	this is a weighted average of the stereographic and	azimuthal 
	equal-area projections, which is fine for most fish eye lenses.  
	But k must be 1 to fit rectilinear and orthographic lenses; and
	for the equal-angle projection, k must approach infinity.  As k 
	grows large, p loses effect, because the initial parts of the 
	sin and tan functions are very similar. 

	For fitting image data one uses the inverse model:
	  a = p * k * atan(r/k) + (1-p) * k * asin(r/k)
	In the sine function, k must not be allowed to get smaller
	than the actual maximum radius, as asin(r/k) is undefined 
	when r > k.  So maybe we need two different k values:
	  a = p * kt * atan(r/kt) + (1-p) * ks * asin(r/ks)
    with ks held >= rmax.  Does this mean we need 3 model 
	parameters?

	The answer has to be no.  Each of p, ks, kt has a "dead zone"
	where is has no effect: ks is dead when p = 1, kt is dead when
	p = 0, and p is dead when ks and kt are both large.  So they 
	are rather like the triangular coordinates of a 2D point.
	We can control them with 2 parameters, u and v, that range from 
	0 to 1:  p = 1/2 + u * (v - 1/2), ks = R/u, kt = 1/u.  Of course
	the lower limit on u must be just a hair positive to keep ks 
	and kt finite.
	  
	A big problem is how to estimate the actual field of view.
	Let A be the maximum angle of incidence = fov/2, and R be the
	corresponding maximum radius in the focal plane, divided by the
	focal length.  There is clearly an upper limit on R, namely 
	half the diagonal of the image sensor / FL. The model gives
	  A = p * k * atan( R/k ) + (1-p) * k * asin( R/k )
	But we can't compute A until we have fixed p and k by fitting
	the model to some data.  Moreover, the actual image circle may
	be smaller than the sensor diagonal.

*/

/***************************************************************************
 *   Copyright (C) 2009 Thomas K Sharpless                                 *
 *   tksharpless@gmail.com                                                 *
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

#ifndef _LENSFUNC_H
#define _LENSFUNC_H

/* ssifn -- 
  A simple strictly increasing adjustable function.

  Spans the range [0:1] in both x and y and can be
  read as y(x) or as x(y).

  The fn is a cubic spline curve, defined recursively in 
  terms of the number of levels, k. There are 2^^k 
  segments and 2^^k - 1 parameters. For k = 1 there is 1 
  parameter, that locates a point on the diagonal from 
  (0,1) to (1,0).  That point is the mutual corner of two 
  new boxes on whose diagonals two level 2 parameters
  locate new points.  That creates 4 boxes at level 3, and 
  so on. 

  The corner points are tabulated as x and y arrays and
  converted to a pair of monotonic Hermite splines that
  can be read as y(x) and x(y).

  The basic parameters range from -1/2 at the lower right
  corner of a box to 1/2 at the upper left corner.  For
  convenience in dealing with unconstrained optimizers,
  there is an interface that maps this range to the 
  tangent of an angle (-infinity at lwr rgt, +infinity
  at upper left).

  Initially all parameters are 0 (straight line).

  The fns to copy params in and out take an optional argument
  k to select a "leading subset", e.g. k = 1 just copies the
  center point parameter.  This k must be <= the k passed to
  c'tor and defaults to that value.  Note the params are
  copied in left to right order in any case.

*/

class ssifn {
public:
	ssifn( int k = 4 );	// default 15 adjustable points
	~ssifn();
	int getParamCount( int k = 0 );	// returns number of params
	int getParams( double * p, int k = 0 );	// returns count
	int setParams( double * p, int k = 0 );	// returns 1 ok 0 fail
	int getInfParams( double * p, int k = 0 );	// returns count
	int setInfParams( double * p, int k = 0 );	// returns 1 ok 0 fail
	double yofx( double x );
	double xofy( double y );
protected:
	int kmax;
	int npts;
	double * para;
	double * xval, * yval;
	double * Cxy, * Cyx;
	int setSpline();
	void sssub(int l, int r);
};

/* A libpano stack function transforms 'dest' coordinates to 'source' 
   coordinates according to an unspecified set of parameters.
   These definitions are from filter.h
*/
typedef int (*trfn)( double x_dest,double  y_dest, double* x_src, double* y_src, void* params );
// Function descriptor to be executed by exec_function
struct fDesc {
	trfn	func;			// The function to be called
	void	*param;			// The parameters to be used
};		

class lensFunc {
public:
	typedef enum{
		no_lens = 0,
		rectilinear,
		equal_angle,
		equal_area,
		stereographic,
		orthographic,
		fisheye_model,
		universal_model
	} lens_kind;

	typedef enum{
		no_cam = 0,
		digicam,
		slitcam,
		scanner
	} camera_kind;

	typedef enum{
		align = 0,
		calibrate
	} opt_purpose;

	lensFunc();
	lensFunc( camera_kind c_type,
			  int hPixels, int vPixels,		// sensor dimensions
		      double hPixmm, double vPixmm, // pixel spacings
			  lens_kind l_type, double FLmm,
			  double cropRadPix = 0
			);

	~lensFunc(){}

/* coordinate mapping API
   CAUTION: all image coordinates must be in a circle whose diameter
   is the diagonal of the sensor given to the c'tor or setCamLens().  
   Points outside that circle may cause errors.
*/

  /* put function and parameter pointers in a libPano fDesc
     Note these remain valid so long as the lens model is
	 not changed via the calibration API  */
	int fD_photo_sphere( fDesc * pfd );	// to real 
	int fD_sphere_photo( fDesc * pfd );	// to ideal
  /* compute the image <=> 2d sphere (equal angle fisheye) mappings */
	int photo_sphere( double x_sphr, double  y_sphr, double* x_phot, double* y_phot ); 
	int sphere_photo( double x_phot, double  y_phot, double* x_sphr, double* y_sphr );
  /* compute image <=> Cartesian unit sphere mappings */
	int photo_cart3d( double xyz_sphr[3], double* x_phot, double* y_phot ); 
  	int cart3d_photo( double x_phot, double  y_phot, double xyz_sphr[3] );
/* set the color channel to be mapped (selects a tca parameter) */
	bool setColorChannel( int rgb );
  /* parameter optimization API
    setOptPurpose() selects a set of parameters and returns their number
	  polyN:  0 -- no radial polynomial
	          1 -- default for lens model
			  2 -- even-order
			  3 -- cubic
			  4 -- quintic
	  optCen: true to optimize center offsets
	  optScl: true to optimize h and v scale factors
	  optTca: true to optimize tca factor for color
    get/setOptParams() copy the selected values out/in
  */
	int setOptPurpose( opt_purpose p,
		               int optRad = 2,	// detail level of radius correction
					   bool optCen = false,	// optimize center shifts
					   bool optScl = false,	// optimize fov
					   bool optTca = false );	// optimize a tca param
	int getOptParams( double * ppa );	// copy to array
	int setOptParams( double * ppa );	// copy from array

/* Calibration API

    setCamLens() must be called first (typically from c'tor).  It
	returns a pointer to a text error message, or null if no error.
	
	Then you can either optimize the model, or reload optimized primary 
	parameters.  Don't load "made up" parameters.  
	
	After an optimization you can read out the working optimized values 
	and store them as primary if OK.

	Each "set" step yields a usable mapping function unless the
	passed parameter values are illegal, when these fns return false
	with all old values retained.

  */
	const char *
		setCamLens(  camera_kind c_type,
					  int hPixels, int vPixels,
					  double hPixmm, double vPixmm,
					  lens_kind l_type, double FLmm,
					  double cropRadPix = 0
					);

  // inquiry functions
	double getFL_mm(){ return FL_mm; }
	double avgFL_pix(){ return 0.5 * (hFLpix + vFLpix); }
	const char * errMsg(){ return errmsg; }	// null if no error
	const char * camDesc(); 
	const char * lensDesc();

  // translate internal radius function to/from external polynomial
//	bool getRadiusPoly( double * pc, int polyN, bool fwd = false );
//	bool setRadiusPoly( double * pc, int polyN, bool fwd = false );

  // set primary and working parameter values
	bool set_f_scale( double h_scl, double v_scl );
	bool set_c_shift( double h_shfmm, double y_shfmm );
  // use set_proj_params() only to reload optimized values!
	bool set_proj_params( double sina, double tana, double tanf );
	bool set_rad_params( double pp[7] );
	bool set_tca_params( double RperG, double BperG );
  // get primary parameter values
	void get_f_scale( double& h_scl, double& v_scl );
	void get_c_shift( double& h_shfmm, double& v_shfmm );
	void get_proj_params( double & sina, double & tana, double & tanf );
	void get_rad_params( double pp[7] );
	void get_tca_params( double& RperG, double& BperG );
  // get working (optimizable) parameter vlaues
	void get_w_f_scale( double& h_scl, double& v_scl );
	void get_w_c_shift( double& h_shfmm, double& v_shfmm );
	void get_w_proj_params( double & sina, double & tana, double & tanf );
	void get_w_rad_params( double pp[7] );
	void get_w_tca_params( double& RperG, double& BperG );

/* round-trip tests for verifying internal computations
   return inv(fwd(arg)) - arg or fwd(inv(arg)) - arg.
*/
  // currently valid max arguments
	double test_Tr_max();
	double test_Ta_max();
	double test_Sr_max();
	double test_Sa_max();
  // trig fns
	double test_Trar( double v );
	double test_Tara( double v );
  // spline tables
	double test_Srar( double v );
	double test_Sara( double v );

 private:
  /* primary design parameters */
	lens_kind lens;	// selects a model
	camera_kind camera;	// selects a model
	double FL_mm;		// nominal focal length
	double FOV_deg;		// nominal view cone angle
	int hPix_num, vPix_num;  // nominal image dimensions
	double hPix_mm, vPix_mm; // horizontal & vertical resolutions
    /* derived parameters */
	double hFLpix, vFLpix;	// focal lengths in pixels
    double Rmaxrad;    // largest valid R = radius/flpix
	/* derived limits */
	double min_sina;
	double min_tana;
	double hc_s_lim, vc_s_lim;

/* calibrated parameters   */
	double f_scale[2];	// h, v multipliers for FLmm
	double c_shift[2];	// optical center offsets
 	double rad_params[7];	// spline params
	double tca_params[2];	// red/grn, blu/grn radius
	double proj_sina;	// a in A = a * sin( R / a )
	double proj_tana;	// a in A = a * tan( R / a )
	double proj_tanf;	// fraction tan in model
  /* volatile working parameters (optimizable) 
     note real working rad params are inside radfn
	 note sina, tana, tanf are optimized indirectly
		via w_proj_u and w_proj_v.
  */
	double w_hFLpix, w_vFLpix;	// scaled focal lengths
	double w_maxrad;
	double w_f_scale[2];
	double w_c_shift[2];
 	double w_rad_params[7];	
	double w_tca_params[2];	
	double w_proj_sina;	
	double w_proj_tana;	
	double w_proj_tanf;	
  // actual projection params 
	double w_proj_u, w_proj_v;
	void setup_proj();  // applies w_proj_u, v

  /* state */
	const char * errmsg;
	bool model_valid;
	bool spline_valid;
	int color;	// 0: red, 1: green, 2: blue
  // these deternine which params get optimized...
	int opt_rad_k;	// number of radius fn levels, 0:4
	bool opt_center;	
	bool opt_scale;
	bool opt_tca;
  /* cubic spline tables */
#define NSpts 110
	double r_val[ NSpts ], a_val[ NSpts ];
	double Ca2r[ 4 * NSpts ], 
		   Cr2a[ 4 * NSpts ];
  // validate model and fill the tables
	bool setSpline();
  // partial radial functions
  // ideal fns computed direct from model parameters
	double AofR( double R );
	double RofA( double A );	
  /* internal radius correction function
    Note yofx() is real(ideal), xofy() is ideal(real); 
	they cover 0:1 in both x and y
  */
	ssifn radfn;
};

#endif	//ndef _LENSFUNC_H


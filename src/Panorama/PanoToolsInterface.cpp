// -*- c-basic-offset: 4 -*-

/** @file PanoToolsInterface.cpp
 *
 *  @brief implementation of PanoToolsInterface Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#include "panoinc.h"
#include "PT/PanoToolsInterface.h"

#include <utility>

using namespace std;
using namespace PT;
using namespace PTools;

using namespace vigra;
using namespace utils;


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
static void SetCorrectionRadius_copy( cPrefs *cP )
{
    double a[4];
    int i,k;
	
    for( i=0; i<3; i++ )
    {
        for( k=0; k<4; k++ )
        {
            a[k] = 0.0;//1.0e-10;
            if( cP->radial_params[i][k] != 0.0 )
            {
                a[k] = (k+1) * cP->radial_params[i][k];
            }
        }
        cP->radial_params[i][4] = smallestRoot_copy( a );
    }
}


Transform::Transform()
    : m_initialized(false), m_srcTX(0), m_srcTY(0),
      m_destTX(0), m_destTY(0)
{

}

Transform::~Transform()
{
    if (!m_initialized) {
        freeImage(m_srcImage);
        freeImage(m_dstImage);
    }
}

void Transform::updatePTData(const Diff2D &srcSize,
                             const PT::VariableMap & srcVars,
                             Lens::LensProjectionFormat & srcProj,
                             const Diff2D & destSize,
                             PanoramaOptions::ProjectionFormat & destProj,
                             double destHFOV)
{
    if (m_initialized) {
        freeImage(m_srcImage);
        freeImage(m_dstImage);
    }

    // fill our data into the Pano Tools structures.
    setFullImage(m_srcImage, srcSize, 0,
                 srcVars,  srcProj, true);
    setDestImage(m_dstImage, destSize, 0, destProj, destHFOV);
}


void Transform::createTransform(const Panorama & pano, unsigned int imgNr,
                                const PanoramaOptions & dest, Diff2D srcSize)
{
    const PanoImage & img = pano.getImage(imgNr);
    if (srcSize.x == 0 && srcSize.y == 0) {
        srcSize.x = img.getWidth();
        srcSize.y = img.getHeight();
    }
    createTransform(srcSize,
                    pano.getImageVariables(imgNr),
                    pano.getLens(img.getLensNr()).projectionFormat,
                    Diff2D(dest.width, dest.getHeight()),
                    dest.projectionFormat, dest.HFOV);
}


void Transform::createTransform(const Diff2D & srcSize,
                                const VariableMap & srcVars,
                                Lens::LensProjectionFormat srcProj,
                                const Diff2D &destSize,
                                PanoramaOptions::ProjectionFormat destProj,
                                double destHFOV)
{
    m_srcTX = destSize.x/2.0;
    m_srcTY = destSize.y/2.0;

    m_destTX = srcSize.x/2.0;
    m_destTY = srcSize.y/2.0;

    updatePTData(srcSize, srcVars, srcProj,
                 destSize, destProj, destHFOV);
    // create the actual stack
    SetMakeParams( m_stack, &m_mp, &m_srcImage , &m_dstImage, 0 );
}


void Transform::createInvTransform(const Panorama & pano, unsigned int imgNr,
                                const PanoramaOptions & dest, Diff2D srcSize)
{
    const PanoImage & img = pano.getImage(imgNr);
    if (srcSize.x == 0 && srcSize.y == 0) {
        srcSize.x = img.getWidth();
        srcSize.y = img.getHeight();
    }
    createInvTransform(srcSize,
                       pano.getImageVariables(imgNr),
                       pano.getLens(img.getLensNr()).projectionFormat,
                       Diff2D(dest.width, dest.getHeight()),
                       dest.projectionFormat, dest.HFOV);
}

void Transform::createInvTransform(const Diff2D & srcSize,
                                   const VariableMap & srcVars,
                                   Lens::LensProjectionFormat srcProj,
                                   const Diff2D & destSize,
                                   PanoramaOptions::ProjectionFormat destProj,
                                   double destHFOV)
{
    m_srcTX = srcSize.x/2.0;
    m_srcTY = srcSize.y/2.0;

    m_destTX = destSize.x/2.0;
    m_destTY = destSize.y/2.0;
    updatePTData(srcSize, srcVars, srcProj,
                 destSize, destProj, destHFOV);
    // create the actual stack
    SetInvMakeParams( m_stack, &m_mp, &m_srcImage , &m_dstImage, 0 );
}


void PTools::setDestImage(Image & image, Diff2D size,
                          unsigned char * imageData,
                          const PT::PanoramaOptions::ProjectionFormat & format,
                          double destHFOV)
{
    SetImageDefaults(&image);
    image.width = size.x;
    image.height = size.y;
    image.bytesPerLine = image.width*3;
    image.bitsPerPixel = 24;
    image.dataSize = image.height * image.bytesPerLine;
    // Allocate memory for pointer to pointer to image data
    image.data = (unsigned char**)malloc( sizeof(unsigned char*) );
    if(image.data == NULL) {
        DEBUG_FATAL("Out of memory");
    }
    *(image.data) = imageData;
    switch (format) {
    case PanoramaOptions::RECTILINEAR:
        image.format = _rectilinear;
        break;
    case PanoramaOptions::CYLINDRICAL:
        image.format= _panorama;
        break;
    case PanoramaOptions::EQUIRECTANGULAR:
        image.format = _equirectangular;
        break;
    }
    image.hfov = destHFOV;
}


void PTools::setFullImage(Image & image, Diff2D size,
                          unsigned char * imageData, const VariableMap & vars,
                          const PT::Lens::LensProjectionFormat format,
                          bool correctDistortions)
{
    SetImageDefaults(&image);
    image.width = size.x;
    image.height = size.y;
    image.bytesPerLine = image.width*3;
    image.bitsPerPixel = 24;
    image.dataSize = image.height * image.bytesPerLine;

    // Allocate memory for pointer to pointer to image data
    image.data = (unsigned char**)malloc( sizeof(unsigned char*) );
    if(image.data == NULL) {
        DEBUG_FATAL("Out of memory");
    }
    *(image.data) = imageData;

    image.dataformat = _RGB;
    switch (format) {
    case Lens::RECTILINEAR:
        image.format = _rectilinear;
        break;
    case Lens::PANORAMIC:
        image.format = _panorama;
        break;
    case Lens::CIRCULAR_FISHEYE:
        image.format = _fisheye_circ;
        break;
    case Lens::FULL_FRAME_FISHEYE:
        image.format = _fisheye_ff;
        break;
    case Lens::EQUIRECTANGULAR_LENS:
        image.format = _equirectangular;
        break;
    }
    image.hfov = const_map_get(vars,"v").getValue();
    image.yaw = const_map_get(vars,"y").getValue();
    image.pitch = const_map_get(vars,"p").getValue();
    image.roll = const_map_get(vars,"r").getValue();

    //fill cPrefs struct
    if (correctDistortions) {
        initCPrefs(image.cP,vars);
    }

    // no name
    image.name[0]=0;
    image.yaw = const_map_get(vars,"y").getValue();
    image.yaw = const_map_get(vars,"y").getValue();


    image.selection.top = 0;
    image.selection.left = 0;
    image.selection.right = image.width;
    image.selection.bottom = image.height;
}

void PTools::initCPrefs(cPrefs & p, const VariableMap &vars)
{
    SetCorrectDefaults(&p);

    double val;
    double a = const_map_get(vars,"a").getValue();
    double b = const_map_get(vars,"b").getValue();
    double c = const_map_get(vars,"c").getValue();
    if (a != 0.0 || b != 0.0 || c != 0) {
        p.radial = 1;
        p.radial_params[0][3] = p.radial_params[1][3] = p.radial_params[2][3] = a;
        p.radial_params[0][2] = p.radial_params[1][2] = p.radial_params[2][2] = b;
        p.radial_params[0][1] = p.radial_params[1][1] = p.radial_params[2][1] = c;
        double d = 1.0 - (a+b+c);
        p.radial_params[0][0] = p.radial_params[1][0] = p.radial_params[2][0] = d;
    } else {
        p.radial = 0;
    }

    val = const_map_get(vars,"e").getValue();
    if (val != 0.0) {
        p.vertical = TRUE;
        p.vertical_params[0] = p.vertical_params[1] = p.vertical_params[2] = val;
    } else {
        p.vertical = FALSE;
        p.vertical_params[0] = p.vertical_params[1] = p.vertical_params[2] = 0;
    }

    val = const_map_get(vars,"d").getValue();
    if (val != 0.0) {
        p.horizontal = TRUE;
        p.horizontal_params[0] = p.horizontal_params[1] = p.horizontal_params[2] = val;
    } else {
        p.horizontal = FALSE;
        p.horizontal_params[0] = p.horizontal_params[1] = p.horizontal_params[2] = 0;
    }
    // FIXME add shear parameters
    p.shear = FALSE;
    p.resize = FALSE;
    p.luminance = FALSE;
    p.cutFrame = FALSE;
    p.fourier = FALSE;

    // calculate correction radius
    // copied function from pano12.dll (missing under windows... strange)
    SetCorrectionRadius_copy(&p);

}


// ===========================================================================
// ===========================================================================


void PTools::createAdjustPrefs(aPrefs  & p, TrformStr & transf)
{
    SetAdjustDefaults(&p);
    p.interpolator = _nn;
    p.mode = _insert;

    SetImageDefaults(&(p.im));
    SetImageDefaults(&(p.pano));
}


// ===========================================================================
// ===========================================================================

// prepare a Trform struct for the adjust operation, image -> pano
void PTools::createAdjustTrform(TrformStr & trf)
{
    trf.src = (Image *) malloc(sizeof(Image));
    SetImageDefaults(trf.src);
    trf.dest = (Image *) malloc(sizeof(Image));
    SetImageDefaults(trf.dest);
    trf.success = TRUE;
    trf.tool = _adjust;
    trf.mode = _destSupplied | _honor_valid;
    trf.data = 0;
    trf.interpolator = _nn;
    trf.gamma = 1.0;
}


// free the resources associated with a TrformStr.
// createAdjustTrform must have been used to create it.
void PTools::freeTrform(TrformStr & trf)
{
    if (trf.dest) {
        if (trf.dest->data) {
            free(trf.dest->data);
        }
        free(trf.dest);
    }
    if (trf.src) {
        if (trf.src->data) {
            free(trf.src->data);
        }
        free(trf.src);
    }
}

void PTools::freeImage(Image &img)
{
    if (img.data) {
        free(img.data);
    }
}

void PTools::setAdjustSrcImg(TrformStr & trf, aPrefs & ap,
                             int width, int height, unsigned char * imageData,
                             const PT::VariableMap & vars,
                             const PT::Lens::LensProjectionFormat format,
                             bool correctDistortions)
{
    DEBUG_ASSERT(trf.src);
    if (trf.src->data) {
        free(trf.src->data);
    }
    setFullImage(*(trf.src), vigra::Diff2D(width,height), imageData, vars, format,
                 correctDistortions);
    ap.im = *(trf.src);
}

void PTools::setAdjustDestImg(TrformStr & trf, aPrefs & ap,
                              int width, int height, unsigned char * imageData,
                              const PT::PanoramaOptions & opts)
{
    DEBUG_ASSERT(trf.dest);
    if (trf.dest->data) {
        free(trf.dest->data);
    }
    setDestImage(*(trf.dest), vigra::Diff2D(width, height), imageData, opts.projectionFormat, opts.HFOV);
    ap.pano = *(trf.dest);
}


void PTools::setOptVars(optVars & opt, const std::set<std::string> & optvars)
{
    opt.hfov    = set_contains(optvars,"v") ? 1 : 0;
    opt.yaw     = set_contains(optvars,"y") ? 1 : 0;
    opt.pitch   = set_contains(optvars,"p") ? 1 : 0;
    opt.roll    = set_contains(optvars,"r") ? 1 : 0;
    opt.a       = set_contains(optvars,"a") ? 1 : 0;
    opt.b       = set_contains(optvars,"b") ? 1 : 0;
    opt.c       = set_contains(optvars,"c") ? 1 : 0;
    opt.d       = set_contains(optvars,"d") ? 1 : 0;
    opt.e       = set_contains(optvars,"e") ? 1 : 0;
    opt.shear_x = set_contains(optvars,"f") ? 1 : 0;
    opt.shear_y = set_contains(optvars,"g") ? 1 : 0;
}

PTools::AlignInfoWrap::AlignInfoWrap()
{
    gl.im  = NULL;
    gl.opt = NULL;
    gl.cpt = NULL;
    gl.t   = NULL;
    gl.cim = NULL;

    gl.numIm  = 0;
    gl.numPts = 0;
    gl.nt     = 0;
}

PTools::AlignInfoWrap::~AlignInfoWrap()
{
    DisposeAlignInfo(&gl);

    if (gl.im) free(gl.im);
    if (gl.opt) free(gl.opt);
    if (gl.cpt) free(gl.cpt);
    if (gl.t) free(gl.t);
    if (gl.cim) free(gl.cim);
}

bool PTools::AlignInfoWrap::setInfo(const PT::Panorama & pano,
                                    const UIntSet & imgs,
                                    const OptimizeVector & optvec)
{
    // based on code from ParseScript

    gl.im  = NULL;
    gl.opt = NULL;
    gl.cpt = NULL;
    gl.t   = NULL;
    gl.cim = NULL;

    // Determine number of images and control points
    gl.numIm 	= imgs.size();
    gl.nt 	= 0;

    // Allocate Space for Pointers to images, preferences and control points
	
    gl.im  = (Image*)	     malloc( gl.numIm 	* sizeof(Image) );
    gl.opt = (optVars*)	     malloc( gl.numIm 	* sizeof(optVars) );
    gl.t   = (triangle*)     malloc( gl.nt 	* sizeof(triangle) );
    gl.cim = (CoordInfo*)    malloc( gl.numIm 	* sizeof(CoordInfo) );

    if( gl.im == NULL || gl.opt == NULL || gl.cpt == NULL || gl.t == NULL || gl.cim == NULL )
    {
        DEBUG_FATAL("Not enough memory");
    }

    SetImageDefaults(&(gl.pano));	
    // Default: Use buffer 'buf' for stitching
    SetStitchDefaults(&(gl.st)); strcpy( gl.st.srcName, "buf" );
    for(int i=0; i<gl.numIm; i++)
    {
        SetImageDefaults( &(gl.im[i]) );
        SetOptDefaults	( &(gl.opt[i]));
        SetCoordDefaults( &(gl.cim[i]), i);
    }

    unsigned int cImgNr=0;
    for (UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it) {
        const PanoImage & pimg = pano.getImage(*it);
        const VariableMap & vars = pano.getImageVariables(*it);
        const Lens & lens = pano.getLens(pimg.getLensNr());
        
        // set the image information, with pointer to dummy image data
        setFullImage(gl.im[cImgNr],
                     Diff2D(pimg.getWidth(), pimg.getHeight()),
                     0, vars, lens.projectionFormat, true);

        // set optimisation flags
        setOptVars(gl.opt[cImgNr], optvec[cImgNr]);

        cImgNr++;
    }

    // set control points
    // find all controls point pairs inside the given images
    // (stupid implementation)
    const CPVector & controlPoints = pano.getCtrlPoints();

    CPVector cps;

    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        PT::ControlPoint point = *it;

        int matchCount = 0;
        for (UIntSet::const_iterator iit = imgs.begin(); iit != imgs.end(); ++iit) {
            if (point.image1Nr == *iit) {
                matchCount += 1;
            }
            if (point.image2Nr == *iit){
                matchCount += 1;
            }
        }

        if (matchCount == 2) {
            // found a control point.. add to control points list
            cps.push_back(point);
        }
    }

    gl.numPts = cps.size();
    gl.cpt = (controlPoint*) malloc( gl.numPts * sizeof(controlPoint) );

    for (PT::CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        unsigned int i = it - controlPoints.begin();
        // control point stuff.
        gl.cpt[i].type = it->mode;
        gl.cpt[i].num[0] = it->image1Nr;
        gl.cpt[i].x[0] = it->x1;
        gl.cpt[i].y[0] = it->y1;

        gl.cpt[i].num[1] = it->image2Nr;
        gl.cpt[i].x[1] = it->x2;
        gl.cpt[i].y[1] = it->y2;
    }

/*    
    if (CheckParams(&gl) != 0) {
        DEBUG_FATAL("CheckParams error");
        return false;
    }
*/
    gl.fcn	= fcnPano;
    return true;
}

PT::VariableMapVector PTools::AlignInfoWrap::getVariables() const 
{
    VariableMapVector res;
    if (gl.im) {
        for (int i = 0; i < gl.numIm; i++) {
            VariableMap vars;
            Variable v("a", gl.im[i].hfov);
            std::string a("v");
            make_pair(a,v);
            make_pair(std::string("v"), Variable("a", gl.im[i].hfov));
            vars.insert(make_pair(string("v"), Variable("a", gl.im[i].hfov)));
            vars.insert(make_pair(string("y"), Variable("y", gl.im[i].yaw)));
            vars.insert(make_pair(string("r"), Variable("r", gl.im[i].roll)));
            vars.insert(make_pair(string("p"), Variable("p", gl.im[i].pitch)));
            vars.insert(make_pair(string("a"), Variable("a", gl.im[i].cP.radial_params[0][3])));
            vars.insert(make_pair(string("b"), Variable("a", gl.im[i].cP.radial_params[0][2])));
            vars.insert(make_pair(string("c"), Variable("a", gl.im[i].cP.radial_params[0][1])));
            
            vars.insert(make_pair(string("e"), Variable("a", gl.im[i].cP.vertical_params[0])));
            vars.insert(make_pair(string("d"), Variable("a", gl.im[i].cP.horizontal_params[0])));
            
            res.push_back(vars);
        }
    }
    return res;
}


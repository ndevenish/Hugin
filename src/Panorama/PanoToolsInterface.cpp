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

#include <vigra/basicimage.hxx>
#include "common/utils.h"
#include "common/stl_utils.h"
#include "PT/PanoramaMemento.h"

#include "PT/PanoToolsInterface.h"

using namespace std;
using namespace PT;
using namespace PTools;

using namespace vigra;


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


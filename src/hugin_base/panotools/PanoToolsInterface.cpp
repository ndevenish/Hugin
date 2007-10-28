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

#include <hugin_config.h>

#include <stdlib.h>

#include "PanoToolsInterface.h"

#include <vector>
#include <set>
#include <hugin_utils/utils.h>
#include <hugin_utils/stl_utils.h>
#include <panodata/PanoImage.h>
#include <panodata/PanoramaData.h>



namespace HuginBase { namespace PTools {

    
//------------------------------------------------------------------------------
// really strange. the pano12.dll for windows doesn't seem to
// contain the SetCorrectionRadius function, so it is included here

void SetCoordDefaults_copy( CoordInfo *c, int num );
void cubeZero_copy( double *a, int *n, double *root );
void squareZero_copy( double *a, int *n, double *root );
double cubeRoot_copy( double x );
double smallestRoot_copy( double *p );
void SetCorrectionRadius_copy( cPrefs *cP );
int CheckParams_copy( AlignInfo *g );


void SetCoordDefaults_copy( CoordInfo *c, int num )
{
        c->num	= num;
        c->x[0] = (double) num;
        c->x[1] = c->x[2] = 0.0;
        c->set[0] = c->set[1] = c->set[2] = TRUE;
}

void cubeZero_copy( double *a, int *n, double *root )
{
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

void squareZero_copy( double *a, int *n, double *root ){
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

double cubeRoot_copy( double x ){
    if( x == 0.0 )
        return 0.0;
    else if( x > 0.0 )
        return pow(x, 1.0/3.0);
    else
        return - pow(-x, 1.0/3.0);
}

double smallestRoot_copy( double *p ){
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
void SetCorrectionRadius_copy( cPrefs *cP )
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

int CheckParams_copy( AlignInfo *g )
{
    int i;
    int		err = -1;
    char 	*errmsg[] = {
                "No Parameters to optimize",
                "No input images",
                "No Feature Points",
                "Image width must be positive",
                "Image height must be positive",
                "Field of View must be positive",
                "Field of View must be smaller than 180 degrees in rectilinear Images",
                "Unsupported Image Format (must be 0,1,2,3 or 4)",
                "Panorama Width must be positive",
                "Panorama Height must be positive",
                "Field of View must be smaller than 180 degrees in rectilinear Panos",
                "Unsupported Panorama Format",
                "Control Point Coordinates must be positive",
                "Invalid Image Number in Control Point Descriptions"
                };

    if( g->numParam == 0 )				err = 0;
    if( g->numIm	== 0 )				err = 1;
    if( g->numPts	== 0 )				err = 2;
    
    // Check images
    
    for( i=0; i<g->numIm; i++)
    {
        if( g->im[i].width  <= 0 )		err = 3;
        if( g->im[i].height <= 0 )		err = 4;
        if( g->im[i].hfov   <= 0.0 )	err = 5;
        if( g->im[i].format == _rectilinear && g->im[i].hfov >= 180.0 )	err = 6;
        if( g->im[i].format != _rectilinear && g->im[i].format != _panorama &&
            g->im[i].format != _fisheye_circ && g->im[i].format != _fisheye_ff && g->im[i].format != _equirectangular)
                                        err = 7;
    }
    
    // Check Panorama specs
    
    if( g->pano.hfov <= 0.0 )	err = 5;
    if( g->pano.width <=0 )		err = 8;
    if( g->pano.height <=0 )		err = 9;
    if( g->pano.format == _rectilinear && g->pano.hfov >= 180.0 )	err = 10;
    if( g->pano.format != _rectilinear && g->pano.format != _panorama &&
            g->pano.format != _equirectangular ) err = 11;
    
    // Check Control Points
    
    for( i=0; i<g->numPts; i++)
    {
        if( g->cpt[i].x[0] < 0 || g->cpt[i].y[0] < 0 || g->cpt[i].x[1] < 0 || g->cpt[i].y[1] < 0 )
            err = 12;
        if( g->cpt[i].num[0] < 0 || g->cpt[i].num[0] >= g->numIm ||
            g->cpt[i].num[1] < 0 || g->cpt[i].num[1] >= g->numIm )			err = 13;
    }
    
    if( err != -1 )
    {
        PrintError( errmsg[ err ] );
        return -1;
    }
    else
        return 0;
}

//------------------------------------------------------------------------------


    
Transform::~Transform()
{
    if (!m_initialized) {
        freeImage(m_srcImage);
        freeImage(m_dstImage);
    }
}

void Transform::updatePTData(const vigra::Diff2D &srcSize,
                             const VariableMap & srcVars,
                             Lens::LensProjectionFormat & srcProj,
                             const vigra::Diff2D & destSize,
                             PanoramaOptions::ProjectionFormat & destProj,
                             const std::vector<double> & destProjParam,
                             double destHFOV)
{
    if (m_initialized) {
        freeImage(m_srcImage);
        freeImage(m_dstImage);
    }

    // fill our data into the Pano Tools structures.
    setFullImage(m_srcImage, srcSize, 0,
                 srcVars,  srcProj, true);
    setDestImage(m_dstImage, destSize, 0, destProj, destProjParam, destHFOV);
}


void Transform::createTransform(const PanoramaData & pano, unsigned int imgNr,
                                const PanoramaOptions & dest, vigra::Diff2D srcSize)
{
    const PanoImage & img = pano.getImage(imgNr);
    if (srcSize.x == 0 && srcSize.y == 0) {
        srcSize.x = img.getWidth();
        srcSize.y = img.getHeight();
    }
    createTransform(srcSize,
                    pano.getImageVariables(imgNr),
                    pano.getLens(img.getLensNr()).getProjection(),
                    vigra::Diff2D(dest.getWidth(), dest.getHeight()),
                    dest.getProjection(), 
                    dest.getProjectionParameters(),
                    dest.getHFOV(),
                    vigra::Diff2D(img.getWidth(), img.getHeight()));
}

void Transform::createInvTransform(const SrcPanoImage & src, const PanoramaOptions & dest)
{
    VariableMap vars;
    // not very nice, but I don't like to change all the stuff in this file..
    vars.insert(make_pair(std::string("v"), Variable(std::string("v"), src.getHFOV())));
    vars.insert(make_pair(std::string("a"), Variable("a", src.getRadialDistortion()[0])));
    vars.insert(make_pair(std::string("b"), Variable("b", src.getRadialDistortion()[1])));
    vars.insert(make_pair(std::string("c"), Variable("c", src.getRadialDistortion()[2])));
    vars.insert(make_pair(std::string("d"), Variable("d", src.getRadialDistortionCenterShift().x)));
    vars.insert(make_pair(std::string("e"), Variable("e", src.getRadialDistortionCenterShift().y)));
    vars.insert(make_pair(std::string("g"), Variable("g", src.getShear().x)));
    vars.insert(make_pair(std::string("t"), Variable("t", src.getShear().y)));

    vars.insert(make_pair(std::string("r"), Variable("r", src.getRoll())));
    vars.insert(make_pair(std::string("p"), Variable("p", src.getPitch())));
    vars.insert(make_pair(std::string("y"), Variable("y", src.getYaw())));

    createInvTransform(src.getSize(),
                       vars,
                       (Lens::LensProjectionFormat) src.getProjection(),
                       dest.getSize(),
                       dest.getProjection(),
                       dest.getProjectionParameters(),
                       dest.getHFOV(),
                       src.getSize());
}

void Transform::createTransform(const SrcPanoImage & src, const PanoramaOptions & dest)
{

    VariableMap vars;
    // not very nice, but I don't like to change all the stuff in this file..
    vars.insert(make_pair(std::string("v"), Variable(std::string("v"), src.getHFOV())));
    vars.insert(make_pair(std::string("a"), Variable("a", src.getRadialDistortion()[0])));
    vars.insert(make_pair(std::string("b"), Variable("b", src.getRadialDistortion()[1])));
    vars.insert(make_pair(std::string("c"), Variable("c", src.getRadialDistortion()[2])));
    vars.insert(make_pair(std::string("d"), Variable("d", src.getRadialDistortionCenterShift().x)));
    vars.insert(make_pair(std::string("e"), Variable("e", src.getRadialDistortionCenterShift().y)));
    vars.insert(make_pair(std::string("g"), Variable("g", src.getShear().x)));
    vars.insert(make_pair(std::string("t"), Variable("t", src.getShear().y)));

    vars.insert(make_pair(std::string("r"), Variable("r", src.getRoll())));
    vars.insert(make_pair(std::string("p"), Variable("p", src.getPitch())));
    vars.insert(make_pair(std::string("y"), Variable("y", src.getYaw())));

    createTransform(src.getSize(),
                    vars,
                    (Lens::LensProjectionFormat) src.getProjection(),
                    dest.getSize(),
                    dest.getProjection(),
                    dest.getProjectionParameters(),
                    dest.getHFOV(),
                    src.getSize());
}


void Transform::createTransform(const vigra::Diff2D & srcSize,
                                VariableMap srcVars,
                                Lens::LensProjectionFormat srcProj,
                                const vigra::Diff2D &destSize,
                                PanoramaOptions::ProjectionFormat destProj,
                                const std::vector<double> & destProjParam,
                                double destHFOV,
                                const vigra::Diff2D & originalSrcSize)
{
    m_srcTX = destSize.x/2.0;
    m_srcTY = destSize.y/2.0;

    m_destTX = srcSize.x/2.0;
    m_destTY = srcSize.y/2.0;

    // adjust parameters dependant on the image size.
    if (originalSrcSize.x != 0 && originalSrcSize.y != 0) {
        double rx = srcSize.x / (double)originalSrcSize.x;
        double ry = srcSize.y / (double)originalSrcSize.y;
        map_get(srcVars,"d").setValue( map_get(srcVars,"d").getValue() * rx );
        map_get(srcVars,"e").setValue( map_get(srcVars,"e").getValue() * ry );
    }

    updatePTData(srcSize, srcVars, srcProj,
                 destSize, destProj, destProjParam, destHFOV);
    // create the actual stack
    SetMakeParams( m_stack, &m_mp, &m_srcImage , &m_dstImage, 0 );
}


void Transform::createInvTransform(const PanoramaData & pano, unsigned int imgNr,
                                   const PanoramaOptions & dest, vigra::Diff2D srcSize)
{
    const PanoImage & img = pano.getImage(imgNr);
    if (srcSize.x == 0 && srcSize.y == 0) {
        srcSize.x = img.getWidth();
        srcSize.y = img.getHeight();
    }
    createInvTransform(srcSize,
                       pano.getImageVariables(imgNr),
                       pano.getLens(img.getLensNr()).getProjection(),
                       vigra::Diff2D(dest.getWidth(), dest.getHeight()),
                       dest.getProjection(),
                       dest.getProjectionParameters(),
                       dest.getHFOV(),
                       vigra::Diff2D(img.getWidth(), img.getHeight()));
}

void Transform::createInvTransform(const vigra::Diff2D & srcSize,
                                   VariableMap srcVars,
                                   Lens::LensProjectionFormat srcProj,
                                   const vigra::Diff2D & destSize,
                                   PanoramaOptions::ProjectionFormat destProj,
                                   const std::vector<double> & destProjParam,
                                   double destHFOV,
                                   const vigra::Diff2D & originalSrcSize)
{
    m_srcTX = srcSize.x/2.0;
    m_srcTY = srcSize.y/2.0;

    m_destTX = destSize.x/2.0;
    m_destTY = destSize.y/2.0;

    // adjust parameters dependant on the image size.
    if (originalSrcSize.x != 0 && originalSrcSize.y != 0) {
        double rx = srcSize.x / (double)originalSrcSize.x;
        double ry = srcSize.y / (double)originalSrcSize.y;
        map_get(srcVars,"d").setValue( map_get(srcVars,"d").getValue() * rx );
        map_get(srcVars,"e").setValue( map_get(srcVars,"e").getValue() * ry );
    }

    updatePTData(srcSize, srcVars, srcProj,
                 destSize, destProj, 
                 destProjParam,
                 destHFOV);
    // create the actual stack
    SetInvMakeParams( m_stack, &m_mp, &m_srcImage , &m_dstImage, 0 );
}


bool Transform::transform(double & x_dest, double & y_dest,
               double x_src, double y_src) const
{
    void * params= (void *) (&m_stack);
    return execute_stack_new(x_src, y_src, &x_dest, &y_dest, params) != 0;
}

///
bool Transform::transform(hugin_utils::FDiff2D& dest, const hugin_utils::FDiff2D & src) const
{
    double x_dest, y_dest;
    void * params= (void *) (&m_stack);
    bool ok = execute_stack_new(src.x, src.y, &x_dest, &y_dest, params) != 0;
    dest.x = x_dest;
    dest.y = y_dest;
    return ok;
}

/** like transform, but return image coordinates, not cartesian
*  coordinates
*/
bool Transform::transformImgCoord(double & x_dest, double & y_dest,
                       double x_src, double y_src) const
{
    x_src -= m_srcTX - 0.5 ;
    y_src -= m_srcTY - 0.5;
    
    void * params= (void *) (&m_stack);
    bool ok = execute_stack_new(x_src, y_src, &x_dest, &y_dest, params) != 0;
    x_dest += m_destTX - 0.5;
    y_dest += m_destTY - 0.5;
    return ok;
}




AlignInfoWrap::AlignInfoWrap()
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

AlignInfoWrap::~AlignInfoWrap()
{
//    DisposeAlignInfo(&gl);

//    delete[](gl.img);
//    delete[](gl.opt);
//    delete[](gl.cpt);
//    delete[](gl.t);
//    delete[](gl.cim);


    if (gl.im) free(gl.im);
    if (gl.opt) free(gl.opt);
    if (gl.cpt) free(gl.cpt);
    if (gl.t) free(gl.t);
    if (gl.cim) free(gl.cim);

}


bool AlignInfoWrap::setInfo(const PanoramaData & pano)
{
    const VariableMapVector & variables = pano.getVariables();
    const CPVector & controlPoints = pano.getCtrlPoints();
    const OptimizeVector & optvec = pano.getOptimizeVector();

    // based on code from ParseScript by H. Dersch

/*
    delete(gl.im);
    delete(gl.opt);
    delete(gl.cpt);
    delete(gl.t);
    delete(gl.cim);
*/
    if (gl.im) free(gl.im);
    if (gl.opt) free(gl.opt);
    if (gl.cpt) free(gl.cpt);
    if (gl.t) free(gl.t);
    if (gl.cim) free(gl.cim);

    // Determine number of images and control points
    gl.numIm 	= int(variables.size());
    gl.nt 	= 0;

    int optVarCounter=0;
    // be careful. linked variables should not be specified multiple times.
    std::vector<std::set<std::string> > linkvars(pano.getNrOfLenses());

    for (unsigned imgNr = 0; imgNr < variables.size(); imgNr++)
    {
        unsigned int lensNr = pano.getImage(imgNr).getLensNr();
        const Lens & lens = pano.getLens(lensNr);
        const std::set<std::string> & optvar = optvec[imgNr];
        for (std::set<std::string>::const_iterator sit = optvar.begin();
             sit != optvar.end(); ++sit )
        {
            if (set_contains(lens.variables,*sit)) {
                // it is a lens variable
                if (const_map_get(lens.variables,*sit).isLinked()) {
                    if (! set_contains(linkvars[lensNr], *sit))
                    {
                        // linked, count only once
                        optVarCounter++;
                        linkvars[lensNr].insert(*sit);
                    }
                } else {
                    // unlinked lens variable, count as usual
                    optVarCounter++;
                }
            } else {
                // not a lens variable, count as usual
                optVarCounter++;
            }
        }
    }
    gl.numParam = optVarCounter;

    gl.numPts = int(controlPoints.size());

    // Allocate Space for Pointers to images, preferences and control points

    // use memory allocation routines from pano12.dll
    gl.im  = (Image*)	     malloc( gl.numIm 	* sizeof(::Image) );
    gl.opt = (optVars*)	     malloc( gl.numIm 	* sizeof(::optVars) );
    gl.t   = (triangle*)     malloc( gl.nt 	* sizeof(::triangle) );
    gl.cim = (CoordInfo*)    malloc( gl.numIm 	* sizeof(::CoordInfo) );
    gl.cpt = (controlPoint*) malloc( gl.numPts  * sizeof(::controlPoint) );

    if( gl.im == NULL || gl.opt == NULL || gl.cpt == NULL || gl.t == NULL || gl.cim == NULL )
    {
        DEBUG_FATAL("Not enough memory");
    }

    const PanoramaOptions & opts = pano.getOptions();
    setDestImage(gl.pano, vigra::Diff2D(opts.getWidth(), opts.getHeight()),
                 0, opts.getProjection(), 
                 opts.getProjectionParameters(),
                 opts.getHFOV());

    // Default: Use buffer 'buf' for stitching
    SetStitchDefaults(&(gl.st)); strcpy( gl.st.srcName, "buf" );
    for(int imgNr=0; imgNr<gl.numIm; imgNr++)
    {
        SetImageDefaults( &(gl.im[imgNr]) );
        SetOptDefaults	( &(gl.opt[imgNr]));
        SetCoordDefaults_copy( &(gl.cim[imgNr]), imgNr);
    }

    std::map<unsigned int, unsigned int> linkAnchors;
    for(unsigned imgNr=0; (int)imgNr< gl.numIm; imgNr++)
    {
        const PanoImage & pimg = pano.getImage(imgNr);
        const VariableMap & vars = variables[imgNr];
        unsigned lensNr = pimg.getLensNr();
        //pano.getImageVariables(*it);
        const Lens & lens = pano.getLens(lensNr);

        // set the image information, with pointer to dummy image data
        setFullImage(gl.im[imgNr],
                     vigra::Diff2D(pimg.getWidth(), pimg.getHeight()),
                     0, vars, lens.getProjection(), true);
        
// macro to set optimisation vars without a lot of cut n paste
#define PT_SET_OPT(n,v) \
{ if (set_contains(optvec[imgNr], #v )) { \
    if (set_contains(lens.variables, #v) \
        && const_map_get(lens.variables, #v ).isLinked()) {\
        if (set_contains(linkAnchors, lensNr) \
            && linkAnchors[lensNr] != imgNr) \
            { \
                gl.opt[imgNr]. n = linkAnchors[lensNr] + 2; \
            }else { \
                gl.opt[imgNr]. n  = 1; \
                    linkAnchors[lensNr] = imgNr; \
            } \
    } else { \
        gl.opt[imgNr]. n = 1; \
    } \
} else { \
    gl.opt[imgNr]. n = 0; \
} }
        
        // set optimisation flags
        PT_SET_OPT(hfov, v);
        PT_SET_OPT(yaw,  y);
        PT_SET_OPT(pitch, p);
        PT_SET_OPT(roll, r);
        PT_SET_OPT(a, a);
        PT_SET_OPT(b, b);
        PT_SET_OPT(c, c);
        PT_SET_OPT(d, d);
        PT_SET_OPT(e, e);
        PT_SET_OPT(shear_x, g);
        PT_SET_OPT(shear_y, t);
    
#undef PT_SET_OPT
    
    }
    

    unsigned i=0;
    for (CPVector::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        // control point stuff.
        gl.cpt[i].type = it->mode;
        gl.cpt[i].num[0] = it->image1Nr;
        gl.cpt[i].x[0] = it->x1;
        gl.cpt[i].y[0] = it->y1;

        gl.cpt[i].num[1] = it->image2Nr;
        gl.cpt[i].x[1] = it->x2;
        gl.cpt[i].y[1] = it->y2;
        i++;
    }


    if( CheckParams_copy( &gl ) != 0 ) {
        DEBUG_FATAL("CheckParams() returned false!");
        return false;
    }
    gl.fcn	= fcnPano;
    return true;
}

void AlignInfoWrap::setGlobal()
{
    SetGlobalPtr( &gl );
}

VariableMapVector AlignInfoWrap::getVariables() const
{
    return GetAlignInfoVariables(gl);
}

CPVector AlignInfoWrap::getCtrlPoints() const
{
    return GetAlignInfoCtrlPoints(gl);
}


VariableMapVector GetAlignInfoVariables(const AlignInfo& gl)
{
    VariableMapVector res;
    if (gl.im) {
        for (int i = 0; i < gl.numIm; i++) {
            VariableMap vars;
            vars.insert(make_pair(std::string("v"), Variable("v", gl.im[i].hfov)));
            vars.insert(make_pair(std::string("y"), Variable("y", gl.im[i].yaw)));
            vars.insert(make_pair(std::string("r"), Variable("r", gl.im[i].roll)));
            vars.insert(make_pair(std::string("p"), Variable("p", gl.im[i].pitch)));
            vars.insert(make_pair(std::string("a"), Variable("a", gl.im[i].cP.radial_params[0][3])));
            vars.insert(make_pair(std::string("b"), Variable("b", gl.im[i].cP.radial_params[0][2])));
            vars.insert(make_pair(std::string("c"), Variable("c", gl.im[i].cP.radial_params[0][1])));

            vars.insert(make_pair(std::string("e"), Variable("e", gl.im[i].cP.vertical_params[0])));
            vars.insert(make_pair(std::string("d"), Variable("d", gl.im[i].cP.horizontal_params[0])));

            res.push_back(vars);
        }
    }
    return res;
}

CPVector GetAlignInfoCtrlPoints(const AlignInfo& gl)
{
    CPVector result;
    if (gl.cpt) {
        for (int i = 0; i < gl.numPts; i++) {
            ControlPoint pnt(gl.cpt[i].num[0], gl.cpt[i].x[0], gl.cpt[i].y[0],
                             gl.cpt[i].num[1], gl.cpt[i].x[1], gl.cpt[i].y[1], (ControlPoint::OptimizeMode) gl.cpt[i].type);
            pnt.error = sqrt(distSquared(i));
            result.push_back(pnt);
        }
    }
    return result;
}


void setDestImage(Image & image, vigra::Diff2D size,
                          unsigned char * imageData,
                          const PanoramaOptions::ProjectionFormat & format,
                          const std::vector<double> & projParams,
                          double destHFOV)
{
    SetImageDefaults(&image);
    image.width = size.x;
    image.height = size.y;
    image.bytesPerLine = image.width*3;
    image.bitsPerPixel = 24;
    image.dataSize = image.height * image.bytesPerLine;
    // Allocate memory for pointer to pointer to image data
//    image.data = (unsigned char**)malloc( sizeof(unsigned char*) );
//    if(image.data == NULL) {
//        DEBUG_FATAL("Out of memory");
//    }
    image.data = 0;
#ifdef HasPANO13
    pano_projection_features projd;
    if (panoProjectionFeaturesQuery((int) format, &projd)) {
        image.format = projd.internalFormat;
    } else {
        image.format = _equirectangular;
        PrintError("unsupported projection");
    }
    image.formatParamCount = projd.numberOfParameters;
    assert(image.formatParamCount == (int) projParams.size());
    for (int i=0; i < projd.numberOfParameters; i++) {
        image.formatParam[i] = projParams[i];
        DEBUG_DEBUG("projection parameter " << i << ": " << image.formatParam[i]);
    }
#else
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
    case PanoramaOptions::FULL_FRAME_FISHEYE:
        image.format = _fisheye_ff;
        break;
    case PanoramaOptions::STEREOGRAPHIC:
        image.format = _stereographic;
        break;
    case PanoramaOptions::MERCATOR:
        image.format = _mercator;
        break;
    case PanoramaOptions::TRANSVERSE_MERCATOR:
        image.format = _trans_mercator;
        break;
    case PanoramaOptions::SINUSOIDAL:
        image.format = _sinusoidal;
        break;
#ifdef HasPANO13
    case PanoramaOptions::LAMBERT:
        image.format = _lambert;
        break;
    case PanoramaOptions::LAMBERT_AZIMUTHAL:
        image.format = _lambertazimuthal;
        break;
#endif
    default:
        PrintError("unsupported projection");
    }
#endif
    image.hfov = destHFOV;
}


// internal function, used by setFullImage() to set the distortion parameters
void initCPrefs(cPrefs & p, const VariableMap &vars)
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
    val = const_map_get(vars, "g").getValue();
    double val2 = const_map_get(vars, "t").getValue();
    if (val2 != 0.0 || val != 0.0) {
        p.shear = TRUE;
        p.shear_x = val;
        p.shear_y = val2;
    } else {
        p.shear = FALSE;
    }
    p.resize = FALSE;
    p.luminance = FALSE;
    p.cutFrame = FALSE;
    p.fourier = FALSE;

    // calculate correction radius
    // copied function from pano12.dll (missing under windows... strange)
    SetCorrectionRadius_copy(&p);

}

void setFullImage(Image & image, vigra::Diff2D size,
                          unsigned char * imageData, const VariableMap & vars,
                          const Lens::LensProjectionFormat format,
                          bool correctDistortions)
{
    SetImageDefaults(&image);
    image.width = size.x;
    image.height = size.y;
    image.bytesPerLine = image.width*3;
    image.bitsPerPixel = 24;
    image.dataSize = image.height * image.bytesPerLine;

#if 0
    // Allocate memory for pointer to pointer to image data
    image.data = (unsigned char**)malloc( sizeof(unsigned char*) );
    if(image.data == NULL) {
        DEBUG_FATAL("Out of memory");
    }
    *(image.data) = imageData;
#else    
    image.data = 0;
#endif

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
    case Lens::EQUIRECTANGULAR:
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

// ===========================================================================
// ===========================================================================


void createAdjustPrefs(aPrefs  & p, TrformStr & transf)
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
void createAdjustTrform(TrformStr & trf)
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
void freeTrform(TrformStr & trf)
{
    if (trf.dest) {
        if (trf.dest->data) {
            myfree((void**)trf.dest->data);
        }
        free((void**)trf.dest);
    }
    if (trf.src) {
        if (trf.src->data) {
            myfree((void**)trf.src->data);
        }
        free((void**)trf.src);
    }
}

void freeImage(Image &img)
{
    if (img.data) {
        myfree((void**)img.data);
    }
}

void setAdjustSrcImg(TrformStr & trf, aPrefs & ap,
                             int width, int height, unsigned char * imageData,
                             const VariableMap & vars,
                             const Lens::LensProjectionFormat format,
                             bool correctDistortions)
{
    DEBUG_ASSERT(trf.src);
    if (trf.src->data) {
        myfree((void**)trf.src->data);
    }
    setFullImage(*(trf.src), vigra::Diff2D(width,height), imageData, vars, format,
                 correctDistortions);
    ap.im = *(trf.src);
}

void setAdjustDestImg(TrformStr & trf, aPrefs & ap,
                              int width, int height, unsigned char * imageData,
                              const PanoramaOptions & opts)
{
    DEBUG_ASSERT(trf.dest);
    if (trf.dest->data) {
        myfree((void**)trf.dest->data);
    }
    setDestImage(*(trf.dest), vigra::Diff2D(width, height), imageData, opts.getProjection(), 
                   opts.getProjectionParameters(),
                   opts.getHFOV());
    ap.pano = *(trf.dest);
}


void setOptVars(optVars & opt, const std::set<std::string> & optvars)
{
    // FIXME: BUG does not support linked variables!!!!
    // set the optimisation value to img+2, if it is linked to img...
    opt.hfov    = set_contains(optvars,"v") ? 1 : 0;
    opt.yaw     = set_contains(optvars,"y") ? 1 : 0;
    opt.pitch   = set_contains(optvars,"p") ? 1 : 0;
    opt.roll    = set_contains(optvars,"r") ? 1 : 0;
    opt.a       = set_contains(optvars,"a") ? 1 : 0;
    opt.b       = set_contains(optvars,"b") ? 1 : 0;
    opt.c       = set_contains(optvars,"c") ? 1 : 0;
    opt.d       = set_contains(optvars,"d") ? 1 : 0;
    opt.e       = set_contains(optvars,"e") ? 1 : 0;
    opt.shear_x = set_contains(optvars,"g") ? 1 : 0;
    opt.shear_y = set_contains(optvars,"t") ? 1 : 0;
}


}} // namespace

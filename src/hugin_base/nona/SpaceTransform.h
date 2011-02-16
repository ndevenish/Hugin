// -*- c-basic-offset: 4 -*-
/** @file nona/SpaceTransform.h
*
*  @brief implementation of Space Transformation
*
*  @author Alexandre Jenny <alexandre.jenny@le-geo.com>
*
*  @todo The file implements a lot of functions of libpano new. 
*        These should be replace with libpano versions.
*
*/

/*
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
    
#ifndef _NONA_SPACETRANSFORM_H
#define _NONA_SPACETRANSFORM_H
    
#include <vigra/diff2d.hxx>
#include <hugin_math/Matrix3.h>

#include <panodata/PanoramaData.h>


namespace HuginBase {
namespace Nona {
        
    
/** Parameters for transformation calls
*  Can be just one double, two double, 4 double, a matrix, matrix and a double
*/
struct _FuncParams
{
    union {
        double var0;
        double distance;
        double shift;
    };
    double	var1;
    double	var2;
    double	var3;
    double	var4;
    double	var5;
    double  var6;
    double  var7;
    Matrix3	mt;
};


/** Transformation function type
*
*/
typedef	void (*trfn)( double x_dest, double y_dest, double* x_src, double* y_src, const _FuncParams &params );


/** Function descriptor to be executed by exec_function
*
*/
typedef struct _fDesc
{
    trfn		func;	// function to be called
    _FuncParams	param;	// parameters to be used
} fDescription;



/**
 *
 */
class IMPEX SpaceTransform
{
            
    public:
        /** ctor.
         */
        SpaceTransform();

        /** dtor.
         */
        ~SpaceTransform();

        /** Init Transform
         * Create the stack of matrices for direct transform
         */
        void Init(  const SrcPanoImage & img,
                    const vigra::Diff2D & destSize,
                    PanoramaOptions::ProjectionFormat destProj,
                    double destHFOV );

        /** Init Inv Transform
         * Create the stack of matrices for reverse transform
         */
        void InitInv(   const SrcPanoImage & img,
                        const vigra::Diff2D & destSize,
                        PanoramaOptions::ProjectionFormat destProj,
                        double destHFOV );

        
        // Different ways to create the transform
        
    public:
        /** transformation for radial correction only
         */
        void InitRadialCorrect(const vigra::Size2D & sz, const std::vector<double> & radDist, 
                               const hugin_utils::FDiff2D & centerShift);

        /** init radial correction from pano image description and selected channel
         *  (R=0, G=1, B=2), (TCA corr)
         */
        void InitRadialCorrect(const SrcPanoImage & src, int channel=1);

        void InitInvRadialCorrect(const SrcPanoImage & src, int channel=1);

        void createTransform(const SrcPanoImage & src, const PanoramaOptions & dest);
        
        void createInvTransform(const SrcPanoImage & src, const PanoramaOptions & dest);

        // create pano -> img transform
        void createTransform(const PanoramaData & pano, unsigned int imgNr,
                             const PanoramaOptions & dest,
                             vigra::Diff2D srcSize=vigra::Diff2D(0,0));

        // create image->pano transformation
        void createInvTransform(const PanoramaData & pano, unsigned int imgNr,
                                const PanoramaOptions & dest,
                                vigra::Diff2D srcSize=vigra::Diff2D(0,0));
        
        void createTransform(const vigra::Diff2D & srcSize,
                         const VariableMap & srcVars,
                         Lens::LensProjectionFormat srcProj,
                         const vigra::Diff2D &destSize,
                         PanoramaOptions::ProjectionFormat destProj,
                             double destHFOV);
        
        // create image->pano transformation
        void createInvTransform(const vigra::Diff2D & srcSize,
                                const VariableMap & srcVars,
                                Lens::LensProjectionFormat srcProj,
                                const vigra::Diff2D & destSize,
                                PanoramaOptions::ProjectionFormat destProj,
                                double destHFOV);
        
        
    public:
        /** transform
         * Get the new coordinates
         */
        bool transform(hugin_utils::FDiff2D& dest, const hugin_utils::FDiff2D & src) const;

        /** like transform, but return image coordinates, not cartesian
         *  coordinates
         */
        bool transformImgCoord(double & x_dest, double & y_dest, double x_src, double y_src) const;

        bool transformImgCoord(hugin_utils::FDiff2D & dest, const hugin_utils::FDiff2D &src) const
        {
            return transformImgCoord(dest.x, dest.y, src.x, src.y);
        }
        
        
    public:
        /** returns true if this transform is an identity transform */
        bool isIdentity()
        {
            return m_Stack.size() == 0;
        }

        
    private:
        /// add a new transformation
        void AddTransform( trfn function_name, double var0, double var1 = 0.0f, double var2 = 0.0f, double var3 = 0.0f, double var4=0.0f, double var5=0.0f, double var6=0.0f, double var7=0.0f );
        void AddTransform( trfn function_name, Matrix3 m, double var0, double var1=0.0f, double var2=0.0f, double var3=0.0f);
        
        
    private:
        /// was the class initialized ?
        bool m_Initialized;

        /// used to convert from screen to cartesian coordinates
        double m_srcTX, m_srcTY;
        double m_destTX, m_destTY;

        /// vector of transformations
        std::vector<fDescription>	m_Stack;

};




/** combine 4rd degree polynomials
 *
 *  Computes new polynomial so that:
 *   c(x) ~= q(p(x))
 *  c is also a 4rd degree polynomial, and the expansion is cut after x^4
 *  constant term is assumed to be 0, and not included in p,q and c
 */
template <class VECTOR>
void combinePolynom4(const VECTOR & p,
                     const VECTOR & q,
                     VECTOR & c);


/** Internal function to estimate the image scaling required to avoid
 *  black stripes at the image borders
 */
template <class TRANSFORM>
void traceImageOutline(vigra::Size2D sz,
                       TRANSFORM & transf,
                       vigra::Rect2D & inside,
                       vigra::Rect2D & boundingBox);


/** \brief Calculate effective scaling factor for a given source image.
 */
IMPEX double estScaleFactorForFullFrame(const SrcPanoImage & src);


/**
 * \brief Calculate effective scaling factor.
 *
 * This function returns the smalles scale factor that has been applied
 * 
 * If values < 1 are returned, black borders will occur. In that case the
 * distortion correction parameters might need to be adjusted to avoid
 * the black borders.
 *
 *
 * \param coef1  lens distortion coefficients, including d coefficient.
 * \param width  image width
 * \param height image height
 * \return smallest r_corr / r_orig in areas that might lead to black borders.
 */
double estRadialScaleCrop(const std::vector<double> & coeff, int width, int height);





//==============================================================================
// template implementations

template <class VECTOR>
void combinePolynom4(const VECTOR & p, const VECTOR & q, VECTOR & c)
{
    double d3 = p[3]*p[3]*p[3];
    c[0] = (3*q[1]*p[3]*p[3]*p[2]+q[3]*p[0]+q[2]*(2*p[3]*p[1]+p[2]*p[2])+q[0]*d3*p[3]);
    c[1] = (2*q[2]*p[3]*p[2]+q[1]*d3+q[3]*p[1]);
    c[2] = (q[3]*p[2]+q[2]*p[3]*p[3]);
    c[3] = q[3]*p[3];

    /*
    old code for polynoms of up to x^3
    c[0] = q[0]*(p[2]*(2*p[3]*p[1]+p[2]*p[2])+p[3]*(2*p[3]*p[0]+2*p[2]*p[1])+p[0]*p[3]*p[3]+2*p[1]*p[3]*p[2])
           +q[1]*(2*p[3]*p[0]+2*p[2]*p[1])+q[2]*p[0];

    c[1] = q[0]*(p[3]*(2*p[3]*p[1]+p[2]*p[2])+2*p[2]*p[2]*p[3]+p[1]*p[3]*p[3])
            +q[2]*p[1]+q[1]*(2*p[3]*p[1]+p[2]*p[2]);

    c[2] = 3*q[0]*p[3]*p[3]*p[2]+2*q[1]*p[3]*p[2]+q[2]*p[2];
    c[3] = q[0]*p[3]*p[3]*p[3]+q[1]*p[3]*p[3]+q[2]*p[3]+q[3];
    */
}


template <class TRANSFORM>
void traceImageOutline(vigra::Size2D sz, TRANSFORM & transf, vigra::Rect2D & inside, vigra::Rect2D & boundingBox)
{
    boundingBox = vigra::Rect2D();
    inside = vigra::Rect2D();
    int x=0;
    int y=0;
    double xd;
    double yd;
    transf.transformImgCoord(xd,yd, x,y);
    // calculate scaling factor that would be required to avoid black borders
//    double scale = std::max((-sz.x/2)/(xd-sz.x/2), (-sz.y/2)/(yd-sz.y/2));
    int left = 0;
    int right = sz.x;
    int top = 0;
    int bottom = sz.y;
    // left
    for (y=0; y < sz.y; y++) {
        transf.transformImgCoord(xd,yd, x,y);
        boundingBox |= vigra::Point2D(hugin_utils::roundi(xd), hugin_utils::roundi(yd));
        left = std::max(hugin_utils::roundi(xd), left);
    }
    // right
    x = sz.x-1;
    for (y=0; y < sz.y; y++) {
        transf.transformImgCoord(xd,yd, x,y);
        boundingBox |= vigra::Point2D(hugin_utils::roundi(xd), hugin_utils::roundi(yd));
        right = std::min(hugin_utils::roundi(xd), right);
    }
    // bottom
    y=sz.y-1;
    for (x=0; x < sz.x; x++) {
        transf.transformImgCoord(xd,yd, x,y);
        boundingBox |= vigra::Point2D(hugin_utils::roundi(xd), hugin_utils::roundi(yd));
        bottom = std::min(hugin_utils::roundi(yd), bottom);
    }
    // top
    y=0;
    for (x=0; x < sz.x; x++) {
        transf.transformImgCoord(xd,yd, x,y);
        boundingBox |= vigra::Point2D(hugin_utils::roundi(xd), hugin_utils::roundi(yd));
        top = std::max(hugin_utils::roundi(yd), top);
    }
    inside.setUpperLeft(vigra::Point2D(left, top));
    inside.setLowerRight(vigra::Point2D(right, bottom));
}



} // namespace
} // namespace

#endif

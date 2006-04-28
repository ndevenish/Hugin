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

#ifndef _SPACETRANSFORM_H
#define _SPACETRANSFORM_H


#include "common/Matrix3.h"
#include "PT/Panorama.h"


namespace PT {

/** Parameters for transformation calls
 *  Can be just one double, two double, 4 double, a matrix, matrix and a double
 */
struct _FuncParams
{
	union
	{
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

/** Generate the matrice for the transformation of spaces
 *
 * @todo
 * @todo
 * @todo
 * @todo
 * @todo
 * @todo
 *
 */
class SpaceTransform
{
public :
	/** ctor.
     */
    SpaceTransform();

    /** dtor.
     */
	~SpaceTransform();

	/** Init Transform
	 * Create the stack of matrices for direct transform
	 */
	void Init(	const vigra::Diff2D & srcSize,
                const VariableMap & srcVars,
				PT::Lens::LensProjectionFormat srcProj,
				const vigra::Diff2D & destSize,
				PT::PanoramaOptions::ProjectionFormat destProj,
				double destHFOV );

	/** Init Inv Transform
	 * Create the stack of matrices for reverse transform
	 */
	void InitInv(	const vigra::Diff2D & srcSize,
					const VariableMap & srcVars,
					PT::Lens::LensProjectionFormat srcProj,
					const vigra::Diff2D & destSize,
					PT::PanoramaOptions::ProjectionFormat destProj,
					double destHFOV );

	// Different ways to create the transform
	void createTransform(const vigra::Diff2D & srcSize,
                         const PT::VariableMap & srcVars,
                         PT::Lens::LensProjectionFormat srcProj,
                         const vigra::Diff2D &destSize,
                         PT::PanoramaOptions::ProjectionFormat destProj,
                         double destHFOV)
	{
		Init( srcSize, srcVars, srcProj, destSize, destProj, destHFOV);
	}

    /** transformation for radial correction only
     */
    void InitRadialCorrect(const vigra::Size2D & sz, std::vector<double> & radDist, 
                           const FDiff2D & centerShift);

    /** init radial correction from pano image description and selected channel
     *  (R=0, G=1, B=2), (TCA corr)
     */
    void InitRadialCorrect(const SrcPanoImage & src, int channel=1);

    void createTransform(const PT::SrcPanoImage & src, const PT::PanoramaOptions & dest);
    void createInvTransform(const PT::SrcPanoImage & src, const PT::PanoramaOptions & dest);

    // create pano -> img transform
	void createTransform(const PT::Panorama & pano, unsigned int imgNr,
                         const PT::PanoramaOptions & dest,
                         vigra::Diff2D srcSize=vigra::Diff2D(0,0))
	{
		const PanoImage & img = pano.getImage(imgNr);
		if (srcSize.x == 0 && srcSize.y == 0)
		{
			srcSize.x = img.getWidth();
			srcSize.y = img.getHeight();
		}
		Init(	srcSize,
                pano.getImageVariables(imgNr),
                pano.getLens(img.getLensNr()).getProjection(),
                vigra::Diff2D(dest.getWidth(), dest.getHeight()),
                dest.getProjection(), dest.getHFOV() );
	}

    // create image->pano transformation
    void createInvTransform(const vigra::Diff2D & srcSize,
                            const PT::VariableMap & srcVars,
                            PT::Lens::LensProjectionFormat srcProj,
                            const vigra::Diff2D & destSize,
                            PT::PanoramaOptions::ProjectionFormat destProj,
                            double destHFOV)
	{
		InitInv( srcSize, srcVars, srcProj, destSize, destProj, destHFOV);
	}

    // create image->pano transformation
    void createInvTransform(const PT::Panorama & pano, unsigned int imgNr,
                            const PT::PanoramaOptions & dest,
                            vigra::Diff2D srcSize=vigra::Diff2D(0,0))
	{
		const PanoImage & img = pano.getImage(imgNr);
		if (srcSize.x == 0 && srcSize.y == 0)
		{
			srcSize.x = img.getWidth();
			srcSize.y = img.getHeight();
		}
		InitInv(	srcSize,
                    pano.getImageVariables(imgNr),
                    pano.getLens(img.getLensNr()).getProjection(),
                    vigra::Diff2D(dest.getWidth(), dest.getHeight()),
                    dest.getProjection(), dest.getHFOV());
	}

    /** transform
     * Get the new coordinates
     */
    bool transform(FDiff2D& dest, const FDiff2D & src) const;

    /** like transform, but return image coordinates, not cartesian
     *  coordinates
     */
    bool transformImgCoord(double & x_dest, double & y_dest, double x_src, double y_src) const;

    bool transformImgCoord(FDiff2D & dest, const FDiff2D &src) const
    {
        return transformImgCoord(dest.x, dest.y, src.x, src.y);
    }

    /** returns true if this transform is an identity transform */
    bool isIdentity()
    {
        return m_Stack.size() == 0;
    }

private :
    /// was the class initialized ?
    bool m_Initialized;

    /// used to convert from screen to cartesian coordinates
    double m_srcTX, m_srcTY;
    double m_destTX, m_destTY;

    /// vector of transformations
    std::vector<fDescription>	m_Stack;

    /// add a new transformation
    void AddTransform( PT::trfn function_name, double var0, double var1 = 0.0f, double var2 = 0.0f, double var3 = 0.0f, double var4=0.0f, double var5=0.0f, double var6=0.0f, double var7=0.0f );
    void AddTransform( PT::trfn function_name, Matrix3 m, double var0, double var1=0.0f, double var2=0.0f, double var3=0.0f);
};

/** combine 3rd degree polynomials.
 *
 *  Computes new polynomial so that:
 *   c(x) ~= q(p(x))
 *  c is also a 3rd degree polynomial, and the expansion is cut after x^3
 */
template <class VECTOR>
void combinePolynom3(const VECTOR & p, const VECTOR & q, VECTOR & c)
{
    c[0] = q[0]*(p[2]*(2*p[3]*p[1]+p[2]*p[2])+p[3]*(2*p[3]*p[0]+2*p[2]*p[1])+p[0]*p[3]*p[3]+2*p[1]*p[3]*p[2])
           +q[1]*(2*p[3]*p[0]+2*p[2]*p[1])+q[2]*p[0];

    c[1] = q[0]*(p[3]*(2*p[3]*p[1]+p[2]*p[2])+2*p[2]*p[2]*p[3]+p[1]*p[3]*p[3])
            +q[2]*p[1]+q[1]*(2*p[3]*p[1]+p[2]*p[2]);

    c[2] = 3*q[0]*p[3]*p[3]*p[2]+2*q[1]*p[3]*p[2]+q[2]*p[2];
    c[3] = q[0]*p[3]*p[3]*p[3]+q[1]*p[3]*p[3]+q[2]*p[3]+q[3];
}


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
 *
 *****************************************************/
double estRadialScaleCrop(std::vector<double> coeff, int width, int height);


} // namespace PT

#endif

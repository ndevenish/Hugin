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
    void transform(FDiff2D& dest, const FDiff2D & src);

    /** like transform, but return image coordinates, not cartesian
     *  coordinates
     */
    void transformImgCoord(double & x_dest, double & y_dest, double x_src, double y_src);

private :
    /// was the class initialized ?
    bool m_Initialized;

    /// used to convert from screen to cartesian coordinates
    double m_srcTX, m_srcTY;
    double m_destTX, m_destTY;

    /// vector of transformations
    std::vector<fDescription>	m_Stack;

    /// add a new transformation
    void AddTransform( PT::trfn function_name, double var0, double var1 = 0.0f, double var2 = 0.0f, double var3 = 0.0f, double var4=0.0f, double var5=0.0f );
    void AddTransform( PT::trfn function_name, Matrix3 m, double var0, double var1=0.0f, double var2=0.0f, double var3=0.0f);
};


} // namespace PT

#endif

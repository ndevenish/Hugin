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

#ifndef PI
	#define PI 3.14159265358979323846
#endif

#define DEG_TO_RAD( x )		( (x) * 2.0 * PI / 360.0 )
#define RAD_TO_DEG( x )		( (x) * 360.0 / ( 2.0 * PI ) )

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
                pano.getLens(img.getLensNr()).projectionFormat,
				vigra::Diff2D(dest.width, dest.getHeight()),
                dest.projectionFormat, dest.HFOV );
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
                    pano.getLens(img.getLensNr()).projectionFormat,
					vigra::Diff2D(dest.width, dest.getHeight()),
                    dest.projectionFormat, dest.HFOV);
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
	void AddTransform( PT::trfn function_name, double var0, double var1 = 0.0f, double var2 = 0.0f, double var3 = 0.0f );
	void AddTransform( PT::trfn function_name, Matrix3 m, double var0, double var1=0.0f, double var2=0.0f, double var3=0.0f);
};


/** struct to hold a image state for stitching
 */
template <class RemapImage, class DistImage>
struct RemappedPanoImage
{
    RemapImage image;   ///< remapped image
    DistImage dist;     ///< corrosponding distances
    /// upper left corner of remapped image in complete pano
    vigra::Diff2D ul;

    /** get the distance from the image center to the point @p p.
     *
     * @param p point in output panorama coordinates.
     *
     * @return distance to center of image,
     *         or FLT_MAX if outside remapped image.
     */
    float getDistanceFromCenter(vigra::Diff2D & p)
        {
            vigra::Diff2D lr = ul + image.size() - vigra::Diff2D(1,1);
            if (p.x < ul.x || p.x >= lr.x || p.y < ul.y || p.y >= lr.y) {
                // not in ROI
                return FLT_MAX;
            }
            return dist.accessor()(dist.upperLeft()+(p-ul));
        }

    typename RemapImage::value_type get(const vigra::Diff2D & p)
        {
//            vigra::Diff2D lr = ul + image.size() - vigra::Diff2D(1,1);
//            if (p.x < ul.x || p.x >= lr.x || p.y < ul.y || p.y >= lr.y) {
//                DEBUG_DEBUG("point not in ROI, p: " << p << " ul: " << ul << " lr: " << lr);
//            }
            return image.accessor()(image.upperLeft()+(p-ul));
        }

};


/** remaps transform image into output panorama.
 *
 *  This is a front end function for transformImage(),
 *  that calculates the ROI and uses the Panorama Object
 *  to setup the Transform used.
 *
 *  @param pano Panorama
 *  @param imgNr number of Image
 *  @param src  source image
 *  @param opts Panorama options that specify the output
 *  @param dest panorama image (will be modified in size, therefor, we
 *                              need a full image)
 *  @param dist distance image for @p dest (might be resized as well)
 *
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageType, class DistImageType, 
          class Interpolator>
//          class DestImageIterator, class DestAccessor,
//          class DistImageIterator, class DistAccessor>
void remapImage(const PT::Panorama & pano, unsigned int imgNr,
                vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                const PT::PanoramaOptions & opts,
                RemappedPanoImage<DestImageType, DistImageType> & remapped,
                Interpolator interp)

//                vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
//                vigra::Diff2D destUL,
//                vigra::triple<DistImageIterator, DistImageIterator, DistAccessor> centerDist)
{

    vigra::Diff2D srcSize(src.second-src.first);

    // create transforms
    SpaceTransform t;
    SpaceTransform invT;
    t.createTransform(pano, imgNr, opts, srcSize);
    invT.createInvTransform(pano, imgNr, opts, srcSize);

    // calculate ROI for this image.
    // outline of this image in final panorama
    std::vector<FDiff2D> outline;
    FDiff2D ul;
    FDiff2D lr;
    PTools::calcBorderPoints(srcSize, invT, back_inserter(outline),
                             ul, lr);
    DEBUG_DEBUG("imgnr: " << imgNr << " ROI: " << ul << ", " << lr << endl);

    // create an image with the right size..
    vigra::Diff2D ulInt((int)floor(ul.x), (int)floor(ul.y));
    vigra::Diff2D lrInt((int)ceil(lr.x), (int)ceil(lr.y));

    remapped.ul = ulInt;
    remapped.image.resize(lrInt-ulInt);
    remapped.dist.resize(lrInt-ulInt);

    // remap image with that transform
    // FIXME: further optimisation by using outline possible
    PTools::transformImage(src,
                           vigra::destImageRange(remapped.image),
                           ulInt,
                           t,
                           vigra::destImageRange(remapped.dist),
                           interp);
}

/** Transform an image into the panorama
 *
 *  It can be used for partial transformations as well, if the boundig
 *  box of a remapped image is known.
 *
 *  Usage: create an output image @dest that should contain the remapped
 *         @p src image. if @p dest doesn't cover the whole output panorama,
 *         use @p destUL to specify the offset of @p dest from the output
 *         panorama.
 *
 *  @param src    source image
 *  @param dest   (partial) panorama image. the image size needed to
 *                hold the complete remapped image can be calculated using
 *                calcBorderPoints().
 *  @param destUL upper left point of @p dest in final panorama. set to (0,0)
 *                if @p dest has the same size as the complete panorama.
 *  @param transform function used to remap the picture.
 *  @param centerDist image, with the same size as dest, that will contain the
 *                distance of the corrosponding pixel from the center of @p
 *                src. This is useful to calculate nice seams. Use a null
 *                image if this information is not needed.
 *  @param interp Interpolator class (calculates weights for interpolation)
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class SpaceTransform,
          class DistImageIterator, class DistAccessor,
          class Interpolator>
void transformImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                    vigra::Diff2D destUL,
                    SpaceTransform & transform,
                    vigra::triple<DistImageIterator, DistImageIterator, DistAccessor> centerDist,
                    Interpolator interp)
{
    vigra::Diff2D destSize = dest.second - dest.first;
    vigra::Diff2D distSize = centerDist.second - centerDist.first;

    bool calcDist=true;
    if (distSize.x == 0 && distSize.y == 0) {
        calcDist=false;
    }

    if (calcDist) {
        DEBUG_ASSERT(distSize == destSize);
    }
    int xstart = destUL.x;
    int xend   = destUL.x + destSize.x;
    int ystart = destUL.y;
    int yend   = destUL.y + destSize.y;

    vigra::Diff2D srcSize = src.second - src.first;
    // FIXME: use d & e here.
    vigra::Diff2D srcMiddle = srcSize / 2;

//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

    //InterpolatingAccessor(src.third, interp);
    InterpolatingAccessor<SrcAccessor, 
                          typename SrcAccessor::value_type,
                          Interpolator> interpol(src.third, interp);
    
    
//    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

//    vigra::BilinearInterpolatingAccessor interpol(src.third);

    // create dest y iterator
    DestImageIterator yd(dest.first);
    // create dist y iterator
    DistImageIterator ydist(centerDist.first);
    // loop over the image and transform
    for(int y=ystart; y < yend; ++y, ++yd.y, ++ydist.y)
    {
        // create x iterators
        DestImageIterator xd(yd);
        DistImageIterator xdist(ydist);
        for(int x=xstart; x < xend; ++x, ++xd.x, ++xdist.x)
        {
            double sx,sy;
            transform.transformImgCoord(sx,sy,x,y);
            // make sure that the interpolator doesn't
            // access pixels outside.. Should we introduce
            // some sort of border treatment?
            if (sx < interp.size/2 -1
                || sx > srcSize.x-interp.size/2 - 1
                || sy < interp.size/2 - 1  
                || sy > srcSize.y-interp.size/2 - 1)
            {
                if (calcDist) {
                    // save an invalid distance
                    *xdist = FLT_MAX;
                }
                // nothing..
            } else {
//                cout << x << "," << y << " -> " << sx << "," << sy << " " << endl;

//                nearest neighbour
//                *xd = src.third(src.first, vigra::Diff2D((int)round(sx), (int)round(sy)));
                // use given interpolator function.
                *xd = interpol(src.first, sx, sy);
                if (calcDist) {
                    double mx = sx - srcMiddle.x;
                    double my = sy - srcMiddle.y;
                    *xdist = sqrt(mx*mx + my*my);
                }
            }
        }
    }
}


/** Calculate the outline of the current image.
 *
 *  @param srcSize Size of source picture ( an be small, to
 *                 if not every point is needed)
 *  @param transf  Transformation from image to pano
 *  @param result  insert border point into result container
 *  @param ul      Upper Left corner of the image roi
 *  @param lr      Lower right corner of the image roi
 */
template <class OutputIterator, class SpaceTransform>
void calcBorderPoints(vigra::Diff2D srcSize,
                              SpaceTransform & transf,
                              OutputIterator result,
                              FDiff2D & ul,
                              FDiff2D & lr)
{
    ul.x = DBL_MAX;
    ul.y = DBL_MAX;
    lr.x = DBL_MIN;
    lr.y = DBL_MIN;

    int x = 0;
    int y = 0;

    for (x=0; x<srcSize.x ; x++) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }
    x = srcSize.x;
    for (y=0; y<srcSize.y ; y++) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }
    y = srcSize.y;
    for (x=srcSize.x-1; x>0 ; --x) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }
    x = 0;
    for (y=srcSize.y-1 ; y > 0 ; --y) {
        double sx,sy;
        transf.transformImgCoord(sx,sy,x,y);
        if (ul.x > sx) ul.x = sx;
        if (ul.y > sy) ul.y = sy;
        if (lr.x < sx) lr.x = sx;
        if (lr.y < sy) lr.y = sy;
        *result = FDiff2D((float)sx, (float) sy);
    }
    DEBUG_DEBUG("bounding box: upper left: " << ul.x << "," << ul.y
                << "  lower right: " << lr.x << "," << lr.y);
}


template <class T, int nr>
void fillVector(T vec[3], T &val, int len)
{
    for (int i=0; i<len; i++) vec[i] = val;
}


} // namespace PT

#endif
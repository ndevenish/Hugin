// -*- c-basic-offset: 4 -*-
/** @file PanoToolsInterface.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef PT_PANOTOOLSINTERFACE_H
#define PT_PANOTOOLSINTERFACE_H

#include "common/math.h"

#include "vigra/accessor.hxx"
#include "vigra/interpolating_accessor.hxx"

#include "PT/Panorama.h"
#include "PT/PanoramaMemento.h"

extern "C" {
#include "../pano12/panorama.h"
#include "../pano12/filter.h"
}

class wxImage;

namespace vigra
{
    class Diff2D;
}

/** mainly consists of wrapper around the pano tools library,
 *  to assist in ressource management and to provide a
 *  nicer interface.
 *
 *  It can be used to feed data from our model directly into the
 *  panotools library
 */
namespace PTools {



/** Holds transformations for Image -> Pano and the other way */
class Transform
{
public:
    /** construct a new Transform object, without 
     *  initializing a transformation
     *
     *  use createTransform or createInvTransform to create a
     *  transformation, and transform to execute it.
     */
    Transform();

    ~Transform();

    /** initialize pano->image transformation
     *
     *  Steps of transform:
     *
     *  1. pano_proj -> erect
     *  2. rotate equirect?? ( rotate yaw in screenpoints ? )
     *  3. spherical -> erect ??
     *  4. persp_sphere ( rotate point with rotation matrix)
     *  5. sphere_tp -> image_proj
     *  6. distortion correction
     *  7. shift image origin
     *  8. shear image
     */
    void createTransform(const vigra::Diff2D & srcSize,
                         const PT::VariableMap & srcVars,
                         PT::Lens::LensProjectionFormat srcProj,
                         const vigra::Diff2D &destSize,
                         PT::PanoramaOptions::ProjectionFormat destProj,
                         double destHFOV);

    /** create pano -> img transform */
    void createTransform(const PT::Panorama & pano, unsigned int imgNr,
                         const PT::PanoramaOptions & dest,
                         vigra::Diff2D srcSize=vigra::Diff2D(0,0));


    /** create image->pano transformation */
    void createInvTransform(const vigra::Diff2D & srcSize,
                            const PT::VariableMap & srcVars,
                            PT::Lens::LensProjectionFormat srcProj,
                            const vigra::Diff2D & destSize,
                            PT::PanoramaOptions::ProjectionFormat destProj,
                            double destHFOV);

    /** create image->pano transformation */
    void createInvTransform(const PT::Panorama & pano, unsigned int imgNr,
                            const PT::PanoramaOptions & dest,
                            vigra::Diff2D srcSize=vigra::Diff2D(0,0));

    /** excecute transform
     */
    void transform(double & x_dest, double & y_dest,
                   double x_src, double y_src)
        {
            execute_stack(x_src, y_src, &x_dest, &y_dest, &m_stack);
        }

    /** like transform, but return image coordinates, not cartesian
     *  coordinates
     */
    void transformImgCoord(double & x_dest, double & y_dest,
                   double x_src, double y_src)
        {
            x_src -= m_srcTX;
            y_src -= m_srcTY;

            execute_stack(x_src, y_src, &x_dest, &y_dest, &m_stack);
            x_dest += m_destTX;
            y_dest += m_destTY;
        }

    void transform(FDiff2D& dest, const FDiff2D & src)
        {
            double x_dest, y_dest;
            execute_stack(src.x, src.y, &x_dest, &y_dest, &m_stack);
            dest.x = x_dest;
            dest.y = y_dest;
        }

private:

    // update internal PT data structs.
    void updatePTData(const vigra::Diff2D &srcSize,
                      const PT::VariableMap & srcVars,
                      PT::Lens::LensProjectionFormat & srcProj,
                      const vigra::Diff2D & destSize,
                      PT::PanoramaOptions::ProjectionFormat & destProj,
                      double destHFOV);

    bool m_initialized;

    Image m_srcImage;
    Image m_dstImage;
    struct MakeParams	m_mp;
    struct fDesc 	m_stack[15];

    // used to convert from screen to cartesian coordinates
    double m_srcTX, m_srcTY;
    double m_destTX, m_destTY;

    // private, no copy constructor for the pt structures yet.
    Transform(const Transform &);
    Transform & operator=(const Transform &);
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
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageType, class DistImageType>
//          class DestImageIterator, class DestAccessor,
//          class DistImageIterator, class DistAccessor>
void remapImage(const PT::Panorama & pano, unsigned int imgNr,
                vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                const PT::PanoramaOptions & opts,
                RemappedPanoImage<DestImageType, DistImageType> & remapped)

//                vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
//                vigra::Diff2D destUL,
//                vigra::triple<DistImageIterator, DistImageIterator, DistAccessor> centerDist)
{

    vigra::Diff2D srcSize(src.second-src.first);

    // create transforms
    Transform t;
    Transform invT;
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
                           vigra::destImageRange(remapped.dist));
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
 *
 */
template <class SrcImageIterator, class SrcAccessor,
          class DestImageIterator, class DestAccessor,
          class Transform,
          class DistImageIterator, class DistAccessor>
void transformImage(vigra::triple<SrcImageIterator, SrcImageIterator, SrcAccessor> src,
                    vigra::triple<DestImageIterator, DestImageIterator, DestAccessor> dest,
                    vigra::Diff2D destUL,
                    Transform & transform,
                    vigra::triple<DistImageIterator, DistImageIterator, DistAccessor> centerDist)
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


    vigra::BilinearInterpolatingAccessor<SrcAccessor, typename SrcAccessor::value_type> interpol(src.third);

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
            if (sx < 0 || sx > srcSize.x-1
                || sy < 0 || sy > srcSize.y-1)
            {
                if (calcDist) {
                    // save an invalid distance
                    *xdist = FLT_MAX;
                }
                // nothing..
            } else {
//                cout << x << "," << y << " -> " << sx << "," << sy << " " << endl;

//                *xd = src.third(src.first, vigra::Diff2D((int)round(sx), (int)round(sy)));
                // do a nearest neighbour interpolation
                // fixme, border check here?
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
template <class OutputIterator, class Transform>
void calcBorderPoints(vigra::Diff2D srcSize,
                              Transform & transf,
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


/** set an output image, with properties from @p opts,
 *  that points to the bitmap data of @p imgData
 */
void setDestImage(Image & image, vigra::Diff2D size, unsigned char * imageData,
                  const PT::PanoramaOptions::ProjectionFormat & format,
                  double destHFOV);

/** fills @p image with a complete input image, including distortion
 *  correction parameters if @p correctDistortions is set.
 */
void setFullImage(Image & image, vigra::Diff2D size, unsigned char * imageData,
                  const PT::VariableMap & vars,
                  const PT::Lens::LensProjectionFormat format,
                  bool correctDistortions);

// internal function, used by setFullImage() to set the distortion parameters
void initCPrefs(cPrefs & p, const PT::VariableMap &vars);

/** create an empty aPrefs structure, suitable for transforming
 *  a input picture into an output picture.
 *
 *  the input/output pictures must be specified with: setAdjustSrcImg()
 *  and setAdjustDestImg()
 */
void createAdjustPrefs(aPrefs  & p, TrformStr & transf);

/** set a new input image for inserting into the panorama.
 */
void setAdjustSrcImg(TrformStr & trf, aPrefs & ap,
                     int width, int height, unsigned char * imageData,
                     const PT::VariableMap & vars,
                     const PT::Lens::LensProjectionFormat format,
                     bool correctDistortions);


/** set a new output image for the panorama */
void setAdjustDestImg(TrformStr & trf, aPrefs & ap,
                      int width, int height, unsigned char * imageData,
                      const PT::PanoramaOptions & opts);


void freeTrform(TrformStr & trf);


/** prepare a Trform struct for the adjust operation, image -> pano
 *  see use createAdjustPrefs(), setAdjustSrcImg() and setAdjustDestImg()
 *  to specify the images and transformation options
 */
void createAdjustTrform(TrformStr & trf);

/** free the resources associated with a TrformStr.
 *   createAdjustTrform() must have been used to create @p trf
 */
void freeTrform(TrformStr & trf);

/** free the pointer storage needed by Image
 *
 *  does NOT free the data referenced by image
 */
void freeImage(Image &img);

} // namespace

#endif // PT_PANOTOOLSINTERFACE_H

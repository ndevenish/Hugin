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

#include "PT/Interpolators.h"
#include "PT/Panorama.h"
#include "PT/PanoramaMemento.h"

extern "C" {
    #include <pano12/panorama.h>
    #include <pano12/filter.h>
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
            x_src -= m_srcTX - 0.5 ;
            y_src -= m_srcTY - 0.5;

            execute_stack(x_src, y_src, &x_dest, &y_dest, &m_stack);
            x_dest += m_destTX - 0.5;
            y_dest += m_destTY - 0.5;
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
    PT::transformImage(src,
                       vigra::destImageRange(remapped.image),
                       ulInt,
                       t,
                       vigra::destImageRange(remapped.dist),
                       interp);
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


/** set variables to optimize */
void setOptVars(optVars & opt, const std::set<std::string> & optvars);


/** class around the central align info struct of the panotools
 *  library
 */
class AlignInfoWrap
{
public:
    AlignInfoWrap();

    ~AlignInfoWrap();
    bool setInfo(const PT::Panorama & pano,
                 const PT::UIntVector & imgs,
                 const PT::OptimizeVector & optvec);

    void setGlobal()
    {
        SetGlobalPtr(& gl);
    }

    /** get the variables stored in this AlignInfo */
    PT::VariableMapVector getVariables() const;

    AlignInfo gl;
};

} // namespace

#endif // PT_PANOTOOLSINTERFACE_H

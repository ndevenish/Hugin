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

#include <PT/Panorama.h>
#include <PT/PanoramaMemento.h>
#include <common/math.h>

extern "C" {
#include <pano12/panorama.h>
#include <pano12/filter.h>
}

// remove fu'*!%$# min & max macros, that come from some windows include
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

class wxImage;

namespace vigra{
    struct Diff2D;
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
    /** construct a new Transform object, without a transformation
     *
     *  use createTransform or createInvTransform to create a
     *  transformation, and transform to execute it.
     */
    Transform();

    ~Transform();

    /** pano -> image transformation
     */
    void createTransform(const PT::Panorama & pano, unsigned int imgNr, const PT::PanoramaOptions & opts);

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


    /** initialize pano ->img transformation */
    void createInvTransform(const vigra::Diff2D & srcSize,
                            const PT::VariableMap & srcVars,
                            PT::Lens::LensProjectionFormat srcProj,
                            const vigra::Diff2D & destSize,
                            PT::PanoramaOptions::ProjectionFormat destProj,
                            double destHFOV);


    /** excecute transform
     */
    void transform(double & x_dest, double & y_dest,
                   double x_src, double y_src)
        {
            execute_stack(x_src, y_src, &x_dest, &y_dest, &m_stack);
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

    // private, no copy constructor for the pt structures yet.
    Transform(const Transform &);
    Transform & operator=(const Transform &);
};



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
void PTools::freeImage(Image &img);

} // namespace

#endif // PT_PANOTOOLSINTERFACE_H

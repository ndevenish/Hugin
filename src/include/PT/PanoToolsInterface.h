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

//#include "vigra_ext/Interpolators.h"

#include "PT/Panorama.h"
#include "PT/PanoramaMemento.h"

extern "C" {
    #include <pano12/panorama.h>
    #include <pano12/filter.h>
}

// remove stupid #defines from the evil windows.h
#ifdef __WXMSW__
#include <wx/msw/winundef.h>
#undef DIFFERENCE
#endif


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
                         PT::VariableMap srcVars,
                         PT::Lens::LensProjectionFormat srcProj,
                         const vigra::Diff2D &destSize,
                         PT::PanoramaOptions::ProjectionFormat destProj,
                         double destHFOV,
                         const vigra::Diff2D & origSrcSize);

    /** create pano -> img transform */
    void createTransform(const PT::Panorama & pano, unsigned int imgNr,
                         const PT::PanoramaOptions & dest,
                         vigra::Diff2D srcSize=vigra::Diff2D(0,0));


    /** create image->pano transformation 
     *
     *  @param srcSize size of input image
     *  @param variables of input image
     *  @param srcProj projection of the image
     *  @param destSize  output panorama size  
     *  @param destProj  panorama projection
     *  @param destHFOV  HFOV of panorama
     *  @param origSrcSize  original input image size, 0,0 if the same
     *                      as srcSize.
     *
     *  origSrcSize is needed, because the @p variables are only
     *  valid for the original input size. To transform a smaller
     *  image, like a preview image, the parameters have to be adjusted.
     *  The origial image size, for which @p variables are valid needs
     *  to be know for this.
     */
    void createInvTransform(const vigra::Diff2D & srcSize,
                            PT::VariableMap srcVars,
                            PT::Lens::LensProjectionFormat srcProj,
                            const vigra::Diff2D & destSize,
                            PT::PanoramaOptions::ProjectionFormat destProj,
                            double destHFOV,
                            const vigra::Diff2D & origSrcSize);

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
q *  see use createAdjustPrefs(), setAdjustSrcImg() and setAdjustDestImg()
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
                 const PT::VariableMapVector & variables,
                 const PT::CPVector & controlPoints,
                 const PT::OptimizeVector & optvec);

    void setGlobal()
    {
        SetGlobalPtr(& gl);
    }

    /** get the variables stored in this AlignInfo */
    PT::VariableMapVector getVariables() const;
    const PT::CPVector & getCtrlPoints();

    std::map<int,int> m_ctrlPointMap;
    PT::CPVector m_controlPoints;

    AlignInfo gl;
};

} // namespace

#endif // PT_PANOTOOLSINTERFACE_H

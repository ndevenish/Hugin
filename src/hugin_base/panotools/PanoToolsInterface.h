// -*- c-basic-offset: 4 -*-
/** @file panotools/PanoToolsInterface.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PANOTOOLS_PANOTOOLSINTERFACE_H
#define _PANOTOOLS_PANOTOOLSINTERFACE_H

#include <hugin_shared.h>
#include <hugin_config.h>

#include <iostream>
#include <string>
#include <set>

#include <hugin_math/hugin_math.h>
#include <panodata/PanoramaVariable.h>
#include <panodata/Lens.h>
#include <panodata/PanoramaOptions.h>
#include <panodata/SrcPanoImage.h>
#include <panodata/ControlPoint.h>


// libpano includes ------------------------------------------------------------

#ifdef _WIN32
// include windows.h with sensible defines, otherwise
// panotools might include with its stupid, commonly
// named macros all over the place.
#define _STLP_VERBOSE_AUTO_LINK
//#define _USE_MATH_DEFINES
#define NOMINMAX
#define VC_EXTRALEAN
#include <windows.h>
#undef DIFFERENCE
#undef min
#undef max
#undef MIN
#undef MAX
#endif


extern "C" {
#ifdef __INTEL__
#define __INTELMEMO__
#undef __INTEL__
#endif

#include <pano13/panorama.h>
#include <pano13/filter.h>

#ifdef __INTELMEMO__
#define __INTEL__
#undef __INTELMEMO__
#endif

#include <pano13/filter.h>

// somehow these are still set after panorama.h has been included
#undef DIFFERENCE
#undef min
#undef max
#undef MIN
#undef MAX

}

//------------------------------------------------------------------------------


namespace vigra { class Diff2D; }
namespace HuginBase { class PanoramaData; }



/** mainly consists of wrapper around the pano tools library,
 *  to assist in ressource management and to provide a
 *  nicer interface.
 *
 *  It can be used to feed data from our model directly into the
 *  panotools library
 */
namespace HuginBase { namespace PTools {


/** Holds transformations for Image -> Pano and the other way */
class IMPEX Transform
{
        
    public:
        /** construct a new Transform object, without
         *  initializing a transformation
         *
         *  use createTransform or createInvTransform to create a
         *  transformation, and transform to execute it.
         */
        Transform()
          : m_initialized(false), m_srcTX(0), m_srcTY(0),
            m_destTX(0), m_destTY(0)
        {
            // initialize pointer
            m_srcImage.data = NULL;
            m_dstImage.data = NULL;
        }

        ///
        ~Transform();
        
    private:
        // private, no copy constructor for the pt structures yet.
        Transform(const Transform &);
        Transform & operator=(const Transform &);

        
    public:
        /** @todo The next few functions could be rewritten to make more
         * effective use of SrcPanoImages.
         */
        
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
                             VariableMap srcVars,
                             Lens::LensProjectionFormat srcProj,
                             const vigra::Diff2D &destSize,
                             PanoramaOptions::ProjectionFormat destProj,
                             const std::vector<double> & destProjParam,
                             double destHFOV,
                             const vigra::Diff2D & origSrcSize);

        /** create pano -> img transform */
        void createTransform(const PanoramaData& pano, unsigned int imgNr,
                             const PanoramaOptions & dest,
                             vigra::Diff2D srcSize=vigra::Diff2D(0,0));

        ///
        void createTransform(const SrcPanoImage & src, const PanoramaOptions & dest);

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
                                VariableMap srcVars,
                                Lens::LensProjectionFormat srcProj,
                                const vigra::Diff2D & destSize,
                                PanoramaOptions::ProjectionFormat destProj,
                                const std::vector<double> & destProjParam,
                                double destHFOV,
                                const vigra::Diff2D & origSrcSize);

        /** create image->pano transformation */
        void createInvTransform(const PanoramaData& pano, unsigned int imgNr,
                                const PanoramaOptions & dest,
                                vigra::Diff2D srcSize=vigra::Diff2D(0,0));

        /** create image->pano transformation */
        void createInvTransform(const SrcPanoImage & src, const PanoramaOptions & dest);

        /** excecute transform
         */
        bool transform(double & x_dest, double & y_dest,
                       double x_src, double y_src) const;

        ///
        bool transform(hugin_utils::FDiff2D& dest, const hugin_utils::FDiff2D & src) const;

        /** like transform, but return image coordinates, not cartesian
         *  coordinates
         */
        bool transformImgCoord(double & x_dest, double & y_dest,
                               double x_src, double y_src) const;

        bool transformImgCoordPartial(double & x_dest, double & y_dest, double x_src, double y_src) const;

        ///
        bool transformImgCoord(hugin_utils::FDiff2D& dest, const hugin_utils::FDiff2D & src) const
            { return transformImgCoord(dest.x, dest.y, src.x, src.y); }


        bool emitGLSL(std::ostringstream& oss) const;
        
    private:
        // update internal PT data structs.
        void updatePTData(const vigra::Diff2D &srcSize,
                          const VariableMap & srcVars,
                          Lens::LensProjectionFormat & srcProj,
                          const vigra::Diff2D & destSize,
                          PanoramaOptions::ProjectionFormat & destProj,
                          const std::vector<double> & destProjParam,
                          double destHFOV);

        
    private:
        bool m_initialized;

        Image m_srcImage;
        Image m_dstImage;
        struct MakeParams	m_mp;
        struct fDesc 	m_stack[15];

        // used to convert from screen to cartesian coordinates
        double m_srcTX, m_srcTY;
        double m_destTX, m_destTY;
        
};


/** set an output image, with properties from @p opts,
 *  that points to the bitmap data of @p imgData
 */
IMPEX void setDestImage(Image & image, vigra::Diff2D size, unsigned char * imageData,
                  const PanoramaOptions::ProjectionFormat & format,
                  const std::vector<double> & projParams,
                  double destHFOV);

/** fills @p image with a complete input image, including distortion
 *  correction parameters if @p correctDistortions is set.
 */
IMPEX void setFullImage(Image & image, vigra::Diff2D size, unsigned char * imageData,
                  const VariableMap & vars,
                  const Lens::LensProjectionFormat format,
                  bool correctDistortions);

/** free the pointer storage needed by Image
 *
 *  does NOT free the data referenced by image
 */
IMPEX void freeImage(Image &img);

///
IMPEX VariableMapVector GetAlignInfoVariables(const AlignInfo & gl);

///
IMPEX CPVector GetAlignInfoCtrlPoints(const AlignInfo & gl);



}} // namespace

#endif // _H

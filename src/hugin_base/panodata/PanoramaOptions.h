// -*- c-basic-offset: 4 -*-
/** @file PanoramaOptions.h
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

#ifndef _PANODATA_PANORAMAMEMENTO_H
#define _PANODATA_PANORAMAMEMENTO_H

#include <hugin_config.h>

#include <string>
#include <vector>
//#include <map>
//#include <algorithm>
//#include <set>
//#include <math.h>
#include <vigra/diff2d.hxx>

#include <vigra/windows.h>

extern "C" {

    #ifdef __INTEL__
    #define __INTELMEMO__
    #undef __INTEL__
    #endif

    #include <pano13/panorama.h>

    #ifdef __INTELMEMO__
    #define __INTEL__
    #undef __INTELMEMO__
    #endif

    // remove stupid #defines from the evil windows.h

#ifdef DIFFERENCE
#undef DIFFERENCE
#endif
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef MIN
#undef MIN
#endif
#ifdef MAX
#undef MAX
#endif

    }

//
//#include "PT/PanoImage.h"

#include <hugin_shared.h>
#include <vigra_ext/Interpolators.h>

namespace HuginBase {

/** Panorama image options
 *
 *  this holds the settings for the final panorama
 */
class IMPEX PanoramaOptions
{
        
    public:
        /** Projection of final panorama
         */
        enum ProjectionFormat {
            RECTILINEAR = 0,
            CYLINDRICAL = 1,
            EQUIRECTANGULAR = 2,
            FULL_FRAME_FISHEYE = 3,
            STEREOGRAPHIC = 4,
            MERCATOR = 5,
            TRANSVERSE_MERCATOR = 6,
            SINUSOIDAL = 7,
            LAMBERT = 8,
            LAMBERT_AZIMUTHAL = 9,
            ALBERS_EQUAL_AREA_CONIC = 10,
            MILLER_CYLINDRICAL = 11,
            PANINI = 12,
            ARCHITECTURAL = 13,
            ORTHOGRAPHIC = 14,
            EQUISOLID = 15,
            EQUI_PANINI = 16,
            BIPLANE = 17,
            TRIPLANE = 18,
            GENERAL_PANINI = 19,
            THOBY_PROJECTION = 20,
            HAMMER_AITOFF = 21,
        };

        /** PTStitcher acceleration */
        enum PTStitcherAcceleration {
            NO_SPEEDUP,
            MAX_SPEEDUP,
            MEDIUM_SPEEDUP  // for projects with morphing.
        };

        /** Fileformat
         */
        enum FileFormat {
            JPEG = 0,
            PNG,
            TIFF,
            TIFF_m,
            TIFF_mask,
            TIFF_multilayer,
            TIFF_multilayer_mask,
            PICT,
            PSD,
            PSD_m,
            PSD_mask,
            PAN,
            IVR,
            IVR_java,
            VRML,
            QTVR,
            HDR,
            HDR_m,
            EXR,
            EXR_m,
            FILEFORMAT_NULL
        };

        /** output mode */
        enum OutputMode {
            OUTPUT_LDR=0,
            OUTPUT_HDR
        };

        enum HDRMergeType {
            HDRMERGE_AVERAGE=0,
            HDRMERGE_DEGHOST=1
        };

        /** blenders */
        enum BlendingMechanism {
            NO_BLEND=0,
            PTBLENDER_BLEND=1,
            ENBLEND_BLEND=2,
            SMARTBLEND_BLEND=3,
            PTMASKER_BLEND=4
        };

        ///
        enum Remapper {
            NONA=0,
            PTMENDER
        };

        /** type of color correction
         */
        enum ColorCorrection {
            NONE = 0,
            BRIGHTNESS_COLOR,
            BRIGHTNESS,
            COLOR
        };

    public:
        PanoramaOptions()
        {
            reset();
        };

        virtual ~PanoramaOptions() {};

        virtual void reset()
        {
            m_projectionFormat = EQUIRECTANGULAR;
            m_hfov = 360;
            m_size = vigra::Size2D(3000, 1500);
            m_roi = vigra::Rect2D(m_size);
            outfile = "panorama";
            tiff_saveROI = true;
            tiffCompression = "LZW";
            quality = 100;
            colorCorrection = NONE;
            colorReferenceImage = 0;
            optimizeReferenceImage = 0;
            gamma = 1.0;
            interpolator = vigra_ext::INTERP_CUBIC;
            // featherWidth = 10;
            outputFormat = TIFF_m;
            remapAcceleration = MAX_SPEEDUP;
            blendMode = ENBLEND_BLEND;
            hdrMergeMode = HDRMERGE_AVERAGE;
            remapper = NONA;
            remapUsingGPU = false;
            saveCoordImgs = false;
            huberSigma = 2;
            photometricHuberSigma = 2/255.0;
            photometricSymmetricError = false;
            outputMode = OUTPUT_LDR;

            outputLDRBlended = true;
            outputLDRLayers = false;
            outputLDRExposureRemapped = false;
            outputLDRExposureLayers = false;
            outputLDRExposureLayersFused = false;
            outputLDRStacks = false;
            outputLDRExposureBlended = false;
            outputHDRBlended = false;
            outputHDRLayers = false;
            outputHDRStacks = false;

            outputLayersCompression = "LZW";
            outputImageType = "tif";
            outputImageTypeCompression = "LZW";
            outputImageTypeHDR= "exr";
            outputImageTypeHDRCompression = "LZW";

            enblendOptions = "";
            enfuseOptions  = "";
            hdrmergeOptions = "";

            outputEMoRParams.resize(5,0.0);
            outputExposureValue = 0.0;
            outputPixelType = "";

            panoProjectionFeaturesQuery(m_projectionFormat, &m_projFeatures);
            resetProjectionParameters();

            outputStacksMinOverlap = 0.7;
            outputLayersExposureDiff = 0.5;
        }

    public:
        ///
        void printScriptLine(std::ostream & o,bool forPTOptimizer=false) const;

        /// return string name of output file format
        static const std::string & getFormatName(FileFormat f);

        /** returns the FileFormat corrosponding to name.
         *
         *  if name is not recognized, FileFormat::TIFF is returned
         */
        static FileFormat getFormatFromName(const std::string & name);

        /** return the extension used by the current output format */
        const std::string & getOutputExtension() const;

    public:
        /** set panorama width 
         *  keep the HFOV, if keepView=true
         */
        void setWidth(unsigned int w, bool keepView = true);

        /** set panorama height 
         *
         *  This changes the panorama vfov
         */
        void setHeight(unsigned int h);

        /* get panorama width */
        unsigned int getWidth() const
            { return m_size.x; }

        /** get panorama height */
        unsigned int getHeight() const
            {return m_size.y;}

        /// get size of output image
        vigra::Size2D getSize() const
            { return m_size; }

        ///
        const vigra::Rect2D & getROI() const
            { return m_roi; }

        ///
        void setROI(const vigra::Rect2D & val)
            { m_roi = val & vigra::Rect2D(m_size); }

        /** set the Projection format and adjust the hfov/vfov
         *  if nessecary
         */
        void setProjection(ProjectionFormat f);

        /** sets the optional parameters to their default values */
        void resetProjectionParameters();

        ///
        PanoramaOptions::ProjectionFormat getProjection() const
            { return m_projectionFormat; };

        /** Get the optional projection parameters */
        const std::vector<double> & getProjectionParameters() const
            { return m_projectionParams; }

        /** set the optional parameters (they need to be of the correct size) */
        void setProjectionParameters(const std::vector<double> & params);

        /** true, if FOV calcuations are supported for projection \p f */
        bool fovCalcSupported(ProjectionFormat f) const;

        /** set the horizontal field of view.
         *  also updates the image height (keep pano
         *  field of view similar.)
         */
        void setHFOV(double h, bool keepView=true);

        ///
        double getHFOV() const
            { return m_hfov; }
        
        ///
        void setVFOV(double v);
        
        ///
        double getVFOV() const;

        /** get maximum possible hfov with current projection */
        double getMaxHFOV() const
            { return m_projFeatures.maxHFOV; }

        /** get maximum possible vfov with current projection */
        double getMaxVFOV() const
            { return m_projFeatures.maxVFOV; }

    public:
        //TODO: Write accessor methods; make instance variables private unless absolutely neccesary for backward-compatibility.
        
        std::string outfile;
        FileFormat outputFormat;

        // jpeg options
        int quality;

        // TIFF options
        std::string tiffCompression;
        bool tiff_saveROI;

        ColorCorrection colorCorrection;
        unsigned int colorReferenceImage;

        // misc options
        double gamma;
        vigra_ext::Interpolator interpolator;

        unsigned int optimizeReferenceImage;
        // unsigned int featherWidth;

        PTStitcherAcceleration remapAcceleration;
        BlendingMechanism blendMode;
        HDRMergeType hdrMergeMode;
        Remapper remapper;
        bool remapUsingGPU;

        bool saveCoordImgs;

        double huberSigma;

        double photometricHuberSigma;
        double photometricSymmetricError;

        // modes related to high dynamic range output
        OutputMode outputMode;

        bool outputLDRBlended;         ///< save blended panorama (LDR)
        bool outputLDRLayers;          ///< save remapped layers (LDR)
        bool outputLDRExposureRemapped;///< save remapped layers (no exposure adjustment)
        bool outputLDRExposureLayers;  ///< save blended exposure layers, do not perform fusion (no exposure adjustment)
        bool outputLDRExposureLayersFused; ///< save blended exposure layers which are then fused (no exposure adjustment)
        bool outputLDRStacks;          /// < save exposure fused stacks (no exposure adjustment)
        bool outputLDRExposureBlended; ///< save blended exposure layers created from fused image stacks (no exposure adjustment)
        bool outputHDRBlended;         ///< save blended panorama (HDR)
        bool outputHDRLayers;          ///< save remapped layers (HDR)
        bool outputHDRStacks;          ///< save image stacks (HDR)

        std::string outputLayersCompression;
        std::string outputImageType;
        std::string outputImageTypeCompression;
        std::string outputImageTypeHDR;
        std::string outputImageTypeHDRCompression;

        std::string enblendOptions;
        std::string enfuseOptions;
        std::string hdrmergeOptions;

        // select the exposure of the output images in LDR mode.
        double outputExposureValue;
        std::vector<float> outputEMoRParams;

        // choose pixel type for output images.
        std::string outputPixelType;

        // parameters for generating output layers and stacks
        double outputStacksMinOverlap;
        double outputLayersExposureDiff;

        pano_projection_features m_projFeatures;

    private:
        static const std::string fileformatNames[];
        static const std::string fileformatExt[];
        double m_hfov;
        double m_vfov;
    //    unsigned int m_width;
    //    unsigned int m_height;
        ProjectionFormat m_projectionFormat;

        std::vector<double> m_projectionParams;
        vigra::Size2D m_size;
        vigra::Rect2D m_roi;
};


} // namespace
#endif // _H

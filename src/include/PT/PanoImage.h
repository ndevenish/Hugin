// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
 *
 *  @brief implementation of HFOVDialog Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef PANOIMAGE_H
#define PANOIMAGE_H

#include <iostream>
#include <vector>
#include <common/utils.h>
#include <common/math.h>
#include <vigra/diff2d.hxx>

namespace PT {

class Panorama;

/** optimization & stitching options. */
class ImageOptions {

public:
    ImageOptions()
    : featherWidth(10),
      ignoreFrameWidth(0),
      morph(false),
      docrop(false),
      autoCenterCrop(true),
      m_vigCorrMode(VIGCORR_RADIAL|VIGCORR_DIV),
      responseType(0),
      active(true)
     { };

    // PT state
    /// u10           specify width of feather for stitching. default:10
    unsigned int featherWidth;
    /// m20           ignore a frame 20 pixels wide. default: 0
    unsigned int ignoreFrameWidth;

    /// Morph-to-fit using control points.
    bool morph;

	// crop parameters
    bool docrop;
    bool autoCenterCrop;
    vigra::Rect2D cropRect;

    /// vignetting correction mode (bitflags, no real enum)
    enum VignettingCorrMode { 
        VIGCORR_NONE = 0,      ///< no vignetting correction
        VIGCORR_RADIAL = 1,    ///< radial vignetting correction
        VIGCORR_FLATFIELD = 2, ///< flatfield correction
        VIGCORR_DIV = 4        ///< correct by division.
    };
    int m_vigCorrMode;
    // coefficients for vignetting correction (even degrees: 0,2,4,6, ...)
    std::string m_flatfield;
    // the response type (Rt)
    int responseType;

    // is image active (displayed in preview and used for optimisation)
    bool active;
};



/** description of a source pano image... 
 *
 *  In the long term, this simplified class will replace
 *  PanoImage and Image options and the variables array.
 */
class SrcPanoImage
{
public:

    enum Projection { RECTILINEAR = 0,
                      PANORAMIC = 1,
                      CIRCULAR_FISHEYE = 2,
                      FULL_FRAME_FISHEYE = 3,
                      EQUIRECTANGULAR = 4 };

    enum CropMode { NO_CROP=0,
                    CROP_RECTANGLE=1,
                    CROP_CIRCLE=2 };

    /// vignetting correction mode (bitflags, no real enum)
    enum VignettingCorrMode { 
        VIGCORR_NONE = 0,      ///< no vignetting correction
        VIGCORR_RADIAL = 1,    ///< radial vignetting correction
        VIGCORR_FLATFIELD = 2, ///< flatfield correction
        VIGCORR_DIV = 4        ///< correct by division.
    };

    enum ResponseType {
        RESPONSE_EMOR=0,                 ///< empirical model of response
        RESPONSE_LINEAR,                 ///< linear response
        RESPONSE_GAMMA,                  ///< a simple gamma response curve
        RESPONSE_FILE,                   ///< load response curve from file (not implemented yet)
        RESPONSE_ICC                     ///< use ICC for transformation into linear data (not implemented yet)
    };

    SrcPanoImage()
    {
        setDefaults();
    }

    SrcPanoImage(const std::string &filename, vigra::Size2D size)
    {
        setDefaults();
        m_filename = filename;
        m_size = size ;
        m_cropRect = vigra::Rect2D(size);
    };

    bool operator==(const SrcPanoImage & other) const;

    void setDefaults()
    {
        m_proj = RECTILINEAR;
        m_hfov = 50;
        m_roll = 0;
        m_pitch = 0;
        m_yaw = 0;

        m_responseType = RESPONSE_EMOR;
        m_emorParams.resize(5);
        for (unsigned i=0; i < 5; i++) {
            m_emorParams[i] = 0;
        }
        m_exposure = 1;
        m_wbRed = 1;
        m_wbBlue = 1;

        m_gamma = 1;

        m_radialDist.resize(4);
        m_radialDistRed.resize(4);
        m_radialDistBlue.resize(4);
        for (unsigned i=0; i < 3; i++) {
            m_radialDist[i] = 0;
            m_radialDistRed[i] = 0;
            m_radialDistBlue[i] = 0;
        }
        m_radialDist[3] = 1;
        m_radialDistRed[3] = 1;
        m_radialDistBlue[3] = 1;
        m_centerShift.x = 0;
        m_centerShift.y = 0;
        m_shear.x = 0;
        m_shear.y = 0;

        m_crop = NO_CROP;

        m_vigCorrMode = VIGCORR_RADIAL|VIGCORR_DIV;
        m_radialVigCorrCoeff.resize(4);
        m_radialVigCorrCoeff[0] = 1;
        for (unsigned i=1; i < 4; i++) {
            m_radialVigCorrCoeff[i] = 0;
        }

        m_exifCropFactor = 0;
        m_exifFocalLength = 0;

        m_lensNr = 0;
        m_featherWidth = 10;
        m_morph = false;
    }

    /** "resize" image,
     *  adjusts all distortion coefficients for usage with a source image
     *  of size @p size
     */
    void resize(const vigra::Size2D & size);


    /** check if a coordinate is inside the source image
     */
    bool isInside(vigra::Point2D p) const
    {
        switch(m_crop) {
            case NO_CROP:
            case CROP_RECTANGLE:
                return m_cropRect.contains(p);
            case CROP_CIRCLE:
            {
                if (0 > p.x || 0 > p.y || p.x >= m_size.x || p.y >= m_size.y) {
                    // outside image
                    return false;
                }
                FDiff2D cropCenter;
                cropCenter.x = m_cropRect.left() + m_cropRect.width()/2.0;
                cropCenter.y = m_cropRect.top() + m_cropRect.height()/2.0;
                double radius2 = std::min(m_cropRect.width()/2.0, m_cropRect.height()/2.0);
                radius2 = radius2 * radius2;
                FDiff2D pf = FDiff2D(p) - cropCenter;
                return (radius2 > pf.x*pf.x+pf.y*pf.y );
            }
        }
        // this should never be reached..
        return false;
    }

    bool horizontalWarpNeeded();

    // property accessors

    const std::string & getFilename() const
    { return m_filename; }
    void setFilename(const std::string & file)
    { m_filename = file; }

    const vigra::Size2D & getSize() const
    { return m_size; }
    void setSize(const vigra::Size2D & val)
    { m_size = val; }

    const Projection & getProjection() const
    { return m_proj; }
    void setProjection(const Projection & val)
    { m_proj = val; }

    const double & getHFOV() const
    { return m_hfov; }
    void setHFOV(const double & val)
    { m_hfov = val; }

    bool getCorrectTCA() const
    { 
        bool nr = (m_radialDistRed[0] == 0.0 && m_radialDistRed[1] == 0.0 &&
                  m_radialDistRed[2] == 0.0 && m_radialDistRed[3] == 1);
        bool nb = (m_radialDistBlue[0] == 0.0 && m_radialDistBlue[1] == 0.0 &&
                  m_radialDistBlue[2] == 0.0 && m_radialDistBlue[3] == 1);
        return !(nr && nb);
    }

    const std::vector<double> & getRadialDistortion() const
    { return m_radialDist; }
    void setRadialDistortion(const std::vector<double> & val)
    {
        DEBUG_ASSERT(val.size() == 4);
        m_radialDist = val; 
    }
    const std::vector<double> & getRadialDistortionRed() const
    { return m_radialDistRed; }
    void setRadialDistortionRed(const std::vector<double> & val)
    {
        DEBUG_ASSERT(val.size() == 4);
        m_radialDistRed = val; 
    }
    const std::vector<double> & getRadialDistortionBlue() const
    { return m_radialDistBlue; }
    void setRadialDistortionBlue(const std::vector<double> & val)
    {
        DEBUG_ASSERT(val.size() == 4);
        m_radialDistBlue = val; 
    }

    const FDiff2D & getRadialDistortionCenterShift() const
    { return m_centerShift; }
    void setRadialDistortionCenterShift(const FDiff2D & val)
    { m_centerShift = val; }

    FDiff2D getRadialDistortionCenter() const
    { return FDiff2D(m_size)/2.0 + m_centerShift; }

    const FDiff2D & getShear() const
    { return m_shear; }
    void setShear(const FDiff2D & val)
    { m_shear = val; }

    int getVigCorrMode() const
    { return m_vigCorrMode; }
    void setVigCorrMode(const int & val)
    { m_vigCorrMode = val; }

    const std::string & getFlatfieldFilename() const
    { return m_flatfield; }
    void setFlatfieldFilename(const std::string & val)
    { m_flatfield = val; }

    const std::vector<double> & getRadialVigCorrCoeff() const
    { return m_radialVigCorrCoeff; }
    void setRadialVigCorrCoeff(const std::vector<double> & val)
    { 
        DEBUG_ASSERT(val.size() == 4);
        m_radialVigCorrCoeff = val; 
    }

    const FDiff2D & getRadialVigCorrCenterShift() const
    { return m_radialVigCorrCenterShift; }
    void setRadialVigCorrCenterShift(const FDiff2D & val)
    { m_radialVigCorrCenterShift = val; }

    FDiff2D getRadialVigCorrCenter() const
    { return (FDiff2D(m_size)-FDiff2D(1,1))/2.0 + m_radialVigCorrCenterShift; }

    int getLensNr() const
    { return m_lensNr; }
    void setLensNr(const int & val)
    { m_lensNr = val; }

    CropMode getCropMode() const
    { return m_crop; }
    void setCropMode(CropMode val)
    {
        m_crop = val;
        if (m_crop == NO_CROP) {
            m_cropRect = vigra::Rect2D(m_size);
        }
    }

    const vigra::Rect2D & getCropRect() const
    { return m_cropRect; }
    void setCropRect(const vigra::Rect2D & val)
    { m_cropRect = val; }

    const double & getRoll() const
    { return m_roll; }
    void setRoll(const double & val)
    { m_roll = val; }
    const double & getPitch() const
    { return m_pitch; }
    void setPitch(const double & val)
    { m_pitch = val; }
    const double & getYaw() const
    { return m_yaw; }
    void setYaw(const double & val)
    { m_yaw = val; }

    /// get the exposure factor
    double getExposure() const
    { return 1.0/pow(2.0, m_exposure); }
    void setExposure(const double & val)
    { m_exposure = log2(1/val); }

    /// get the exposure value (log2 of inverse exposure factor)
    double getExposureValue() const
    { return m_exposure; }
    void setExposureValue(const double & val)
    { m_exposure = val; }

    double getGamma() const
    { return m_gamma; }
    void setGamma(double val)
    { m_gamma = val; }

    double getWhiteBalanceRed() const
    { return m_wbRed; }
    void setWhiteBalanceRed(double val)
    { m_wbRed = val; }
    double getWhiteBalanceBlue() const
    { return m_wbBlue; }
    void setWhiteBalanceBlue(double val)
    { m_wbBlue = val; }

    ResponseType getResponseType() const
    { return m_responseType; }
    void setResponseType(ResponseType val)
    { m_responseType = val; }

    const std::vector<float> & getEMoRParams() const
    { return m_emorParams; }
    void setEMoRParams(const std::vector<float> & val)
    { m_emorParams = val; }


    const std::string & getExifModel() const
    { return m_exifModel; }
    void setExifModel(const std::string & val)
    { m_exifModel = val; }

    const std::string & getExifMake() const
    { return m_exifMake; }
    void setExifMake(const std::string & val)
    { m_exifMake = val; }

    const double & getExifCropFactor() const
    { return m_exifCropFactor; }
    void setExifCropFactor(const double & val)
    { m_exifCropFactor = val; }

    const double & getExifFocalLength() const
    { return m_exifFocalLength; }
    void setExifFocalLength(const double & val)
    { m_exifFocalLength = val; }

    double getVar(const std::string & name) const;
    void setVar(const std::string & name, double val);

private:
    std::string m_filename;
//    VariableVector m_vars;
    vigra::Size2D m_size;

    Projection m_proj;
    double m_hfov;

    ResponseType m_responseType;
    std::vector<float> m_emorParams;
    double m_exposure;
    double m_gamma;
    double m_wbRed, m_wbBlue;

    // orientation in degrees
    double m_roll;
    double m_pitch;
    double m_yaw;

    // radial lens distortion
    std::vector<double> m_radialDist;
    // radial lens distortion (red, blue channel), for TCA correction
    std::vector<double> m_radialDistRed;
    std::vector<double> m_radialDistBlue;
    // Center shift
    FDiff2D m_centerShift;
    // shear
    FDiff2D m_shear;

    // crop description
    CropMode m_crop;
    vigra::Rect2D m_cropRect;

    int m_vigCorrMode;
    // coefficients for vignetting correction (even degrees: 0,2,4,6, ...)
    std::string m_flatfield;
    std::vector<double> m_radialVigCorrCoeff;
    FDiff2D m_radialVigCorrCenterShift;

    // linear pixel transform
    std::vector<double> m_ka;
    std::vector<double> m_kb;


    // store camera information from exif tags...
    std::string m_exifModel;
    std::string m_exifMake;
    double      m_exifCropFactor;
    double      m_exifFocalLength;

    unsigned m_lensNr;
    //
    // panotools options
    //
    // u10           specify width of feather for stitching. default:10
    unsigned int m_featherWidth;
    // Morph-to-fit using control points.
    bool m_morph;
};

/** try to fill out information about the image, by examining the exif data
 *  focalLength and cropFactor will be updated with the ones read from the exif data
 *  If no or not enought exif data was found and valid given focalLength and cropFactor
 *  settings where provided, they will be used for computation of the HFOV.
 */
bool initImageFromFile(SrcPanoImage & img, double & focalLength, double & cropFactor);

/** calculate hfov of an image given focal length, image size and crop factor */
double calcHFOV(SrcPanoImage::Projection proj, double fl, double crop, vigra::Size2D imageSize);

/** calcualte focal length, given crop factor and hfov */
double calcFocalLength(SrcPanoImage::Projection proj, double hfov, double crop, vigra::Size2D imageSize);

/** Holds information about the destination image.
 */
class DestPanoImage
{
public:
    enum Projection { RECTILINEAR = 0,
                      CYLINDRICAL = 1,
                      EQUIRECTANGULAR = 2,
                      FULL_FRAME_FISHEYE = 3};
 
    DestPanoImage()
    {
        m_proj = EQUIRECTANGULAR;
        m_hfov = 360;
        m_size = vigra::Size2D(360,180);
        m_roi = vigra::Rect2D(m_size);
    }

    DestPanoImage(Projection proj, double hfov, vigra::Size2D sz)
     : m_proj(proj), m_hfov(hfov), m_size(sz), m_roi(sz)
    {
    }
    bool horizontalWarpNeeded()
    {
        switch (m_proj)
        {
            case CYLINDRICAL:
            case EQUIRECTANGULAR:
                if (m_hfov == 360) return true;
            case FULL_FRAME_FISHEYE:
            case RECTILINEAR:
            default:
                break;
        }
        return false;
    }
    // data accessors
    const Projection & getProjection() const
    { return m_proj; }
    void setProjection(const Projection & val)
    { m_proj = val; }

    const double & getHFOV() const
    { return m_hfov; }
    void setHFOV(const double & val)
    { m_hfov = val; }

    const vigra::Size2D & getSize() const
    { return m_size; }
    void setSize(const vigra::Size2D & val)
    { m_size = val; }

    const vigra::Rect2D & getROI() const
    { return m_roi; }
    void setROI(const vigra::Rect2D & val)
    { m_roi = val; }

private:
    Projection m_proj;
    double m_hfov;
    vigra::Size2D m_size;
    vigra::Rect2D m_roi;
};

    /** This class holds an source image.
     *
     *  It contains information about its settings for the panorama.
     *
     *  An image should not depend on the panorama.
     */
    class PanoImage
    {
    public:
        PanoImage()
        {
            width=0;
            height=0;
            lensNr=0;
        };

        PanoImage(const std::string &filename,  int width,int height,
                  int lens);

//        PanoImage(const std::string & filename);
        // create from xml node
//        PanoImage(QDomNode & node);

        virtual ~PanoImage();

        virtual const char * isA() const { return "PanoImage"; };

        std::string getFilename() const
            { return filename; }

        void setFilename(std::string fn)
            { filename = fn; }

        const ImageOptions & getOptions() const
            { return options; }

        void setOptions(const ImageOptions & opt)
            { options = opt; }

        unsigned int getHeight() const
            { return height; }
        unsigned int getWidth() const
            { return width; }

        void setLensNr(unsigned int l)
            { lensNr = l; }
        unsigned int getLensNr() const
            { return lensNr; }

        void setSize(const vigra::Size2D & sz)
            { width =sz.x; height = sz.y; }

        void setFeatherWidth(unsigned int w)
            { options.featherWidth = w; }

    private:
        /// common init for all constructors
        void init();
        /// read image info (size, exif header)
        bool readImageInformation();

        // image properties needed by Panorama tools.

        std::string filename;
        int height,width;

        bool imageRead;
        ImageOptions options;
        // the lens of this image
        unsigned int lensNr;
    };

} // namespace

#endif // PANOIMAGE_H

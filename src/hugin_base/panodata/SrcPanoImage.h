// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
 *
 *  @brief 
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 * !! from PanoImage.h 1970
 *
 *  $Id: PanoImage.h 1970 2007-04-18 22:26:56Z dangelo $
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
#include <vigra/diff2d.hxx>

#include <hugin_utils/utils.h>
#include <hugin_math/hugin_math.h>

namespace HuginBase {

class Panorama;


/** description of a source pano image... 
 *
 *  In the long term, this simplified class will replace
 *  PanoImage and Image options and the variables array.
 */
class SrcPanoImage
{
    
public:
    ///
    enum Projection {
        RECTILINEAR = 0,
        PANORAMIC = 1,
        CIRCULAR_FISHEYE = 2,
        FULL_FRAME_FISHEYE = 3,
        EQUIRECTANGULAR = 4
    };
    
    ///
    enum CropMode {
        NO_CROP=0,
        CROP_RECTANGLE=1,
        CROP_CIRCLE=2
    };

    /// vignetting correction mode (bitflags, no real enum)
    enum VignettingCorrMode { 
        VIGCORR_NONE = 0,      ///< no vignetting correction
        VIGCORR_RADIAL = 1,    ///< radial vignetting correction
        VIGCORR_FLATFIELD = 2, ///< flatfield correction
        VIGCORR_DIV = 4        ///< correct by division.
    };

    ///
    enum ResponseType {
        RESPONSE_EMOR=0,                 ///< empirical model of response
        RESPONSE_LINEAR,                 ///< linear response
        RESPONSE_GAMMA,                  ///< a simple gamma response curve
        RESPONSE_FILE,                   ///< load response curve from file (not implemented yet)
        RESPONSE_ICC                     ///< use ICC for transformation into linear data (not implemented yet)
    };

    
public:
    ///
    SrcPanoImage()
    {
        setDefaults();
    }

    ///
    SrcPanoImage(const std::string &filename, vigra::Size2D size)
    {
        setDefaults();
        m_filename = filename;
        m_size = size ;
        m_cropRect = vigra::Rect2D(size);
    };

    ///
    bool operator==(const SrcPanoImage & other) const;

protected:
    ///
    virtual void setDefaults();
    

public:
    /** "resize" image,
     *  adjusts all distortion coefficients for usage with a source image
     *  of size @p size
     */
    void resize(const vigra::Size2D & size);

    /** check if a coordinate is inside the source image
     */
    bool isInside(vigra::Point2D p) const;

    ///
    bool horizontalWarpNeeded();

    
    // property accessors
public:
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

    bool getCorrectTCA() const;

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

    FDiff2D getRadialDistortionCenter() const;

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

    FDiff2D getRadialVigCorrCenter() const;

    int getLensNr() const
    { return m_lensNr; }
    
    void setLensNr(const int & val)
    { m_lensNr = val; }

    CropMode getCropMode() const
    { return m_crop; }
    
    void setCropMode(CropMode val);

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
    double getExposure() const;
    
    void setExposure(const double & val);

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
    
    
    
    /** try to fill out information about the image, by examining the exif data
    *  focalLength and cropFactor will be updated with the ones read from the exif data
    *  If no or not enought exif data was found and valid given focalLength and cropFactor
    *  settings where provided, they will be used for computation of the HFOV.
    */
    static bool initImageFromFile(SrcPanoImage & img, double & focalLength, double & cropFactor);
    
    /** calculate hfov of an image given focal length, image size and crop factor */
    static double calcHFOV(SrcPanoImage::Projection proj, double fl, double crop, vigra::Size2D imageSize);
    
    /** calcualte focal length, given crop factor and hfov */
    static double calcFocalLength(SrcPanoImage::Projection proj, double hfov, double crop, vigra::Size2D imageSize);
    
    

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




} // namespace

#endif // PANOIMAGE_H

// -*- c-basic-offset: 4 -*-

/** @file PanoImage.h
 *
 *  @brief 
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 * !! from PanoImage.h 1970
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

#include "SrcPanoImage.h"

#include <iostream>
#include <vector>
#include <vigra/diff2d.hxx>
#include <vigra/imageinfo.hxx>
#include <hugin_utils/utils.h>
#include <jhead/jhead.h>

#ifdef HUGIN_USE_EXIV2
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#endif

namespace HuginBase {

using namespace hugin_utils;
    
void SrcPanoImage::resize(const vigra::Size2D & sz)
{
        // TODO: check if images have the same orientation.
        // calculate scaling ratio
        double scale = (double) sz.x / m_size.x;
        
        // center shift
        m_centerShift *= scale;
        m_shear *= scale;
        
        // crop
        // ensure the scaled rectangle is inside the new image size
        switch (m_crop)
        {
            case NO_CROP:
                m_cropRect = vigra::Rect2D(sz);
                break;
            case CROP_RECTANGLE:
                m_cropRect = m_cropRect * scale;
                m_cropRect = m_cropRect & vigra::Rect2D(sz);
                break;
            case CROP_CIRCLE:
                m_cropRect = m_cropRect * scale;
                break;
        }
        
        m_size = sz;
        // vignetting correction
        m_radialVigCorrCenterShift *=scale;
}

bool SrcPanoImage::horizontalWarpNeeded()
{
    switch (m_proj)
    {
        case PANORAMIC:
        case EQUIRECTANGULAR:
            if (m_hfov == 360) return true;
        case FULL_FRAME_FISHEYE:
        case CIRCULAR_FISHEYE:
        case RECTILINEAR:
        default:
            break;
    }
    return false;
}

void SrcPanoImage::setDefaults()
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
    m_exifOrientation = 0;
    m_exifAperture = 0;
    m_exifDistance = 0;
    m_exifISO = 0;

    m_lensNr = 0;
    m_featherWidth = 10;
    m_morph = false;
}


bool SrcPanoImage::isInside(vigra::Point2D p) const
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

    
bool SrcPanoImage::getCorrectTCA() const
{ 
    bool nr = (m_radialDistRed[0] == 0.0 && m_radialDistRed[1] == 0.0 &&
               m_radialDistRed[2] == 0.0 && m_radialDistRed[3] == 1);
    bool nb = (m_radialDistBlue[0] == 0.0 && m_radialDistBlue[1] == 0.0 &&
               m_radialDistBlue[2] == 0.0 && m_radialDistBlue[3] == 1);
    return !(nr && nb);
}


FDiff2D SrcPanoImage::getRadialDistortionCenter() const
{ return FDiff2D(m_size)/2.0 + m_centerShift; }


FDiff2D SrcPanoImage::getRadialVigCorrCenter() const
{ return (FDiff2D(m_size)-FDiff2D(1,1))/2.0 + m_radialVigCorrCenterShift; }

void SrcPanoImage::setCropMode(CropMode val)
{
    m_crop = val;
    if (m_crop == NO_CROP) {
        m_cropRect = vigra::Rect2D(m_size);
    }
}

double SrcPanoImage::getExposure() const
{ return 1.0/pow(2.0, m_exposure); }

void SrcPanoImage::setExposure(const double & val)
{ m_exposure = log2(1/val); }


bool SrcPanoImage::operator==(const SrcPanoImage & other) const
{
    //        return true;
    return ( m_proj == other.m_proj &&
             m_hfov == other.m_hfov &&
             m_roll  == other.m_roll  &&
             m_pitch == other.m_pitch  &&
             m_yaw == other.m_yaw &&
             
             m_responseType == other.m_responseType &&
             m_emorParams == other.m_emorParams &&
             m_exposure == other.m_exposure &&
             m_gamma == m_gamma &&
             m_wbRed == m_wbRed &&
             m_wbBlue == m_wbBlue &&
             
             m_radialDist == other.m_radialDist  &&
             m_radialDistRed == other.m_radialDistRed  &&
             m_radialDistBlue == other.m_radialDistBlue  &&
             m_centerShift == other.m_centerShift  &&
             m_shear == other.m_shear  &&
             
             m_crop == other.m_crop  &&
             m_cropRect == other.m_cropRect &&
             
             m_vigCorrMode == other.m_vigCorrMode  &&
             m_radialVigCorrCoeff == other.m_radialVigCorrCoeff &&
             
             m_ka == other.m_ka  &&
             m_kb == other.m_kb  &&
             
             m_exifModel == other.m_exifModel &&
             m_exifMake == other.m_exifMake &&
             m_exifCropFactor == other.m_exifCropFactor &&
             m_exifFocalLength == other.m_exifFocalLength &&
             m_exifOrientation == other.m_exifOrientation &&
             m_exifAperture == other.m_exifAperture &&
             m_exifISO == other.m_exifISO &&
             m_exifDistance == other.m_exifDistance &&

             m_lensNr == other.m_lensNr  &&
             m_featherWidth == other.m_featherWidth  &&
             m_morph == other.m_morph);
}

// convinience functions to extract a set of variables
double SrcPanoImage::getVar(const std::string & name) const
{
    assert(name.size() > 0);
    // TODO: support all variables
    if (name == "Eev") 
        return m_exposure;
    else if (name == "Er")
        return m_wbRed;
    else if (name == "Eb")
        return m_wbBlue;
    else if (name == "Ra")
        return m_emorParams[0];
    else if (name[0] == 'R')
    {
        assert(name.size() == 2);
        int i = name[1] - 'a';
        return m_emorParams[i];
    } else if (name[0] == 'V')
    {
        int i = name[1] - 'a';
        if (i > 0 && i < 4) {
            return m_radialVigCorrCoeff[i];
        } else {
            if (name[1] == 'x') {
                return m_radialVigCorrCenterShift.x;
            } else if (name[1] == 'y') {
                return m_radialVigCorrCenterShift.y;
            }
        }
    } else {
        assert(0 || "Unknown variable in getVar()");
    }
    return 0;
}

void SrcPanoImage::setVar(const std::string & name, double val)
{
    assert(name.size() > 0);
    // TODO: support all variables
    if (name == "Eev") 
        m_exposure = val;
    else if (name == "Er")
        m_wbRed = val;
    else if (name == "Eb")
        m_wbBlue = val;
    else if (name[0] == 'R')
    {
        assert(name.size() == 2);
        int i = name[1] - 'a';
        m_emorParams[i] = val;
    } else if (name[0] == 'V')
    {
        int i = name[1] - 'a';
        if (i >= 0 && i < 4) {
            m_radialVigCorrCoeff[i] = val;
        } else {
            if (name[1] == 'x') {
                m_radialVigCorrCenterShift.x = val;
            } else if (name[1] == 'y') {
                m_radialVigCorrCenterShift.y = val;
            } else {
                DEBUG_ERROR("Unknown variable " << name);
            }
        }
    } else {
        DEBUG_ERROR("Unknown variable " << name);
    }
}


bool SrcPanoImage::readEXIF(double & focalLength, double & cropFactor, bool applyEXIFValues)
{
    std::string filename = getFilename();
    std::string ext = hugin_utils::getExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int)) toupper);

    double roll = 0;
    double eV = 0;
    float isoSpeed = 0;
    float photoFNumber = 0;
    float exposureTime = 0;
    float subjectDistance = 0;

    int width;
    int height;
    try {
        vigra::ImageImportInfo info(filename.c_str());
        width = info.width();
        height = info.height();
    } catch(vigra::PreconditionViolation & ) {
        return false;
    }

    // Setup image with default values
    setSize(vigra::Size2D(width, height));
    if (applyEXIFValues && focalLength > 0 && cropFactor > 0) {
        setHFOV(calcHFOV(getProjection(),
        focalLength, cropFactor, getSize()));
    }

    #ifdef HUGIN_USE_EXIV2 

    Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(filename.c_str());
    if (image.get() == 0) {
        std::cout << "Unable to open file to read EXIF data: " << filename << std::endl;
        return false;
    }

    image->readMetadata();
    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        std::cout << "Unable to read EXIF data from opened file:" << filename << std::endl;
        return false;
    }

    getExiv2Value(exifData,"Exif.Photo.ExposureTime",exposureTime);
    // TODO: reconstruct real exposure value from "rounded" ones saved by the cameras?

    getExiv2Value(exifData,"Exif.Photo.FNumber",photoFNumber);

    if (exposureTime > 0 && photoFNumber > 0) {
        double gain = 1;
        if (getExiv2Value(exifData,"Exif.Photo.ISOSpeedRatings",isoSpeed)) {
            if (isoSpeed > 0) {
                gain = isoSpeed / 100.0;
            }
        }
        eV = log2(photoFNumber * photoFNumber / (gain * exposureTime));
        DEBUG_DEBUG ("Ev: " << eV);
    }

    Exiv2::ExifKey key("Exif.Image.Make");
    Exiv2::ExifData::iterator itr = exifData.findKey(key);
    if (itr != exifData.end())
        setExifMake(itr->toString());
    else
        setExifMake("Unknown");

    Exiv2::ExifKey key2("Exif.Image.Model");
    itr = exifData.findKey(key2);
    if (itr != exifData.end())
        setExifModel(itr->toString());
    else
        setExifModel("Unknown");

    long orientation = 0;
    if (getExiv2Value(exifData,"Exif.Image.Orientation",orientation)) {
        switch (orientation) {
            case 3:  // rotate 180
                roll = 180;
                break;
            case 6: // rotate 90
                roll = 90;
                break;
            case 8: // rotate 270
                roll = 270;
                break;
            default:
                break;
        }
    }

    long pixXdim = 0;
    getExiv2Value(exifData,"Exif.Photo.PixelXDimension",pixXdim);

    long pixYdim = 0;
    getExiv2Value(exifData,"Exif.Photo.PixelYDimension",pixYdim);

    if (pixXdim !=0 && pixYdim !=0 ) {
        double ratioExif = pixXdim/(double)pixYdim;
        double ratioImage = width/(double)height;
        if (fabs( ratioExif - ratioImage) > 0.1) {
            // Image has been modified without adjusting exif tags.
            // Assume user has rotated to upright pose
            roll = 0;
        }
    }
    
    //GWP - CCD info was previously computed by the jhead library.  Migration
    //      to exiv2 means we do it here
    long eWidth = 0;
    getExiv2Value(exifData,"Exif.Image.ImageWidth",eWidth);

    long eLength = 0;
    getExiv2Value(exifData,"Exif.Image.ImageLength",eLength);

    double sensorPixelWidth = 0;
    double sensorPixelHeight = 0;
    if (eWidth > 0 && eLength > 0) {
        sensorPixelHeight = (double)eLength;
        sensorPixelWidth = (double)eWidth;
    } else {
        // No EXIF information, use number of pixels in image
        sensorPixelWidth = width;
        sensorPixelHeight = height;
    }

    // force landscape sensor orientation
    if (sensorPixelWidth < sensorPixelHeight ) {
        double t = sensorPixelWidth;
        sensorPixelWidth = sensorPixelHeight;
        sensorPixelHeight = t;
    }

    DEBUG_DEBUG("sensorPixelWidth: " << sensorPixelWidth);
    DEBUG_DEBUG("sensorPixelHeight: " << sensorPixelHeight);

    long exifResolutionUnits = 0;
    getExiv2Value(exifData,"Exif.Photo.FocalPlaneResolutionUnit",exifResolutionUnits);

    float resolutionUnits= 0;
    switch (exifResolutionUnits) {
        case 3: resolutionUnits = 10.0; break;  //centimeter
        case 4: resolutionUnits = 1.0; break;   //millimeter
        case 5: resolutionUnits = .001; break;  //micrometer
        default: resolutionUnits = 25.4; break; //inches
    }

    DEBUG_DEBUG("Resolution Units: " << resolutionUnits);

    float fplaneXresolution = 0;
    getExiv2Value(exifData,"Exif.Photo.FocalPlaneXResolution",fplaneXresolution);

    float fplaneYresolution = 0;
    getExiv2Value(exifData,"Exif.Photo.FocalPlaneYResolution",fplaneYresolution);

    float CCDWidth = 0;
    if (fplaneXresolution != 0) { 
        CCDWidth = (float)(sensorPixelWidth * resolutionUnits / 
                fplaneXresolution);
    }

    float CCDHeight = 0;
    if (fplaneYresolution != 0) {
        CCDHeight = (float)(sensorPixelHeight * resolutionUnits /
              fplaneYresolution) ;
    }

    DEBUG_DEBUG("CCDHeight:" << CCDHeight);
    DEBUG_DEBUG("CCDWidth: " << CCDWidth);

    // calc sensor dimensions if not set and 35mm focal length is available
    FDiff2D sensorSize;

    if (CCDHeight > 0 && CCDWidth > 0) {
        // read sensor size directly.
        sensorSize.x = CCDWidth;
        sensorSize.y = CCDHeight;
        if (getExifModel() == "Canon EOS 20D") {
            // special case for buggy 20D camera
            sensorSize.x = 22.5;
            sensorSize.y = 15;
        }
        //
        // check if sensor size ratio and image size fit together
        double rsensor = (double)sensorSize.x / sensorSize.y;
        double rimg = (double) width / height;
        if ( (rsensor > 1 && rimg < 1) || (rsensor < 1 && rimg > 1) ) {
            // image and sensor ratio do not match
            // swap sensor sizes
            float t;
            t = sensorSize.y;
            sensorSize.y = sensorSize.x;
            sensorSize.x = t;
        }

        DEBUG_DEBUG("sensorSize.y: " << sensorSize.y);
        DEBUG_DEBUG("sensorSize.x: " << sensorSize.x);

        cropFactor = sqrt(36.0*36.0+24.0*24.0) /
            sqrt(sensorSize.x*sensorSize.x + sensorSize.y*sensorSize.y);
    }
    DEBUG_DEBUG("cropFactor: " << cropFactor);

    float eFocalLength = 0;
    getExiv2Value(exifData,"Exif.Photo.FocalLength",eFocalLength);

    float eFocalLength35 = 0;
    getExiv2Value(exifData,"Exif.Photo.FocalLengthIn35mmFilm",eFocalLength35);

    //The various methods to detmine crop factor
    if (eFocalLength > 0 && cropFactor > 0) {
        // user provided crop factor
        focalLength = eFocalLength;
    } else if (eFocalLength35 > 0 && eFocalLength > 0) {
        cropFactor = eFocalLength35 / eFocalLength;
        focalLength = eFocalLength;
    } else if (eFocalLength35 > 0) {
        // 35 mm equiv focal length available, crop factor unknown.
        // do not ask for crop factor, assume 1.  Probably a full frame sensor
        cropFactor = 1;
        focalLength = eFocalLength35;
    } else if (eFocalLength > 0 && cropFactor <= 0) {
        // need to redo, this time with crop
        focalLength = eFocalLength;
        cropFactor = 0;
    }
    getExiv2Value(exifData,"Exif.Photo.SubjectDistance", subjectDistance);

#else
    if (ext == "JPG" || ext == "JPEG") {
        
        ImageInfo_t exif;
        ResetJpgfile();
        // Start with an empty image information structure.
        
        memset(&exif, 0, sizeof(exif));
        exif.FlashUsed = -1;
        exif.MeteringMode = -1;
        if (ReadJpegFile(exif,filename.c_str(), READ_EXIF)){
#ifdef DEBUG
            ShowImageInfo(exif);
#endif
            std::cout << "exp time: " << exif.ExposureTime  << " f-stop: " <<  exif.ApertureFNumber << std::endl;
            // calculate exposure from exif image
            exposureTime = exif.ExposureTime;
            photoFNumber = exif.ApertureFNumber;
            if (exif.ExposureTime > 0 && exif.ApertureFNumber > 0) {
                double gain = 1;
                isoSpeed = exif.ISOequivalent;
                if (exif.ISOequivalent > 0)
                    gain = exif.ISOequivalent/ 100.0;
                eV = log2(exif.ApertureFNumber*exif.ApertureFNumber/(gain * exif.ExposureTime));
            }

            setExifMake(exif.CameraMake);
            setExifModel(exif.CameraModel);
            DEBUG_DEBUG("exif dimensions: " << exif.ExifImageWidth << "x" << exif.ExifImageWidth);
            switch (exif.Orientation) {
                case 3:  // rotate 180
                    roll = 180;
                    break;
                case 6: // rotate 90
                    roll = 90;
                    break;
                case 8: // rotate 270
                    roll = 270;
                    break;
                default:
                    break;
            }
            // image has been modified without adjusting exif tags
            // assume user has rotated to upright pose
            if (exif.ExifImageWidth && exif.ExifImageLength) {
                double ratioExif = exif.ExifImageWidth / (double)exif.ExifImageLength;
                double ratioImage = width/(double)height;
                if (fabs( ratioExif - ratioImage) > 0.1) {
                    roll = 0;
                }
            }
            
            // calc sensor dimensions if not set and 35mm focal length is available
            FDiff2D sensorSize;
            
             std::cout << "exif.CCDHeight " << exif.CCDHeight << " exif.CCDWidth " << exif.CCDWidth << "\n";
            if (exif.CCDHeight > 0 && exif.CCDWidth > 0) {
                // read sensor size directly.
                sensorSize.x = exif.CCDWidth;
                sensorSize.y = exif.CCDHeight;
                if (strcmp(exif.CameraModel, "Canon EOS 20D") == 0) {
                    // special case for buggy 20D camera
                    sensorSize.x = 22.5;
                    sensorSize.y = 15;
                }
                //
                // check if sensor size ratio and image size fit together
                double rsensor = (double)sensorSize.x / sensorSize.y;
                double rimg = (double) width / height;
                if ( (rsensor > 1 && rimg < 1) || (rsensor < 1 && rimg > 1) ) {
                    // image and sensor ratio do not match
                    // swap sensor sizes
                    float t;
                    t = sensorSize.y;
                    sensorSize.y = sensorSize.x;
                    sensorSize.x = t;
                }
                std::cout << "sensorSize.y " << sensorSize.y << " sensorSize.x " << sensorSize.x << "\n";
                cropFactor = sqrt(36.0*36.0+24.0*24)/sqrt(sensorSize.x*sensorSize.x + sensorSize.y*sensorSize.y);
            }
            
            if (exif.FocalLength > 0 && cropFactor > 0) {
                // user provided crop factor
                focalLength = exif.FocalLength;
            } else if (exif.FocalLength35mm > 0 && exif.FocalLength > 0) {
                cropFactor = exif.FocalLength35mm / exif.FocalLength;
                focalLength = exif.FocalLength;
            } else if (exif.FocalLength35mm > 0) {
                // 35 mm equiv focal length available, crop factor unknown.
                // do not ask for crop factor, assume 1.
                cropFactor = 1;
                focalLength = exif.FocalLength35mm;
            } else if (exif.FocalLength > 0 && cropFactor <= 0) {
                // need to redo, this time with crop
                focalLength = exif.FocalLength;
                cropFactor = 0;
            }
        }
        subjectDistance = exif.Distance;
    }
#endif

    // store some important EXIF tags for later usage.
    setExifFocalLength(focalLength);
    setExifCropFactor(cropFactor);
    setExifOrientation(roll);
    setExifAperture(photoFNumber);
    setExifISO(isoSpeed);
    setExifDistance(subjectDistance);

    DEBUG_DEBUG("Results for:" << filename);
    DEBUG_DEBUG("Focal Length: " << getExifFocalLength());
    DEBUG_DEBUG("Crop Factor:  " << getExifCropFactor());
    DEBUG_DEBUG("Roll:         " << getExifOrientation());

    // Update image with computed values from EXIF
    if (applyEXIFValues) {
        setRoll(roll);
        setExposureValue(eV);
        if (focalLength > 0 && cropFactor > 0) {
            setHFOV(calcHFOV(getProjection(), focalLength, cropFactor, getSize()));
            DEBUG_DEBUG("HFOV:         " << getHFOV());
            return true;
        } else {
            return false;
        }
    }
    return true;
}

double SrcPanoImage::calcHFOV(SrcPanoImage::Projection proj, double fl, double crop, vigra::Size2D imageSize)
{
    // calculate diagonal of film
    double d = sqrt(36.0*36.0 + 24.0*24.0) / crop;
    double r = (double)imageSize.x / imageSize.y;
    
    // calculate the sensor width and height that fit the ratio
    // the ratio is determined by the size of our image.
    FDiff2D sensorSize;
    sensorSize.x = d / sqrt(1 + 1/(r*r));
    sensorSize.y = sensorSize.x / r;
    
    double hfov = 360;
    
    switch (proj) {
        case SrcPanoImage::RECTILINEAR:
            hfov = 2*atan((sensorSize.x/2.0)/fl)  * 180.0/M_PI;
            break;
        case SrcPanoImage::CIRCULAR_FISHEYE:
        case SrcPanoImage::FULL_FRAME_FISHEYE:
            hfov = sensorSize.x / fl * 180/M_PI;
            break;
        case SrcPanoImage::EQUIRECTANGULAR:
        case SrcPanoImage::PANORAMIC:
            hfov = (sensorSize.x / fl) / M_PI * 180;
            break;
        default:
            hfov = 360;
            // TODO: add formulas for other projections
            DEBUG_WARN("Focal length calculations only supported with rectilinear and fisheye images");
    }
    return hfov;
}

double SrcPanoImage::calcFocalLength(SrcPanoImage::Projection proj, double hfov, double crop, vigra::Size2D imageSize)
{
    // calculate diagonal of film
    double d = sqrt(36.0*36.0 + 24.0*24.0) / crop;
    double r = (double)imageSize.x / imageSize.y;
    
    // calculate the sensor width and height that fit the ratio
    // the ratio is determined by the size of our image.
    FDiff2D sensorSize;
    sensorSize.x = d / sqrt(1 + 1/(r*r));
    sensorSize.y = sensorSize.x / r;
    
    switch (proj)
    {
        case SrcPanoImage::RECTILINEAR:
            return (sensorSize.x/2.0) / tan(hfov/180.0*M_PI/2);
            break;
        case SrcPanoImage::CIRCULAR_FISHEYE:
        case SrcPanoImage::FULL_FRAME_FISHEYE:
            // same projection equation for both fisheye types,
            // assume equal area projection.
            return sensorSize.x / (hfov/180*M_PI);
            break;
        case SrcPanoImage::EQUIRECTANGULAR:
        case SrcPanoImage::PANORAMIC:
            return  (sensorSize.x / (hfov/180*M_PI));
            break;
        default:
            // TODO: add formulas for other projections
            DEBUG_WARN("Focal length calculations only supported with rectilinear and fisheye images");
            return 0;
    }
}

double SrcPanoImage::calcCropFactor(SrcPanoImage::Projection proj, double hfov, double focalLength, vigra::Size2D imageSize)
{
    // calculate diagonal of film
    double r = (double)imageSize.x / imageSize.y;

    double x = 36;
    switch (proj)
    {
        case SrcPanoImage::RECTILINEAR:
            x = focalLength * tan(hfov/180.0*M_PI/2);
            break;
        case SrcPanoImage::CIRCULAR_FISHEYE:
        case SrcPanoImage::FULL_FRAME_FISHEYE:
        case SrcPanoImage::EQUIRECTANGULAR:
        case SrcPanoImage::PANORAMIC:
            // same projection equation for both fisheye types,
            // assume equal area projection.
            x = focalLength * (hfov/180*M_PI);
            break;
        default:
            // TODO: add formulas for other projections
            DEBUG_WARN("Focal length calculations only supported with rectilinear and fisheye images");
            return 0;
    }
    // diagonal of sensor
    double diag = x * sqrt(1+ 1/(r*r));
    return sqrt(36.0*36.0 + 24.0*24.0) / diag;
}


#ifdef HUGIN_USE_EXIV2
// Convenience functions to work with Exiv2
bool SrcPanoImage::getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value)
{
    Exiv2::ExifKey key(keyName);
    Exiv2::ExifData::iterator itr = exifData.findKey(key);
    if (itr != exifData.end()) {
        value = itr->toLong();
        DEBUG_DEBUG("" << keyName << ": " << value);
        return true;
    } else {
        return false;
    }
}


bool SrcPanoImage::getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, float & value)
{
    Exiv2::ExifKey key(keyName);
    Exiv2::ExifData::iterator itr = exifData.findKey(key);
    if (itr != exifData.end()) {
        value = itr->toFloat();
        DEBUG_DEBUG("" << keyName << ": " << value);
        return true;
    } else {
        return false;
    }
}
#endif // HUGIN_USE_EXIV2
    
} // namespace

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

#include "SrcPanoImage.h"

#include <iostream>
#include <vector>
#include <vigra/diff2d.hxx>
#include <vigra/imageinfo.hxx>
#include <hugin_utils/utils.h>
#include <jhead/jhead.h>

namespace HuginBase {

    
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




bool SrcPanoImage::initImageFromFile(SrcPanoImage & img, double & focalLength, double & cropFactor)
{
    std::string filename = img.getFilename();
    std::string ext = hugin_utils::getExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int)) toupper);
    
    double roll = 0;
    int width;
    int height;
    try {
        vigra::ImageImportInfo info(filename.c_str());
        width = info.width();
        height = info.height();
    } catch(vigra::PreconditionViolation & ) {
        return false;
    }
    img.setSize(vigra::Size2D(width, height));
    
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
            if (exif.ExposureTime > 0 && exif.ApertureFNumber > 0) {
                double gain = 1;
                if (exif.ISOequivalent > 0)
                    gain = exif.ISOequivalent/ 100.0;
                img.setExposureValue(log2(exif.ApertureFNumber*exif.ApertureFNumber/(gain * exif.ExposureTime)));
            }
            
            img.setExifMake(exif.CameraMake);
            img.setExifModel(exif.CameraModel);
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
    }
    
    img.setExifFocalLength(focalLength);
    img.setExifCropFactor(cropFactor);
    img.setRoll(roll);
    
    
    
    if (focalLength > 0 && cropFactor > 0) {
        img.setHFOV(calcHFOV(img.getProjection(), focalLength, cropFactor, img.getSize()));
        return true;
    } else {
        return false;
    }
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

    
} // namespace

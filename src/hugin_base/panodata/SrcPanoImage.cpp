// -*- c-basic-offset: 4 -*-

/** @file SrcPanoImage.h
 *
 *  @brief 
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *          James Legg
 *
 * !! from PanoImage.h 1970
 *
 */
/*
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

// for debugging
#include <iostream>
#include <stdio.h>
#include <stdexcept>
//#include <wx/wxprec.h>

#include "SrcPanoImage.h"

#include <iostream>
#include <vector>
#include <vigra/diff2d.hxx>
#include <vigra/imageinfo.hxx>
#include <hugin_utils/utils.h>
#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>
#include <exiv2/easyaccess.hpp>
#include <lensdb/LensDB.h>
#include "Exiv2Helper.h"

#ifdef __FreeBSD__
#define log2(x)        (log(x) / M_LN2)
#endif /* __FreeBSD__ */

#include "ImageVariableTranslate.h"

namespace HuginBase {

using namespace hugin_utils;
    
void SrcPanoImage::resize(const vigra::Size2D & sz)
{
        // TODO: check if images have the same orientation.
        // calculate scaling ratio
        double scale = (double) sz.x / m_Size.getData().x;
        
        // center shift
        m_RadialDistortionCenterShift.setData(m_RadialDistortionCenterShift.getData() * scale);
        m_Shear.setData(m_Shear.getData() * scale);
        
        // crop
        // ensure the scaled rectangle is inside the new image size
        switch (m_CropMode.getData())
        {
            case NO_CROP:
                m_CropRect.setData(vigra::Rect2D(sz));
                break;
            case CROP_RECTANGLE:
                m_CropRect.setData(m_CropRect.getData() * scale);
                m_CropRect.setData(m_CropRect.getData() & vigra::Rect2D(sz));
                break;
            case CROP_CIRCLE:
                m_CropRect.setData(m_CropRect.getData() * scale);
                break;
        }
        
        m_Size = sz;
        // vignetting correction
        m_RadialVigCorrCenterShift.setData(m_RadialVigCorrCenterShift.getData() *scale);
        // resize masks
        MaskPolygonVector scaledMasks=m_Masks.getData();
        for(unsigned int i=0;i<scaledMasks.size();i++)
            scaledMasks[i].scale(scale);
        m_Masks.setData(scaledMasks);
        scaledMasks.clear();
        scaledMasks=m_ActiveMasks.getData();
        for(unsigned int i=0;i<scaledMasks.size();i++)
            scaledMasks[i].scale(scale);
        m_ActiveMasks.setData(scaledMasks);
}

bool SrcPanoImage::horizontalWarpNeeded()
{
    switch (m_Projection.getData())
    {
        case PANORAMIC:
        case EQUIRECTANGULAR:
            if (m_HFOV.getData() == 360) return true;
        case FULL_FRAME_FISHEYE:
        case CIRCULAR_FISHEYE:
        case RECTILINEAR:
        case FISHEYE_ORTHOGRAPHIC:
        case FISHEYE_STEREOGRAPHIC:
        case FISHEYE_EQUISOLID:
        case FISHEYE_THOBY:
        default:
            break;
    }
    return false;
}

void BaseSrcPanoImage::setDefaults()
{
    /* Some of the vectors are difficult to initalise with the variables list
     * header, so we make some local variables which are used in it.
     */
    // Radial Distortion defaults
    std::vector<double> distortion_default(4, 0.0);
    distortion_default[3] = 1;
    
    std::vector<double> RadialVigCorrCoeff_default(4, 0.0);
    RadialVigCorrCoeff_default[0] = 1;
    HuginBase::MaskPolygonVector defaultMaskVector;
#define image_variable( name, type, default_value ) m_##name.setData(default_value);
#include "image_variables.h"
#undef image_variable
}

bool SrcPanoImage::isInside(vigra::Point2D p, bool ignoreMasks) const
{
    bool insideCrop=false;
    switch(m_CropMode.getData()) {
        case NO_CROP:
        case CROP_RECTANGLE:
            insideCrop = m_CropRect.getData().contains(p);
            break;
        case CROP_CIRCLE:
        {
            if (0 > p.x || 0 > p.y || p.x >= m_Size.getData().x || p.y >= m_Size.getData().y) {
                // outside image
                return false;
            }
            FDiff2D cropCenter;
            cropCenter.x = m_CropRect.getData().left() + m_CropRect.getData().width()/2.0;
            cropCenter.y = m_CropRect.getData().top() + m_CropRect.getData().height()/2.0;
            double radius2 = std::min(m_CropRect.getData().width()/2.0, m_CropRect.getData().height()/2.0);
            radius2 = radius2 * radius2;
            FDiff2D pf = FDiff2D(p) - cropCenter;
            insideCrop = (radius2 > pf.x*pf.x+pf.y*pf.y );
        }
    }
    if(insideCrop && !ignoreMasks)
        return !(isInsideMasks(p));
    else
        return insideCrop;
}

bool SrcPanoImage::isCircularCrop() const
{
    HuginBase::BaseSrcPanoImage::Projection projection=m_Projection.getData();
    return (projection==CIRCULAR_FISHEYE || projection==FISHEYE_THOBY || projection==FISHEYE_ORTHOGRAPHIC);
};

bool SrcPanoImage::getCorrectTCA() const
{ 
    bool nr = (m_RadialDistortionRed.getData()[0] == 0.0 && m_RadialDistortionRed.getData()[1] == 0.0 &&
               m_RadialDistortionRed.getData()[2] == 0.0 && m_RadialDistortionRed.getData()[3] == 1);
    bool nb = (m_RadialDistortionBlue.getData()[0] == 0.0 && m_RadialDistortionBlue.getData()[1] == 0.0 &&
               m_RadialDistortionBlue.getData()[2] == 0.0 && m_RadialDistortionBlue.getData()[3] == 1);
    return !(nr && nb);
}


FDiff2D SrcPanoImage::getRadialDistortionCenter() const
{ return FDiff2D(m_Size.getData())/2.0 + m_RadialDistortionCenterShift.getData(); }


FDiff2D SrcPanoImage::getRadialVigCorrCenter() const
{ return (FDiff2D(m_Size.getData())-FDiff2D(1,1))/2.0 + m_RadialVigCorrCenterShift.getData(); }

void SrcPanoImage::setCropMode(CropMode val)
{
    m_CropMode.setData(val);
    if (val == NO_CROP) {
        m_CropRect.setData(vigra::Rect2D(m_Size.getData()));
    }
}

void SrcPanoImage::setSize(vigra::Size2D val)
{
    m_Size.setData(val);
    if (m_CropMode.getData() == NO_CROP) {
        m_CropRect.setData(vigra::Rect2D(val));
    }
}

double SrcPanoImage::getExposure() const
{ return 1.0/pow(2.0, m_ExposureValue.getData()); }

void SrcPanoImage::setExposure(const double & val)
{ m_ExposureValue = log2(1/val); }


bool BaseSrcPanoImage::operator==(const BaseSrcPanoImage & other) const
{
    DEBUG_TRACE("");
    return (
#define image_variable( name, type, default_value ) \
    m_##name.getData() == other.m_##name.getData() &&
#include "image_variables.h"
#undef image_variable
    true // All the variable checks above end with && so we need this.
    );
}

// convinience functions to extract a set of variables
double SrcPanoImage::getVar(const std::string & code) const
{
    DEBUG_TRACE("");
    assert(code.size() > 0);
#define image_variable( name, type, default_value ) \
    if (PTOVariableConverterFor##name::checkApplicability(code)) \
        return PTOVariableConverterFor##name::getValueFromVariable(code, m_##name );\
    else 
#include "image_variables.h"
#undef image_variable
    {// this is for the final else.
        DEBUG_ERROR("Unknown variable " << code);
    }
    return 0;
}

void SrcPanoImage::setVar(const std::string & code, double val)
{
    DEBUG_TRACE("Var:" << code << " value: " << val);
    assert(code.size() > 0);
#define image_variable( name, type, default_value ) \
    if (PTOVariableConverterFor##name::checkApplicability(code)) \
        {PTOVariableConverterFor##name::setValueFromVariable(code, m_##name, val);}\
    else 
#include "image_variables.h"
#undef image_variable
    {// this is for the final else.
        DEBUG_ERROR("Unknown variable " << code);
    }
}

VariableMap SrcPanoImage::getVariableMap() const
{
    // make a variable map vector
    
    // fill variable map with details about this image.
    // position
    DEBUG_TRACE("");

    VariableMap vars;
#define image_variable( name, type, default_value ) \
    PTOVariableConverterFor##name::addToVariableMap(m_##name, vars);
#include "image_variables.h"
#undef image_variable

    return vars;
}

bool SrcPanoImage::checkImageSizeKnown()
{
    if(getWidth()==0 || getHeight()==0)
    {
        try
        {
            vigra::ImageImportInfo info(getFilename().c_str());
            setSize(info.size());
        }
        catch(std::exception & )
        {
            return false;
        }
    };
    return true;

};

bool SrcPanoImage::readEXIF()
{
    std::string filename = getFilename();
    double roll = 0;
    if(!checkImageSizeKnown())
    {
        return false;
    };

    // if width==2*height assume equirectangular image
    if (getWidth() == 2 * getHeight())
    {
        FileMetaData metaData = getFileMetadata();
        metaData["projection"] = "equirectangular";
        metaData["HFOV"] = "360";
        setFileMetadata(metaData);
    };

    Exiv2::Image::AutoPtr image;
    try {
        image = Exiv2::ImageFactory::open(filename.c_str());
    }catch(...) {
        std::cerr << __FILE__ << " " << __LINE__ << " Error opening file" << std::endl;
        return false;
    }
    if (image.get() == 0) {
        std::cerr << "Unable to open file to read EXIF data: " << filename << std::endl;
        return false;
    }

    image->readMetadata();

    // look into XMP metadata
    Exiv2::XmpData& xmpData = image->xmpData();
    if (!xmpData.empty())
    {
        // we need to catch exceptions in case file does not contain any GPano tags
        try
        {
            Exiv2::XmpData::iterator pos = xmpData.findKey(Exiv2::XmpKey("Xmp.GPano.ProjectionType"));
            FileMetaData metaData = getFileMetadata();
            if (pos != xmpData.end())
            {
                if (hugin_utils::tolower(pos->toString()) == "equirectangular")
                {
                    long croppedWidth = 0;
                    long croppedHeight = 0;
                    pos = xmpData.findKey(Exiv2::XmpKey("Xmp.GPano.CroppedAreaImageWidthPixels"));
                    if (pos != xmpData.end())
                    {
                        croppedWidth = pos->toLong();
                    }
                    else
                    {
                        // tag is required
                        throw std::logic_error("Required tag CroppedAreaImageWidthPixels missing");
                    };
                    pos = xmpData.findKey(Exiv2::XmpKey("Xmp.GPano.CroppedAreaImageHeightPixels"));
                    if (pos != xmpData.end())
                    {
                        croppedHeight = pos->toLong();
                    }
                    else
                    {
                        // tag is required
                        throw std::logic_error("Required tag CroppedAreaImageHeightPixels missing");
                    };
                    // check if sizes matches, if not ignore all tags
                    if (getWidth() == croppedWidth && getHeight() == croppedHeight)
                    {
                        pos = xmpData.findKey(Exiv2::XmpKey("Xmp.GPano.FullPanoWidthPixels"));
                        double hfov = 0;
                        if (pos != xmpData.end())
                        {
                            hfov = 360 * croppedWidth / (double)pos->toLong();
                        }
                        else
                        {
                            // tag is required
                            throw std::logic_error("Required tag FullPanoWidthPixels missing");
                        };
                        long fullHeight = 0;
                        pos = xmpData.findKey(Exiv2::XmpKey("Xmp.GPano.FullPanoHeightPixels"));
                        if (pos != xmpData.end())
                        {
                            fullHeight = pos->toLong();
                        }
                        else
                        {
                            // tag is required
                            throw std::logic_error("Required tag FullPanoHeightPixels missing");
                        };
                        long cropTop = 0;
                        pos = xmpData.findKey(Exiv2::XmpKey("Xmp.GPano.CroppedAreaTopPixels"));
                        if (pos != xmpData.end())
                        {
                            cropTop = pos->toLong();
                        }
                        else
                        {
                            // tag is required
                            throw std::logic_error("Required tag CroppedAreaTopPixels missing");
                        };

                        // all found, remember for later
                        metaData["projection"] = "equirectangular";
                        metaData["HFOV"] = hugin_utils::doubleToString(hfov, 3);
                        metaData["e"] = hugin_utils::doubleToString(-cropTop - ((getHeight() - fullHeight) / 2.0), 4);
                        setFileMetadata(metaData);
                    };
                };
            };
        }
        catch (std::exception e)
        {
            // just to catch error when image contains no GPano tags
            std::cerr << "Error reading GPano tags from " << filename << "(" << e.what() << ")" << std::endl;
        };
    };

    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        std::cerr << "Unable to read EXIF data from opened file:" << filename << std::endl;
        return !getFileMetadata().empty();
    }

    setExifExposureTime(Exiv2Helper::getExiv2ValueDouble(exifData, Exiv2::exposureTime(exifData)));
    setExifAperture(Exiv2Helper::getExiv2ValueDouble(exifData, Exiv2::fNumber(exifData)));
    
    //read exposure mode
    setExifExposureMode(Exiv2Helper::getExiv2ValueLong(exifData, "Exif.Photo.ExposureMode"));

    // read ISO from EXIF or makernotes
    setExifISO(Exiv2Helper::getExiv2ValueDouble(exifData, Exiv2::isoSpeed(exifData)));

    setExifMake(Exiv2Helper::getExiv2ValueString(exifData, Exiv2::make(exifData)));
    setExifModel(Exiv2Helper::getExiv2ValueString(exifData, Exiv2::model(exifData)));

    //reading lens
    setExifLens(Exiv2Helper::getLensName(exifData));

    long orientation = Exiv2Helper::getExiv2ValueLong(exifData, "Exif.Image.Orientation");
    if (orientation>0 && trustExivOrientation())
    {
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

    long pixXdim = Exiv2Helper::getExiv2ValueLong(exifData,"Exif.Photo.PixelXDimension");
    long pixYdim = Exiv2Helper::getExiv2ValueLong(exifData,"Exif.Photo.PixelYDimension");

    if (pixXdim !=0 && pixYdim !=0 )
    {
        double ratioExif = pixXdim/(double)pixYdim;
        double ratioImage = getWidth()/(double)getHeight();
        if (fabs( ratioExif - ratioImage) > 0.1)
        {
            // Image has been modified without adjusting exif tags.
            // Assume user has rotated to upright pose
            roll = 0;
        }
    }
    // save for later
    setExifOrientation(roll);
    
    double cropFactor=Exiv2Helper::getCropFactor(exifData, getWidth(), getHeight());
    DEBUG_DEBUG("cropFactor: " << cropFactor);

    float eFocalLength = Exiv2Helper::getExiv2ValueDouble(exifData, Exiv2::focalLength(exifData));
    float eFocalLength35 = Exiv2Helper::getExiv2ValueLong(exifData,"Exif.Photo.FocalLengthIn35mmFilm");
    float focalLength=0;
    //The various methods to detmine crop factor
    if (eFocalLength > 0 && cropFactor > 0)
    {
        // user provided crop factor
        focalLength = eFocalLength;
    }
    else
    {
        if (eFocalLength35 > 0 && eFocalLength > 0)
        {
            cropFactor = eFocalLength35 / eFocalLength;
            focalLength = eFocalLength;
        }
        else
        {
            if (eFocalLength35 > 0)
            {
                // 35 mm equiv focal length available, crop factor unknown.
                // do not ask for crop factor, assume 1.  Probably a full frame sensor
                cropFactor = 1;
                focalLength = eFocalLength35;
            }
            else
            {
                if (eFocalLength > 0 && cropFactor <= 0)
                {
                    // need to redo, this time with crop
                    focalLength = eFocalLength;
                    cropFactor = 0;
                }
            };
        };
    };
    setExifFocalLength(focalLength);
    setExifFocalLength35(eFocalLength35);
    setExifCropFactor(cropFactor);

    setExifDistance(Exiv2Helper::getExiv2ValueDouble(exifData, Exiv2::subjectDistance(exifData)));
    setExifDate(Exiv2Helper::getExiv2ValueString(exifData, "Exif.Photo.DateTimeOriginal"));

    double redBalance, blueBalance;
    Exiv2Helper::readRedBlueBalance(exifData, redBalance, blueBalance);
    setExifRedBalance(redBalance);
    setExifBlueBalance(blueBalance);

    DEBUG_DEBUG("Results for:" << filename);
    DEBUG_DEBUG("Focal Length: " << getExifFocalLength());
    DEBUG_DEBUG("Crop Factor:  " << getCropFactor());
    DEBUG_DEBUG("Roll:         " << getExifOrientation());

    return true;
}

bool SrcPanoImage::applyEXIFValues(bool applyEVValue)
{
    setRoll(getExifOrientation());
    if(applyEVValue)
    {
        setExposureValue(calcExifExposureValue());
    };
    // special handling for GPano tags
    FileMetaData metaData = getFileMetadata();
    if (!metaData.empty())
    {
        FileMetaData::const_iterator pos = metaData.find("projection");
        if (pos != metaData.end())
        {
            if (pos->second == "equirectangular")
            {
                pos = metaData.find("HFOV");
                if (pos != metaData.end())
                {
                    double hfov = 0;
                    hugin_utils::stringToDouble(pos->second, hfov);
                    double e = 0;
                    pos = metaData.find("e");
                    if (pos != metaData.end())
                    {
                        hugin_utils::stringToDouble(pos->second, e);
                    };
                    if (hfov != 0)
                    {
                        setProjection(EQUIRECTANGULAR);
                        setHFOV(hfov);
                        setCropFactor(1.0);
                        hugin_utils::FDiff2D p = getRadialDistortionCenterShift();
                        p.y = e;
                        setRadialDistortionCenterShift(p);
                        return true;
                    };
                };
            };
        };
    };
    double cropFactor=getExifCropFactor();
    double focalLength=getExifFocalLength();
    if(cropFactor>0)
    {
        setCropFactor(cropFactor);
    };
    if (focalLength > 0 && cropFactor > 0)
    {
        setHFOV(calcHFOV(getProjection(), focalLength, cropFactor, getSize()));
        DEBUG_DEBUG("HFOV:         " << getHFOV());
        return true;
    }
    else
    {
        return false;
    }
}

bool SrcPanoImage::readCropfactorFromDB()
{
    // finally search in lens database
    if(getCropFactor()<=0 && !getExifMake().empty() && !getExifModel().empty())
    {
        double dbCrop=0;
        if(LensDB::LensDB::GetSingleton().GetCropFactor(getExifMake(),getExifModel(),dbCrop))
        {
            if(dbCrop>0)
            {
                setCropFactor(dbCrop);
                setExifCropFactor(dbCrop);
                return true;
            };
        };
    };
    return false;
};

std::string SrcPanoImage::getDBLensName() const
{
    std::string lens(getExifLens());
    if (!lens.empty())
    {
        return lens;
    }
    lens = getExifMake();
    if (!lens.empty())
    {
        if (!getExifModel().empty())
        {
            lens.append("|");
            lens.append(getExifModel());
            return lens;
        };
    };
    return std::string();
};

bool SrcPanoImage::readProjectionFromDB()
{
    bool success=false;
    const std::string lensname = getDBLensName();
    const double focal = getExifFocalLength();
    if (!lensname.empty())
    {
        const LensDB::LensDB& lensDB=LensDB::LensDB::GetSingleton();
        Projection dbProjection;
        if(lensDB.GetProjection(lensname, dbProjection))
        {
            setProjection(dbProjection);
            success=true;
        };
        if (focal>0)
        {
            double fov;
            if (lensDB.GetFov(lensname, focal, fov))
            {
                // calculate FOV for given image, take different aspect ratios into account
                const double newFocal = calcFocalLength(getProjection(), fov, getCropFactor(), vigra::Size2D(3000,2000));
                const double newFov = calcHFOV(getProjection(), newFocal, getCropFactor(), getSize());
                setHFOV(newFov);
            };
            vigra::Rect2D dbCropRect;
            if (lensDB.GetCrop(lensname, focal, getSize(), dbCropRect))
            {
                setCropMode(isCircularCrop() ? CROP_CIRCLE : CROP_RECTANGLE);
                setCropRect(dbCropRect);
            };
        };
    };
    return success;
};

bool SrcPanoImage::readDistortionFromDB()
{
    const std::string lensname = getDBLensName();
    const double focal = getExifFocalLength();
    if (!lensname.empty() && focal > 0)
    {
        const LensDB::LensDB& lensDB=LensDB::LensDB::GetSingleton();
        std::vector<double> dist;
        if(lensDB.GetDistortion(lensname, focal, dist))
        {
            if(dist.size()==3)
            {
                dist.push_back(1.0-dist[0]-dist[1]-dist[2]);
                setRadialDistortion(dist);
                return true;
            };
        };
    };
    return false;
};

bool SrcPanoImage::readVignettingFromDB()
{
    const std::string lensname = getDBLensName();
    const double focal = getExifFocalLength();
    if (!lensname.empty() && focal > 0)
    {
        const LensDB::LensDB& lensDB=LensDB::LensDB::GetSingleton();
        std::vector<double> vig;
        if(lensDB.GetVignetting(lensname, focal, getExifAperture(), getExifDistance(), vig))
        {
            if (vig.size() == 4)
            {
                setRadialVigCorrCoeff(vig);
                return true;
            };
        };
    };
    return false;
};

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
        case SrcPanoImage::FISHEYE_ORTHOGRAPHIC:
            {
                double val=(sensorSize.x/2.0)/fl;
                double n;
                double frac=modf(val, &n);
                hfov = 2 * asin(frac) * 180.0/M_PI + n * 180.0;
            }
            break;
        case SrcPanoImage::FISHEYE_EQUISOLID:
            hfov = 4 * asin(std::min<double>(1.0, (sensorSize.x/4.0)/fl)) * 180.0/M_PI;
            break;
        case SrcPanoImage::FISHEYE_STEREOGRAPHIC:
            hfov = 4 * atan((sensorSize.x/4.0)/fl) * 180.0/M_PI;
            break;
        case SrcPanoImage::FISHEYE_THOBY:
            hfov = 2 * asin(std::min<double>(1.0, sensorSize.x/(2.0*fl*1.47))) * 180.0/M_PI/0.713;
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
        case SrcPanoImage::FISHEYE_ORTHOGRAPHIC:
            {
                int t=(int)ceil((hfov-180)/360);
                return (sensorSize.x /2.0) / (2 * t + pow ( -1.0, t) * sin(hfov/180.0*M_PI/2.0));
            };
        case SrcPanoImage::FISHEYE_STEREOGRAPHIC:
            return (sensorSize.x/4.0) / tan(hfov/180.0*M_PI/4.0);
        case SrcPanoImage::FISHEYE_EQUISOLID:
            return (sensorSize.x/4.0) / sin(hfov/180.0*M_PI/4.0);
        case SrcPanoImage::FISHEYE_THOBY:
            return (sensorSize.x/2.0) / (1.47 * sin(hfov/180.0*M_PI * 0.713 / 2.0));
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
        case SrcPanoImage::FISHEYE_ORTHOGRAPHIC:
        case SrcPanoImage::FISHEYE_STEREOGRAPHIC:
        case SrcPanoImage::FISHEYE_EQUISOLID:
        case SrcPanoImage::FISHEYE_THOBY:
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

double SrcPanoImage::calcExifExposureValue()
{
    double ev=0;
    double photoFNumber=getExifAperture();
    if(photoFNumber==0)
    {
        //if no F-number was found in EXIF data assume a f stop of 3.5 to get
        //a reasonable ev value if shutter time, e. g. for manual lenses is found
        photoFNumber=3.5;
    };
    if (getExifExposureTime() > 0)
    {
        double gain = 1;
        if (getExifISO()> 0)
        {
            gain = getExifISO() / 100.0;
        }
        ev = log2(photoFNumber * photoFNumber / (gain * getExifExposureTime()));
    };
    return ev;
};

void SrcPanoImage::updateFocalLength(double newFocalLength)
{
    double newHFOV=calcHFOV(getProjection(),newFocalLength,getCropFactor(),getSize());
    if(newHFOV!=0)
    {
        setHFOV(newHFOV);
    };
};

void SrcPanoImage::updateCropFactor(double focalLength, double newCropFactor)
{
    double newHFOV=calcHFOV(getProjection(),focalLength,newCropFactor,getSize());
    if(newHFOV!=0)
    {
        setHFOV(newHFOV);
    };
    setCropFactor(newCropFactor);
};

// mask handling stuff
void SrcPanoImage::addMask(MaskPolygon newMask)
{
    MaskPolygonVector newMasks=m_Masks.getData();
    newMasks.push_back(newMask);
    setMasks(newMasks);
};

void SrcPanoImage::addActiveMask(MaskPolygon newMask)
{
    MaskPolygonVector newMasks=m_ActiveMasks.getData();
    newMasks.push_back(newMask);
    setActiveMasks(newMasks);
};

void SrcPanoImage::clearActiveMasks()
{
    MaskPolygonVector emptyMaskVector;
    m_ActiveMasks.setData(emptyMaskVector);
};

bool SrcPanoImage::hasMasks() const
{
    return m_Masks.getData().size()>0;
};

bool SrcPanoImage::hasPositiveMasks() const
{
    MaskPolygonVector masks=m_Masks.getData();
    if(masks.size()>0)
    {
        for(unsigned int i=0;i<masks.size();i++)
        {
            if(masks[i].isPositive())
            {
                return true;
            };
        };
    };
    return false;
};

bool SrcPanoImage::hasActiveMasks() const
{
    return m_ActiveMasks.getData().size()>0;
};
 
void SrcPanoImage::printMaskLines(std::ostream &o, unsigned int newImgNr) const
{
    if(m_Masks.getData().size()>0)
        for(unsigned int i=0;i<m_Masks.getData().size();i++)
            m_Masks.getData()[i].printPolygonLine(o, newImgNr);
};

void SrcPanoImage::changeMaskType(unsigned int index, HuginBase::MaskPolygon::MaskType newType)
{
    if(index<m_Masks.getData().size())
    {
        MaskPolygonVector editedMasks=m_Masks.getData();
        editedMasks[index].setMaskType(newType);
        m_Masks.setData(editedMasks);
    };
};

void SrcPanoImage::deleteMask(unsigned int index)
{
    if(index<m_Masks.getData().size())
    {
        MaskPolygonVector oldMasks=m_Masks.getData();
        oldMasks.erase(oldMasks.begin()+index);
        m_Masks.setData(oldMasks);
    };
};

void SrcPanoImage::deleteAllMasks()
{
    MaskPolygonVector emptyMaskVector;
    m_Masks.setData(emptyMaskVector);
};

bool SrcPanoImage::isInsideMasks(vigra::Point2D p) const
{
    if(!hasActiveMasks())
        return false;
    bool insideMask=false;
    unsigned int i=0;
    while(!insideMask && i<m_ActiveMasks.getData().size())
    {
        insideMask=m_ActiveMasks.getData()[i].isInside(p);
        i++;
    };
    return insideMask;
};

/**
 * Decides if the Exiv Orientation Tag of an images is plausible.
 * Current checks:
 * - If width is smaller than height, image is probably already rotated, tag may be wrong.
 * @return true if plausible.
 */
bool SrcPanoImage::trustExivOrientation()
{
    if(getSize().width() < getSize().height())
        return false;

    return true;
}

const int SrcPanoImage::getExifDateTime(struct tm* datetime) const
{
    //initialize struct
    std::memset(datetime, 0x0, sizeof(*datetime));
    //ignore daylight saving flag because it is not saved in EXIF date time format
    datetime->tm_isdst=-1;
    return Exiv2::exifTime(m_ExifDate.getData().c_str(),datetime);
};

} // namespace

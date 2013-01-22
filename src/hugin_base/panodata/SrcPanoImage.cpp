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

bool SrcPanoImage::readEXIF(double & focalLength, double & cropFactor, bool applyEXIFValues, bool applyExposureValue)
{
    double eV=0;
    return readEXIF(focalLength,cropFactor,eV,applyEXIFValues, applyExposureValue);
};

bool SrcPanoImage::readEXIF(double & focalLength, double & cropFactor, double & eV, bool applyEXIFValues, bool applyExposureValue)
{
    std::string filename = getFilename();
    std::string ext = hugin_utils::getExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int)) toupper);

    double roll = 0;
    //double eV = 0;
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
    } catch(std::exception & ) {
        return false;
    }

    // Setup image with default values
    setSize(vigra::Size2D(width, height));
    if (applyEXIFValues && focalLength > 0 && cropFactor > 0) {
        setHFOV(calcHFOV(getProjection(),
        focalLength, cropFactor, getSize()));
    }

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
    Exiv2::ExifData &exifData = image->exifData();
    if (exifData.empty()) {
        std::cerr << "Unable to read EXIF data from opened file:" << filename << std::endl;
        return false;
    }

    getExiv2Value(exifData,"Exif.Photo.ExposureTime",exposureTime);
    // TODO: reconstruct real exposure value from "rounded" ones saved by the cameras?

    getExiv2Value(exifData,"Exif.Photo.FNumber",photoFNumber);
    
    //remember aperture for later
    setExifAperture(photoFNumber);
    
    //read exposure mode
    long exposureMode=0;
    getExiv2Value(exifData,"Exif.Photo.ExposureMode",exposureMode);
    setExifExposureMode((int)exposureMode);

    //if no F-number was found in EXIF data assume a f stop of 3.5 to get
    //a reasonable ev value if shutter time, e. g. for manual lenses is found
    if(photoFNumber==0)
    {
        photoFNumber=3.5;
    };
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
    if (itr != exifData.end()) {
        setExifMake(itr->toString());
    } else {
        setExifMake("");
    }

    Exiv2::ExifKey key2("Exif.Image.Model");
    itr = exifData.findKey(key2);
    if (itr != exifData.end()) {
        setExifModel(itr->toString());
    } else {
        setExifModel("");
    }

    //reading lens
    // first we are reading LensModel in Exif section, this is only available
    // with EXIF >= 2.3
    std::string lensName;
#if EXIV2_TEST_VERSION(0,22,0)
    //the string "Exif.Photo.LensModel" is only defined in exiv2 0.22.0 and above
    if(getExiv2Value(exifData,"Exif.Photo.LensModel",lensName))
#else
    if(getExiv2Value(exifData,0xa434,"Photo",lensName))
#endif
    {
        if(lensName.length()>0)
        {
            setExifLens(lensName);
        }
        else
        {
            setExifLens("");
        }
    }
    else
    {
        //no lens in Exif found, now look in makernotes
        Exiv2::ExifData::const_iterator itr2 = Exiv2::lensName(exifData);
        if (itr2!=exifData.end() && itr2->count())
        {
            //we are using prettyPrint function to get string of lens name
            //it2->toString returns for many cameras only an ID number
            setExifLens(itr2->print(&exifData));
        }
        else
        {
            setExifLens("");
        };
    };

    long orientation = 0;
    if (getExiv2Value(exifData,"Exif.Image.Orientation",orientation) && trustExivOrientation()) {
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
    
    // some cameras do not provide Exif.Image.ImageWidth / Length
    // notably some Olympus
    
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

    // some cameras do not provide Exif.Photo.FocalPlaneResolutionUnit
    // notably some Olympus

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

    // some cameras do not provide Exif.Photo.FocalPlaneXResolution and
    // Exif.Photo.FocalPlaneYResolution, notably some Olympus

    float fplaneXresolution = 0;
    getExiv2Value(exifData,"Exif.Photo.FocalPlaneXResolution",fplaneXresolution);

    float fplaneYresolution = 0;
    getExiv2Value(exifData,"Exif.Photo.FocalPlaneYResolution",fplaneYresolution);

    float CCDWidth = 0;
    if (fplaneXresolution != 0) { 
//        CCDWidth = (float)(sensorPixelWidth * resolutionUnits / 
//                fplaneXresolution);
        CCDWidth = (float)(sensorPixelWidth / ( fplaneXresolution / resolutionUnits));
    }

    float CCDHeight = 0;
    if (fplaneYresolution != 0) {
        CCDHeight = (float)(sensorPixelHeight / ( fplaneYresolution / resolutionUnits));
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
        // FIXME: HACK guard against invalid image focal plane definition in EXIF metadata with arbitrarly chosen limits for the crop factor ( 1/100 < crop < 100)
        if (cropFactor < 0.01 || cropFactor > 100) {
            cropFactor = 0;
        }
    } else {
        // alternative way to calculate the crop factor for Olympus cameras

        // Windows debug stuff
        // left in as example on how to get "console output"
        // written to a log file    
        // freopen ("oly.log","a",stdout);
        // fprintf (stdout,"Starting Alternative crop determination\n");
        
        float olyFPD = 0;
        getExiv2Value(exifData,"Exif.Olympus.FocalPlaneDiagonal",olyFPD);

        if (olyFPD > 0.0) {        
            // Windows debug stuff
            // fprintf(stdout,"Oly_FPD:");
            // fprintf(stdout,"%f",olyFPD);
            cropFactor = sqrt(36.0*36.0+24.0*24.0) / olyFPD;
        }
        else {
            // for newer Olympus cameras the FocalPlaneDiagonal tag was moved into
            // equipment (sub?)-directory, so check also there
            getExiv2Value(exifData,"Exif.OlympusEq.FocalPlaneDiagonal",olyFPD);
            if (olyFPD > 0.0) {
                cropFactor = sqrt(36.0*36.0+24.0*24.0) / olyFPD;
            };
        };
   
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

    std::string captureDate;
    getExiv2Value(exifData,"Exif.Photo.DateTimeOriginal",captureDate);


    // store some important EXIF tags for later usage.
    setExifFocalLength(focalLength);
    setExifFocalLength35(eFocalLength35);
    setExifOrientation(roll);
    setExifISO(isoSpeed);
    setExifDistance(subjectDistance);
    setExifDate(captureDate);
    setExifExposureTime(exposureTime);

    DEBUG_DEBUG("Results for:" << filename);
    DEBUG_DEBUG("Focal Length: " << getExifFocalLength());
    DEBUG_DEBUG("Crop Factor:  " << getExifCropFactor());
    DEBUG_DEBUG("Roll:         " << getExifOrientation());

    // Update image with computed values from EXIF
    if (applyEXIFValues) {
        setRoll(roll);
        if (applyExposureValue)
            setExposureValue(eV);
        if(cropFactor>0)
        {
            setExifCropFactor(cropFactor);
        };
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

bool SrcPanoImage::readCropfactorFromDB()
{
    // finally search in lensfun database
    if(getExifCropFactor()<=0 && !getExifMake().empty() && !getExifModel().empty())
    {
        double dbCrop=0;
        if(LensDB::LensDB::GetSingleton().GetCropFactor(getExifMake(),getExifModel(),dbCrop))
        {
            if(dbCrop>0)
            {
                setExifCropFactor(dbCrop);
                return true;
            };
        };
    };
    return false;
};

bool SrcPanoImage::readProjectionFromDB()
{
    bool success=false;
    if(!getExifLens().empty())
    {
        LensDB::LensDB& lensDB=LensDB::LensDB::GetSingleton();
        if(lensDB.FindLens(getExifMake(), getExifModel(), getExifLens()))
        {
            Projection dbProjection;
            if(lensDB.GetProjection(dbProjection))
            {
                setProjection(dbProjection);
                success=true;
            };
            if(getExifFocalLength()>0)
            {
                CropMode dbCropMode;
                FDiff2D cropLeftTop;
                FDiff2D cropRightBottom;
                if(lensDB.GetCrop(getExifFocalLength(),dbCropMode,cropLeftTop,cropRightBottom))
                {
                    switch(dbCropMode)
                    {
                        case NO_CROP:
                            setCropMode(NO_CROP);
                            break;
                        case CROP_CIRCLE:
                            if(isCircularCrop())
                            {
                                setCropMode(CROP_CIRCLE);
                                int width=getSize().width();
                                int height=getSize().height();
                                if(width>height)
                                {
                                    setCropRect(vigra::Rect2D(cropLeftTop.x*width,cropLeftTop.y*height,cropRightBottom.x*width,cropRightBottom.y*height));
                                }
                                else
                                {
                                    setCropRect(vigra::Rect2D((1.0-cropRightBottom.y)*width,cropLeftTop.x*height,(1.0-cropLeftTop.y)*width,cropRightBottom.x*height));
                                };
                            };
                            break;
                        case CROP_RECTANGLE:
                            if(!isCircularCrop())
                            {
                                int width=getSize().width();
                                int height=getSize().height();
                                setCropMode(CROP_RECTANGLE);
                                if(width>height)
                                {
                                    setCropRect(vigra::Rect2D(cropLeftTop.x*width,cropLeftTop.y*height,cropRightBottom.x*width,cropRightBottom.y*height));
                                }
                                else
                                {
                                    setCropRect(vigra::Rect2D((1.0-cropRightBottom.y)*width,cropLeftTop.x*height,(1.0-cropLeftTop.y*width),cropRightBottom.x*height));
                                };
                                fprintf(stdout,"crop rect set: %f,%f-%f,%f \n",getCropRect().left(),getCropRect().top(),getCropRect().right(),getCropRect().bottom());

                            };
                            break;
                    };
                };
            };
        };
    };
    return success;
};

bool SrcPanoImage::readDistortionFromDB()
{
    bool success=false;
    if(!getExifLens().empty() || (!getExifMake().empty() && !getExifModel().empty()))
    {
        LensDB::LensDB& lensDB=LensDB::LensDB::GetSingleton();
        if(lensDB.FindLens(getExifMake(), getExifModel(), getExifLens()))
        {
            if(getExifFocalLength()>0)
            {
                std::vector<double> dist;
                if(lensDB.GetDistortion(getExifFocalLength(),dist))
                {
                    if(dist.size()==3)
                    {
                        dist.push_back(1.0-dist[0]-dist[1]-dist[2]);
                        setRadialDistortion(dist);
                        success=true;
                    };
                };
            };
        };
    };
    return success;
};

bool SrcPanoImage::readVignettingFromDB()
{
    bool success=false;
    if(!getExifLens().empty() || (!getExifMake().empty() && !getExifModel().empty()))
    {
        LensDB::LensDB& lensDB=LensDB::LensDB::GetSingleton();
        if(lensDB.FindLens(getExifMake(), getExifModel(), getExifLens()))
        {
            if(getExifFocalLength()>0)
            {
                std::vector<double> vig;
                if(lensDB.GetVignetting(getExifFocalLength(),getExifAperture(),getExifDistance(),vig))
                {
                    setRadialVigCorrCoeff(vig);
                    success=true;
                };
            };
        };
    };
    return success;
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

void SrcPanoImage::updateFocalLength(double newFocalLength)
{
    double newHFOV=calcHFOV(getProjection(),newFocalLength,getExifCropFactor(),getSize());
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
    setExifCropFactor(newCropFactor);
};

// Convenience functions to work with Exiv2
bool SrcPanoImage::getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, long & value)
{
    Exiv2::ExifKey key(keyName);
    Exiv2::ExifData::iterator itr = exifData.findKey(key);
    if (itr != exifData.end() && itr->count()) {
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
    if (itr != exifData.end() && itr->count()) {
        value = itr->toFloat();
        DEBUG_DEBUG("" << keyName << ": " << value);
        return true;
    } else {
        return false;
    }
}


bool SrcPanoImage::getExiv2Value(Exiv2::ExifData& exifData, std::string keyName, std::string & value)
{
    Exiv2::ExifKey key(keyName);
    Exiv2::ExifData::iterator itr = exifData.findKey(key);
    if (itr != exifData.end() && itr->count()) {
        value = itr->toString();
        DEBUG_DEBUG("" << keyName << ": " << value);
        return true;
    } else {
        return false;
    }
}

bool SrcPanoImage::getExiv2Value(Exiv2::ExifData& exifData, uint16_t tagID, std::string groupName, std::string & value)
{
    Exiv2::ExifKey key(tagID,groupName);
    Exiv2::ExifData::iterator itr = exifData.findKey(key);
    if (itr != exifData.end() && itr->count())
    {
        value = itr->toString();
        return true;
    }
    else
    {
        return false;
    }
}

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

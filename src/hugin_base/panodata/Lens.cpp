// -*- c-basic-offset: 4 -*-

/** @file PanoramaMemento.cpp
 *
 *  @brief implementation of PanoramaMemento Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PanoramaMemento.cpp 1998 2007-05-10 06:26:46Z dangelo $
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

#include "Lens.h"

#include <vigra/impex.hxx>
#include <jhead/jhead.h>
#include <hugin_utils/utils.h>
#include <hugin_utils/stl_utils.h>


namespace HuginBase {

    
    
Lens::Lens()
    : m_hasExif(false), m_projectionFormat(RECTILINEAR),
      m_imageSize(0,0), m_sensorSize(36.0,24.0)
{
    fillLensVarMap(variables);
}


char* Lens::variableNames[] = { "v", "a", "b", "c", "d", "e", "g", "t",
                                    "Va", "Vb", "Vc", "Vd", "Vx", "Vy", 
                                    "Eev", "Er", "Eb",
                                    "Ra", "Rb", "Rc", "Rd", "Re",  0};

double Lens::getHFOV() const
{
    return const_map_get(this->variables,"v").getValue();
}

void Lens::setHFOV(double d)
{
    map_get(variables,"v").setValue(d);
}

double Lens::getFocalLength() const
{

    double HFOV = const_map_get(variables,"v").getValue();
#if 0
    if (isLandscape()) {
        ssize = m_sensorSize;
    } else {
        ssize.y = m_sensorSize.x;
        ssize.x = m_sensorSize.y;
    }
#endif

    switch (m_projectionFormat)
    {
        case RECTILINEAR:
            return (m_sensorSize.x/2.0) / tan(HFOV/180.0*M_PI/2);
            break;
        case CIRCULAR_FISHEYE:
        case FULL_FRAME_FISHEYE:
            // same projection equation for both fisheye types,
            // assume equal area projection.
            return m_sensorSize.x / (HFOV/180*M_PI);
            break;
        default:
            // TODO: add formulas for other projections
            DEBUG_WARN("Focal length calculations only supported with rectilinear and fisheye images");
            return 0;
    }
}

void Lens::setEV(double ev)
{
    map_get(variables, "Eev").setValue(ev);
}

void Lens::setFocalLength(double fl)
{
#if 0
    if (isLandscape()) {
        ssize = m_sensorSize;
    } else {
        ssize.y = m_sensorSize.x;
        ssize.x = m_sensorSize.y;
    }
#endif

    double hfov=map_get(variables, "v").getValue();
    switch (m_projectionFormat) {
        case RECTILINEAR:
            hfov = 2*atan((m_sensorSize.x/2.0)/fl)  * 180.0/M_PI;
            break;
        case CIRCULAR_FISHEYE:
        case FULL_FRAME_FISHEYE:
            hfov = m_sensorSize.x / fl * 180/M_PI;
        default:
            // TODO: add formulas for other projections
            DEBUG_WARN("Focal length calculations only supported with rectilinear and fisheye images");
    }
    map_get(variables, "v").setValue(hfov);
}


void Lens::setCropFactor(double factor)
{
    // calculate diagonal on our sensor
    double d = sqrt(36.0*36.0 + 24.0*24.0) / factor;

    double r = (double)m_imageSize.x / m_imageSize.y;

    // calculate the sensor width and height that fit the ratio
    // the ratio is determined by the size of our image.
    m_sensorSize.x = d / sqrt(1 + 1/(r*r));
    m_sensorSize.y = m_sensorSize.x / r;
}

double Lens::getCropFactor() const
{
    double d2 = m_sensorSize.x*m_sensorSize.x + m_sensorSize.y*m_sensorSize.y;
    return sqrt(36.0*36+24*24) / sqrt(d2);
}


double Lens::getAspectRatio() const
{
    return (double)m_imageSize.x / m_imageSize.y;
}


bool Lens::isLandscape() const
{
    return m_imageSize.x >= m_imageSize.y;
}


bool Lens::initFromFile(const std::string & filename, double &cropFactor, double & roll)
{
    std::string ext = hugin_utils::getExtension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), (int(*)(int)) toupper);

    roll = 0;
    int width;
    int height;
    try {
        vigra::ImageImportInfo info(filename.c_str());
        width = info.width();
        height = info.height();
    } catch(vigra::PreconditionViolation & ) {
        return false;
    }
    setImageSize(vigra::Size2D(width, height));

    if (ext != "JPG" && ext != "JPEG") {
        return false;
    }

    ImageInfo_t exif;
    ResetJpgfile();
    // Start with an empty image information structure.

    memset(&exif, 0, sizeof(exif));
    exif.FlashUsed = -1;
    exif.MeteringMode = -1;

    if (!ReadJpegFile(exif,filename.c_str(), READ_EXIF)){
        DEBUG_DEBUG("Could not read jpg info");
        return false;
    }

#ifdef DEBUG
    ShowImageInfo(exif);
#endif

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

    std::cout << "exp time: " << exif.ExposureTime  << " f-stop: " <<  exif.ApertureFNumber << std::endl;
    // calculate exposure from exif image
    if (exif.ExposureTime > 0 && exif.ApertureFNumber > 0) {
        setEV(log2(exif.ApertureFNumber*exif.ApertureFNumber/exif.ExposureTime));
    }

    // calc sensor dimensions if not set and 35mm focal length is available
    FDiff2D sensorSize;
    double focalLength = 0;


    if (exif.FocalLength > 0 && exif.CCDHeight > 0 && exif.CCDWidth > 0) {
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
        focalLength = exif.FocalLength;
    } else if (exif.FocalLength35mm > 0 && exif.FocalLength > 0) {
        cropFactor = exif.FocalLength35mm / exif.FocalLength;
        focalLength = exif.FocalLength;
    } else if (exif.FocalLength35mm > 0 && cropFactor <= 0) {
        // do not ask for crop factor, even if we will store an invalid sensor size.
        // currenty the sensor size (just the ratio) is not used anywhere.
        cropFactor = 1;
        focalLength = exif.FocalLength35mm;
    } else if (exif.FocalLength > 0 || exif.FocalLength35mm > 0 ) {
        // no complete specification found.. ask the user for sensor/chip size, or crop factor
        if (cropFactor > 0) {
            // crop factor was provided by user
        } else {
            // need to redo, this time with crop
            cropFactor = -1;
            return false;
        }
        if (exif.FocalLength > 0 ) {
            focalLength = exif.FocalLength;
        } else if (exif.FocalLength35mm) {
            focalLength = exif.FocalLength35mm * cropFactor;
        }
    }

    if (sensorSize.x > 0) {
        setSensorSize(sensorSize);
    } else if (cropFactor > 0) {
        setCropFactor(cropFactor);
    } else {
        return false;
    }

    if (focalLength > 0) {
        setFocalLength(focalLength);
    } else {
        return false;
    }

    return true;
}


void Lens::update(const Lens & l)
{
    m_projectionFormat = l.m_projectionFormat;
    m_sensorSize = l.m_sensorSize;
    m_imageSize = l.m_imageSize;
    variables = l.variables;
}



} //namespace

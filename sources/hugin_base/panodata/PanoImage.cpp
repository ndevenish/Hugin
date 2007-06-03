// -*- c-basic-offset: 4 -*-
// PanoImage.cpp
//
// Pablo d'Angelo <pablo.dangelo@web.de>
// Last change: Time-stamp: <27-Okt-2004 19:05:03 pablo@svalbart>
//
//

#include <config.h>
#include <stdio.h>
#include <math.h>
//#include <setjmp.h>

#include <stdexcept>

#include <vigra/impex.hxx>

#include <jhead/jhead.h>


#include <PT/PanoImage.h>
#include <PT/Panorama.h>
#include <common/utils.h>
//#include "hugin/ImageCache.h"

using namespace PT;
using namespace std;
using namespace utils;

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

#if 0
QDomElement ImageOptions::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("options");
    root.setAttribute("feather_width", featherWidth);
    root.setAttribute("ignore_frame_width", ignoreFrameWidth);
    root.setAttribute("morph", morph);
    root.setAttribute("optimize_yaw", optimizeYaw);
    root.setAttribute("optimize_roll", optimizeRoll);
    root.setAttribute("optimize_pitch", optimizePitch);
    root.setAttribute("optimize_HFOV", optimizeFOV);
    root.setAttribute("optimize_a", optimizeA);
    root.setAttribute("optimize_b", optimizeB);
    root.setAttribute("optimize_c", optimizeC);
    root.setAttribute("optimize_d", optimizeD);
    root.setAttribute("optimize_e", optimizeE);

    return root;
}


void ImageOptions::setFromXML(const QDomNode & node)
{
    DEBUG_DEBUG("ImageOptions::setFromXML");
    Q_ASSERT(node.nodeName() == "options");
    QDomNamedNodeMap attrs = node.attributes();
    featherWidth = getAttrib(attrs, "feather_width").toUInt();
    ignoreFrameWidth = getAttrib(attrs, "ignore_frame_width").toUInt();
    morph = getAttrib(attrs, "morph").toUInt() != 0;
}

#endif

PanoImage::PanoImage(const std::string &filename, int width, int height,
                     int lens)
    : filename(filename),
      height(height),
      width(width),
      lensNr(lens)
{
}
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


bool PT::initImageFromFile(SrcPanoImage & img, double & focalLength, double & cropFactor)
{
    std::string filename = img.getFilename();
    std::string ext = utils::getExtension(filename);
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
                if (abs( ratioExif - ratioImage) > 0.1) {
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

double PT::calcHFOV(SrcPanoImage::Projection proj, double fl, double crop, vigra::Size2D imageSize)
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

double PT::calcFocalLength(SrcPanoImage::Projection proj, double hfov, double crop, vigra::Size2D imageSize)
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

#if 0
PanoImage::PanoImage(const std::string &filename)
    : filename(filename)
{
    DEBUG_DEBUG("PanoImage ctor for " << filename);
    init();
    readImageInformation();
}
#endif

void PanoImage::init()
{
    height = 0;
    width = 0;
}


#if 0
PanoImage::PanoImage(QDomNode & node)
{
    setFromXML(node);
}
#endif


PanoImage::~PanoImage()
{
    DEBUG_DEBUG("PanoImage dtor, image= " << filename);
}

#if 0
void PanoImage::setFromXML(QDomNode & elem)
{
    assert(elem.nodeName() == "image");
    QDomNamedNodeMap attrs = elem.attributes();
    filename = getAttrib(attrs, "file");
    isLandscape = getAttrib(attrs, "landscape").toUInt() != 0;
    height = getAttrib(attrs, "height").toUInt();
    width = getAttrib(attrs, "width").toUInt();

    qDebug("PanoImage::setFromXML, file %s", filename.ascii());

    options.setFromXML(elem.namedItem("position"));
    lens.setFromXML(elem.namedItem("lens"));
    options.setFromXML(elem.namedItem("options"));
}


QDomElement PanoImage::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("image");
    root.setAttribute("file", filename);
    root.setAttribute("landscape", isLandscape);
    root.setAttribute("height",height);
    root.setAttribute("width",width);
    root.appendChild(lens.toXML(doc));
    root.appendChild(position.toXML(doc));
    root.appendChild(options.toXML(doc));
    return root;
}
#endif

/*
bool PanoImage::readImageInformation()
{
  std::string::size_type idx = filename.rfind('.');
  if (idx == std::string::npos) {
      DEBUG_DEBUG("could not find extension in filename");
      return false;
  }
  std::string ext = filename.substr( idx+1 );

  DEBUG_ERROR("readImageInformation should be set Image information");
  // this depends on wxWindow
//  wxImage * image = ImageCache::getInstance().getImage(filename);
//  width = image->GetWidth();
//  height = image->GetHeight();


  try {
      // find another platform independant way to read the image info..
      Magick::Image img;
      img.ping(filename.c_str());
      width = img.baseColumns();
      height = img.baseRows();
  } catch( Magick::Exception &e ) {
      DEBUG_NOTICE("error while reading file information for " << filename
                   << ": " << e.what());
      return false;
  }

//  isLandscape = (width > height);
  return true;
}

*/

// -*- c-basic-offset: 4 -*-
// PanoImage.cpp
//
// Pablo d'Angelo <pablo@mathematik.uni-ulm.de>
// Last change: Time-stamp: <04-Apr-2003 23:09:30 pablo@island.wh-wurm.uni-ulm.de>
//
//



#include <stdio.h>
#include <math.h>
//#include <setjmp.h>

#include <stdexcept>

extern "C" {
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <jpeglib.h>
}

#include "PanoImage.h"
#include "Panorama.h"



using namespace PT;

QDomElement LensSettings::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("lens");

    root.setAttribute("projection", projectionFormat);
    root.setAttribute("focal_length", focalLength);
    root.setAttribute("focal_length_conv_factor", focalLengthConversionFactor);
    root.setAttribute("HFOV", HFOV);
    root.setAttribute("a", a);
    root.setAttribute("b", b);
    root.setAttribute("c", c);
    root.setAttribute("d", d);
    root.setAttribute("e", e);
    root.setAttribute("exif_focal_length", exifFocalLength);
    root.setAttribute("exif_focal_length_conv_factor",
                      exifFocalLengthConversionFactor);
    root.setAttribute("exif_HFOV", exifHFOV);
    return root;
}

void LensSettings::setFromXML(const QDomNode & node)
{
    qDebug("LensSettings::setFromXML");
    Q_ASSERT(node.nodeName() == "lens");
    QDomNamedNodeMap attr = node.attributes();
    projectionFormat = (ProjectionFormat) getAttrib(attr, "projection").toUInt();
    focalLength = getAttrib(attr, "focal_length").toDouble();
    focalLengthConversionFactor = getAttrib(attr, "focal_length_conv_factor").toDouble();
    HFOV = getAttrib(attr, "HFOV").toDouble();
    a = getAttrib(attr, "a").toDouble();
    b = getAttrib(attr, "b").toDouble();
    c = getAttrib(attr, "c").toDouble();
    d = getAttrib(attr, "d").toDouble();
    e = getAttrib(attr, "e").toDouble();
    exifFocalLength = getAttrib(attr, "exif_focal_length").toDouble();
    exifFocalLengthConversionFactor = getAttrib(attr, "exif_focal_length_conv_factor").toDouble();
    exifHFOV = getAttrib(attr, "exif_HFOV").toDouble();
}


QDomElement ImagePosition::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("position");

    root.setAttribute("yaw", yaw);
    root.setAttribute("pitch", pitch);
    root.setAttribute("roll", roll);

    return root;
}


void ImagePosition::setFromXML(const QDomNode & node)
{
    qDebug("ImagePosition::setFromXML");
    Q_ASSERT(node.nodeName() == "position");
    QDomNamedNodeMap attrs = node.attributes();
    yaw = getAttrib(attrs, "yaw").toDouble();
    roll = getAttrib(attrs, "roll").toDouble();
    pitch = getAttrib(attrs, "pitch").toDouble();
}


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
    qDebug("ImageOptions::setFromXML");
    Q_ASSERT(node.nodeName() == "options");
    QDomNamedNodeMap attrs = node.attributes();
    featherWidth = getAttrib(attrs, "feather_width").toUInt();
    ignoreFrameWidth = getAttrib(attrs, "ignore_frame_width").toUInt();
    morph = getAttrib(attrs, "morph").toUInt() != 0;
    optimizeYaw = getAttrib(attrs, "optimize_yaw").toUInt() != 0;
    optimizeRoll = getAttrib(attrs, "optimize_roll").toUInt() != 0;
    optimizePitch = getAttrib(attrs, "optimize_pitch").toUInt() != 0;
    optimizeFOV = getAttrib(attrs, "optimize_HFOV").toUInt() != 0;
    optimizeA = getAttrib(attrs, "optimize_a").toUInt() != 0;
    optimizeB = getAttrib(attrs, "optimize_b").toUInt() != 0;
    optimizeC = getAttrib(attrs, "optimize_c").toUInt() != 0;
    optimizeD = getAttrib(attrs, "optimize_d").toUInt() != 0;
    optimizeE = getAttrib(attrs, "optimize_e").toUInt() != 0;
}


PanoImage::PanoImage(Panorama & parent, const QString &filename)
    : pano(parent), filename(filename)
{
  qDebug("PanoImage ctor for %s\n",filename.ascii());
  init();
  readImageInformation();
}

void PanoImage::init()
{
  isLandscape = true;
  height = 0;
  width = 0;
}


PanoImage::PanoImage(Panorama & parent, QDomNode & node)
    : pano(parent)
{
    setFromXML(node);
}


PanoImage::~PanoImage()
{
    qDebug("PanoImage dtor, image=%s", getFilename().ascii());
}


void PanoImage::setFromXML(QDomNode & elem)
{
    Q_ASSERT(elem.nodeName() == "image");
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

static ExifEntry *
search_entry (ExifData *ed, ExifTag tag)
{
  ExifEntry *entry;
  unsigned int i;

  for (i = 0; i < EXIF_IFD_COUNT; i++) {
    entry = exif_content_get_entry (ed->ifd[i], tag);
    if (entry)
      return entry;
  }
  return 0;
}

bool PanoImage::readImageInformation()
{
  bool ok;
  QString ext = filename.section( '.', -1 ).lower();
  if (ext == "jpg") {
    ok = readJPEGInfo();
  } else if (ext == "tiff" || ext == "tif") {
    ok = readTIFFInfo();
  } else if (ext == "png") {
    ok = readPNGInfo();
  } else {
    qFatal("Unknown Image format: %s",filename.ascii());
    return false;
  }
  if (!ok) {
    qFatal("error while reading file information for %s",filename.ascii());
    return false;
  }
  isLandscape = (width > height);
  return true;
}

bool PanoImage::readJPEGInfo()
{
  // read normal jpeg header for image dimensions

  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  struct jpeg_error_mgr jerr;
  /* More stuff */
  FILE * infile;                /* source file */

  /* In this example we want to open the input file before doing anything else,
   * so that the setjmp() error recovery below can assume the file is open.
   * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
   * requires it in order to read binary files.
   */

  if ((infile = fopen(filename.ascii(), "rb")) == NULL) {
    qWarning("can't open %s\n", filename.ascii());
    return false;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We set up the normal JPEG error routines */
  cinfo.err = jpeg_std_error(&jerr);
  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);

  jpeg_destroy_decompress(&cinfo);
  fclose (infile);

  width = cinfo.image_width;
  height = cinfo.image_height;

  isLandscape = (width > height);

  // Try to read EXIF data from the file.
  ExifData * ed = 0;
  ed = exif_data_new_from_file (filename.ascii());
  if (!ed) {
    qDebug("'%s' does not contain EXIF data\n", filename.ascii());
    return true;
  }
  ExifByteOrder order = exif_data_get_byte_order (ed);
  qDebug("EXIF tags %s('%s' byte order):", filename.ascii(),
         exif_byte_order_get_name (order));

  ExifEntry * entry=0;

  // read real focal length;
  entry = search_entry(ed,EXIF_TAG_FOCAL_LENGTH);
  if (entry) {
    ExifRational t = exif_get_rational(entry->data, order);
    lens.exifFocalLength = (double) t.numerator/t.denominator;
  } else {
    return true;
  }

  // resolution in mm.
  double resUnit = 0;
  // read focal plane resolution unit
  entry = search_entry(ed,EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT);
  if (entry) {
    ExifShort res_unit = exif_get_short(entry->data, order);
    switch(res_unit) {
    case 2:
      resUnit = 25.4;
      break;
    case 3:
      resUnit = 10;
    default:
      qWarning("Unknown Focal Plane Resolution Unit in EXIF tag: %d, assuming inch", res_unit);
      resUnit = 2.54;
    }
  } else {
    return true;
  }

  // in mm
  double ccdWidth = 0;
  if (isLandscape) {
    qDebug("landscape image");
    // read focal plane x resolution
    entry = search_entry(ed,EXIF_TAG_FOCAL_PLANE_X_RESOLUTION);
    if (entry) {
      ExifRational t = exif_get_rational(entry->data, order);
      double ccdRes = (double) t.numerator/t.denominator;
      // BUG need to use width during capture.. it might have been
      // resized
      ccdWidth = width * resUnit / ccdRes;
    } else {
      return true;
    }
  } else {
    qDebug("portrait image");
    // read focal plane y resolution (we have a portrait image)
    entry = search_entry(ed,EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION);
    if (entry) {
      ExifRational t = exif_get_rational(entry->data, order);
      double ccdRes = (double) t.numerator/t.denominator;
      // BUG need to use height during capture.. it might have been
      // resized
      ccdWidth = height * resUnit / ccdRes;
    } else {
      return true;
    }
  }
  lens.HFOV = lens.exifHFOV = 2.0 * atan((ccdWidth/2)/lens.exifFocalLength) * 180/M_PI;
  lens.exifFocalLengthConversionFactor = 36 / ccdWidth;
  qDebug("CCD size: %f mm", ccdWidth);
  qDebug("focal length: %f, 35mm: %f HFOV %f",
         lens.exifFocalLength,
         lens.exifFocalLength * lens.exifFocalLengthConversionFactor,
         lens.HFOV);
  exif_data_unref (ed);
  return true;
}

bool PanoImage::readTIFFInfo()
{
  qFatal("readTIFFInfo not implemented");
  return false;
}

bool PanoImage::readPNGInfo()
{
  qFatal("readPNGInfo not implemented");
  return false;
}


void PanoImage::printImageLine(std::ostream &o)
{
    o << "i w" << width << " h" << height
      <<" f" << lens.projectionFormat
      << " y" << position.yaw << " p" << position.pitch << " r" << position.roll;
    if (pano.hasCommonLens() && getNr() != 0) {
        o << " v=0 a=0 b=0 c=0";
    } else {
        o << " v" << lens.HFOV << " a" << lens.a << " b" << lens.b << " c" << lens.c;
    }
    o << " d" << lens.d << " e" << lens.e
      << " u" << options.featherWidth << " m" << options.ignoreFrameWidth
      << (options.morph ? " o" : "")
      << " n\"" << filename << "\"" << std::endl;
}

void PanoImage::printStitchImageLine(std::ostream &o)
{
    o << "o w" << width << " h" << height
      <<" f" << lens.projectionFormat
      << " y" << position.yaw << " p" << position.pitch << " r" << position.roll;
    o << " v" << lens.HFOV << " a" << lens.a << " b" << lens.b << " c" << lens.c;
    o << " d" << lens.d << " e" << lens.e
      << " u" << options.featherWidth << " m" << options.ignoreFrameWidth
      << (options.morph ? " o" : "")
      << " n\"" << filename << "\"" << std::endl;
}


void PanoImage::printOptimizeLine(std::ostream &o)
{
    unsigned int num = getNr();
    o << "v";
    if (options.optimizeYaw) o << " y" << num;
    if (options.optimizeRoll) o << " r" << num;
    if (options.optimizePitch) o << " p" << num;
    if (!pano.hasCommonLens() || num == 0 ) {
        if (options.optimizeFOV) o << " v" << num;
        if (options.optimizeA) o << " a" << num;
        if (options.optimizeB) o << " b" << num;
        if (options.optimizeC) o << " c" << num;
    }
    if (options.optimizeD) o << " d" << num;
    if (options.optimizeE) o << " e" << num;
    o << std::endl;
}

unsigned int PanoImage::getNr()
{
    std::vector<PanoImage*> imgs = pano.getImages();
    std::vector<PanoImage*>::const_iterator it;
    unsigned int i = 0;
    for (it=imgs.begin(); it != imgs.end(); ++it) {
        if (*it == this)
            return i;
        i++;
    }
    throw std::out_of_range("PanoImage::getNr()");
}

void PanoImage::changed()
{
    qDebug("Image has changed");
    pano.reportChangedImage(getNr());
}


const QPixmap & PanoImage::getPixmap()
{
    if (pixmap.isNull()) {
        qDebug("Loading image data from %s",filename.ascii());
        if (!pixmap.load(filename)) {
            qWarning("Could not load image file: %s", filename.ascii());
        }
    }
    return pixmap;
}

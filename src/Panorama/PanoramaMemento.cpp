// -*- c-basic-offset: 4 -*-

/** @file PanoramaMemento.cpp
 *
 *  @brief implementation of PanoramaMemento Class
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

#include <iostream>

extern "C" {
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <jpeglib.h>
}

#include "../utils.h"
#include "PT/PanoramaMemento.h"


using namespace PT;
using namespace std;

PanoramaMemento::~PanoramaMemento()
{

}


PanoramaMemento::PanoramaMemento(const PanoramaMemento & o)
{

}



ostream & Variable::print(ostream & o, bool printLinks) const
{
    o << name;
    if (linked && printLinks) {
        o << "=" << linkImage;
    } else {
        o << value;
    }
    return o;
}

std::ostream & ImageVariables::print(std::ostream & o, bool printLinks) const
{
    yaw.print(o, printLinks) << " ";
    roll.print(o, printLinks) << " ";
    pitch.print(o, printLinks) << " ";
    HFOV.print(o, printLinks) << " ";
    a.print(o, printLinks) << " ";
    b.print(o, printLinks) << " ";
    c.print(o, printLinks) << " ";
    d.print(o, printLinks) << " ";
    e.print(o, printLinks);
    return o;
};

void ImageVariables::updateValues(const ImageVariables & vars)
{
    yaw.setValue(vars.yaw.getValue());
    roll.setValue(vars.roll.getValue());
    pitch.setValue(vars.pitch.getValue());
    HFOV.setValue(vars.HFOV.getValue());
    a.setValue(vars.a.getValue());
    b.setValue(vars.b.getValue());
    c.setValue(vars.c.getValue());
    d.setValue(vars.d.getValue());
    e.setValue(vars.e.getValue());
}


std::ostream & OptimizerSettings::printOptimizeLine(std::ostream & o,
                                                    unsigned int num) const
{
    o << "v";
    if (yaw) o << " y" << num;
    if (roll) o << " r" << num;
    if (pitch) o << " p" << num;
    if (HFOV) o << " v" << num;
    if (a) o << " a" << num;
    if (b) o << " b" << num;
    if (c) o << " c" << num;
    if (d) o << " d" << num;
    if (e) o << " e" << num;
    return o << std::endl;
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


#if 0
QString PT::getAttrib(QDomNamedNodeMap map, QString name)
{
    return map.namedItem(name).nodeValue();
}


//=========================================================================
//=========================================================================


QDomElement Lens::toXML(QDomDocument & doc)
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

void Lens::setFromXML(const QDomNode & node)
{
    DEBUG_DEBUG("LensSettings::setFromXML");
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
#endif


bool Lens::readEXIF(const std::string & filename)
{
    bool isLandscape;
    int width, height;
    width = height = 0;

    std::string::size_type idx = filename.rfind('.');
    if (idx == std::string::npos) {
        DEBUG_DEBUG("could not find extension in filename");
        return false;
    }
    std::string ext = filename.substr( idx+1 );
    if (ext != "jpg" && ext != "JPG") {
        DEBUG_DEBUG("can only read lens data from jpg files, current ext:"
                    << ext);
        return false;
    }

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

    if ((infile = fopen(filename.c_str(), "rb")) == NULL) {
        DEBUG_NOTICE("can't open " << filename.c_str());
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
    ed = exif_data_new_from_file (filename.c_str());
    if (!ed) {
        DEBUG_NOTICE(filename << " does not contain EXIF data");
        return false;
    }
    ExifByteOrder order = exif_data_get_byte_order (ed);
    DEBUG_DEBUG(filename << " EXIF tags are " << exif_byte_order_get_name (order));

    ExifEntry * entry=0;

    // read real focal length;
    entry = search_entry(ed,EXIF_TAG_FOCAL_LENGTH);
    if (entry) {
        ExifRational t = exif_get_rational(entry->data, order);
        focalLength = exifFocalLength = (double) t.numerator/t.denominator;
    } else {
        DEBUG_NOTICE(filename << " does not contain EXIF focal length");
        return false;
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
            DEBUG_NOTICE("Unknown Focal Plane Resolution Unit in EXIF tag: "
                         <<resUnit <<", assuming inch");
            resUnit = 2.54;
        }
    } else {
        DEBUG_NOTICE("No EXIF_TAG_FOCAL_PLANE_RESOLUTION_UNIT found");
        return false;
    }

    // in mm
    double ccdWidth = 0;
    if (isLandscape) {
        DEBUG_DEBUG("landscape image");
        // read focal plane x resolution
        entry = search_entry(ed,EXIF_TAG_FOCAL_PLANE_X_RESOLUTION);
        if (entry) {
            ExifRational t = exif_get_rational(entry->data, order);
            double ccdRes = (double) t.numerator/t.denominator;
            // BUG need to use width during capture.. it might have been
            // resized
            ccdWidth = width * resUnit / ccdRes;
        } else {
            DEBUG_DEBUG("No EXIF_TAG_FOCAL_PLANE_X_RESOLUTION found");
            return false;
        }
    } else {
        DEBUG_DEBUG("portrait image");
        // read focal plane y resolution (we have a portrait image)
        entry = search_entry(ed,EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION);
        if (entry) {
            ExifRational t = exif_get_rational(entry->data, order);
            double ccdRes = (double) t.numerator/t.denominator;
            // BUG need to use height during capture.. it might have been
            // resized
            ccdWidth = height * resUnit / ccdRes;
        } else {
            DEBUG_DEBUG("No EXIF_TAG_FOCAL_PLANE_Y_RESOLUTION found");
            return false;
        }
    }
    HFOV = exifHFOV = 2.0 * atan((ccdWidth/2)/exifFocalLength) * 180/M_PI;
    focalLengthConversionFactor = exifFocalLengthConversionFactor = 36 / ccdWidth;
    DEBUG_DEBUG("CCD size: " << ccdWidth << " mm");
    DEBUG_DEBUG("focal length: " << exifFocalLength << ", 35mm equiv: "
              << exifFocalLength * exifFocalLengthConversionFactor
              << " HFOV: " << HFOV);
    exif_data_unref (ed);
    return true;
}


//=========================================================================
//=========================================================================

#if 0
ControlPoint::ControlPoint(Panorama & pano, const QDomNode & node)
{
    setFromXML(node,pano);
}
#endif


#if 0
QDomNode ControlPoint::toXML(QDomDocument & doc) const
{
    QDomElement elem = doc.createElement("control_point");
    elem.setAttribute("image1", image1->getNr());
    elem.setAttribute("x1", x1);
    elem.setAttribute("y1", y1);
    elem.setAttribute("image2", image2->getNr());
    elem.setAttribute("x2", x2);
    elem.setAttribute("y2", y2);
    elem.setAttribute("mode", mode);
    elem.setAttribute("distance",error);
    return elem;
}

void ControlPoint::setFromXML(const QDomNode & elem, Panorama & pano)
{
    DEBUG_DEBUG("ControlPoint::setFromXML");
    Q_ASSERT(elem.nodeName() == "control_point");
    QDomNamedNodeMap attrs = elem.attributes();
    image1 = pano.getImage(getAttrib(attrs,"image1").toUInt());
    x1 = getAttrib(attrs,"x1").toUInt();
    y1 = getAttrib(attrs,"y1").toUInt();
    image2 = pano.getImage(getAttrib(attrs,"image2").toUInt());
    x2 = getAttrib(attrs,"x2").toUInt();
    y2 = getAttrib(attrs,"y2").toUInt();
    error = getAttrib(attrs,"distance").toDouble();
    mode = (OptimizeMode) getAttrib(attrs, "mode").toUInt();
}

#endif

void ControlPoint::mirror()
{
    unsigned int ti;
    double td;
    ti =image1Nr; image1Nr = image2Nr, image2Nr = ti;
    td = x1; x1 = x2 ; x2 = td;
    td = y1; y1 = y2 ; y2 = td;
}

//=========================================================================
//=========================================================================


#if 0
QDomNode PanoramaOptions::toXML(QDomDocument & doc) const
{
    QDomElement elem = doc.createElement("output");
    elem.setAttribute("projection", projectionFormat);
    elem.setAttribute("HFOV", HFOV);
    elem.setAttribute("width", width);
    elem.setAttribute("height", height);
    elem.setAttribute("output", outfile);
    elem.setAttribute("format", outputFormat);
    elem.setAttribute("jpg_quality", quality);
    elem.setAttribute("progressive", progressive);
    elem.setAttribute("color_correction", colorCorrection);
    elem.setAttribute("color_ref_image", colorReferenceImage);
    elem.setAttribute("gamma", gamma);
    elem.setAttribute("interpolator", interpolator);
    return elem;
}

void PanoramaOptions::setFromXML(const QDomNode & elem)
{
    DEBUG_DEBUG("PanoramaOptions::setFromXML");
    Q_ASSERT(elem.nodeName() == "output");
    QDomNamedNodeMap attrs = elem.attributes();
    projectionFormat = (ProjectionFormat)getAttrib(attrs,"projection").toUInt();
    HFOV = getAttrib(attrs,"HFOV").toDouble();
    width = getAttrib(attrs,"width").toUInt();
    height = getAttrib(attrs,"height").toUInt();
    outfile = getAttrib(attrs,"output");
    outputFormat = getAttrib(attrs,"format");
    quality = getAttrib(attrs,"jpg_quality").toUInt();
    progressive = getAttrib(attrs,"height").toUInt() != 0;
    colorCorrection = (ColorCorrection) getAttrib(attrs, "color_correction").toUInt();
    colorReferenceImage = getAttrib(attrs, "color_ref_image").toUInt();
    gamma = getAttrib(attrs, "gamma").toDouble();
    interpolator = (Interpolator) getAttrib(attrs, "interpolator").toUInt();
}

#endif


void PanoramaOptions::printScriptLine(std::ostream & o) const
{
    o << "p f" << projectionFormat << " w" << width << " h" << height << " v" << HFOV
      << " n\"" << outputFormat << " q" << quality;
    if (progressive) {
        o << " g";
    }
    o << "\"";
    switch (colorCorrection) {
    case NONE:
        break;
    case BRIGHTNESS_COLOR:
        o << " k" << colorReferenceImage;
        break;
    case BRIGHTNESS:
        o << " b" << colorReferenceImage;
        break;
    case COLOR:
        o << " d" << colorReferenceImage;
        break;
    }
    o << std::endl;

    // misc options
    o << "m g" << gamma << " i" << interpolator << std::endl;
}

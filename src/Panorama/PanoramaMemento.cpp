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

#include <common/utils.h>
#include <jhead/jhead.h>
#include <PT/PanoramaMemento.h>

using namespace PT;
using namespace std;

PanoramaMemento::~PanoramaMemento()
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
    if (ext != "jpg" && ext != "JPG" && ext != "jpeg" && ext != "JPEG") {
        DEBUG_NOTICE("can only read lens data from jpg files, current ext:"
                     << ext);
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
    ShowImageInfo(exif);


    isLandscape = (exif.Width > exif.Height);
    double ccdWidth;
    if (!isLandscape) {
        ccdWidth = exif.CCDWidth;
    } else {
        // portrait images must use the ccd height instead
        // of ccd width. we assume that the pixels are squares
        ccdWidth = exif.CCDWidth * exif.Height / exif.Width;
    }

    HFOV = exifHFOV = 2.0 * atan((ccdWidth/2)/exif.FocalLength) * 180/M_PI;
    focalLengthConversionFactor = exifFocalLengthConversionFactor = 36 / ccdWidth;
    DEBUG_DEBUG("CCD size: " << ccdWidth << " mm");
    DEBUG_DEBUG("focal length: " << exifFocalLength << ", 35mm equiv: "
              << exifFocalLength * exifFocalLengthConversionFactor
              << " HFOV: " << HFOV);
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

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

#include <PT/Panorama.h>

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
#if 0
    yaw.setValue(vars.yaw.getValue());
    roll.setValue(vars.roll.getValue());
    pitch.setValue(vars.pitch.getValue());
    HFOV.setValue(vars.HFOV.getValue());
    a.setValue(vars.a.getValue());
    b.setValue(vars.b.getValue());
    c.setValue(vars.c.getValue());
    d.setValue(vars.d.getValue());
    e.setValue(vars.e.getValue());
#else
    yaw=vars.yaw;
    roll=vars.roll;
    pitch=vars.pitch;
    HFOV=vars.HFOV;
    a=vars.a;
    b=vars.b;
    c=vars.c;
    d=vars.d;
    e=vars.e;
#endif
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

/*
QString PT::getAttrib(QDomNamedNodeMap map, QString name)
{
    return map.namedItem(name).nodeValue();
}
*/

//=========================================================================
//=========================================================================

#if 0
void Lens::toXML(std::ostream & o)
{
    o << "<lens type=\"" << prjToString(projection) << "\">" << endl
      << root.setAttribute("focal_length", focalLength)
      << root.setAttribute("focal_length_conv_factor", focalLengthConversionFactor)
      << root.setAttribute("HFOV", HFOV)
      << "<distortion a=\"" << a
      << "\" b=\"" << b
      << "\" c=\"" << c
      << "\" d=\"" << d
      << "\" e=\"" << e << "\"/>" << endl;
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
    if ( !(HFOV  > 0.0) )
        HFOV = 90.0;
    if ( ccdWidth > 0.0 )
      focalLengthConversionFactor = exifFocalLengthConversionFactor = 36 / ccdWidth;
    focalLength = exifFocalLength = exif.FocalLength;
    focalLengthConversionFactor = exifFocalLengthConversionFactor = focalLengthConversionFactor;
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

std::string ControlPoint::modeNames[] = { "x_y", "x", "y" };


const std::string & ControlPoint::getModeName(OptimizeMode mode) const
{
    return modeNames[mode];
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
      << " n\"" << outputFormat;
    if ( outputFormat == "JPEG" ) {
      o << " q" << quality;
      if (progressive) {
          o << " g";
      }
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


bool PanoramaMemento::loadPTScript(std::istream &i)
{
    DEBUG_TRACE("");
    PTParseState state;
    string line;
    unsigned int lineNr = 0;
    while (!i.eof()) {
        std::getline(i, line);
        lineNr++;
        // check for a known line
        switch(line[0]) {
        case 'p':
        {
            DEBUG_DEBUG("p line: " << line);
            string format;
            int i;
            getParam(i,line,"f");
            options.projectionFormat = (PanoramaOptions::ProjectionFormat) i;
            getParam(options.width, line, "w");
            getParam(options.height, line, "h");
            getParam(options.HFOV, line, "v");
            // this is fragile.. hope nobody adds additional whitespace
            // and other arguments than q...
            // n"JPEG q80"
            getPTStringParam(format,line,"n");
            int t = format.find(' ');
            options.outputFormat = format.substr(0,t);
            // FIXME. add argument parsing for output formats
            // FIXME add color correction parsing.
            int cRefImg = 0;
            if (getParam(cRefImg, line,"k")) {
                options.colorCorrection = PanoramaOptions::BRIGHTNESS_COLOR;
            } else if (getParam(cRefImg, line,"b")) {
                options.colorCorrection = PanoramaOptions::BRIGHTNESS;
            } else if (getParam(cRefImg, line,"d")) {
                options.colorCorrection = PanoramaOptions::COLOR;
            } else {
                options.colorCorrection = PanoramaOptions::NONE;
            }
            options.colorReferenceImage=cRefImg;
            break;
        }
        case 'm':
        {
            DEBUG_DEBUG("m line: " << line);
            // parse misc options
            int i;
            getParam(i,line,"i");
            options.interpolator = (PanoramaOptions::Interpolator) i;
            getParam(options.gamma,line,"g");
            break;
        }
        case 'i':
        {
            DEBUG_DEBUG("i line: " << line);
            // parse image lines
            // create a new Image
            string file;
            getPTStringParam(file,line,"n");
            DEBUG_DEBUG("filename: " << file);
            // load the image somehow..

            // create lens for this image
            Lens l;
            getParam(l.HFOV, line, "v");
            getParam(l.a, line, "a");
            getParam(l.b, line, "b");
            getParam(l.c, line, "c");
            getParam(l.d, line, "d");
            getParam(l.e, line, "e");
            int t;
            getParam(t, line, "f");
            l.projectionFormat = (Lens::LensProjectionFormat) t;

            lenses.push_back(l);
            unsigned int lnr = lenses.size()-1;
            int width, height;
            getParam(width, line, "w");
            getParam(height, line, "h");

            images.push_back(PanoImage(file,width, height, lnr));
            ImageOptions opts = images.back().getOptions();
            getParam(opts.featherWidth, line, "u");
            images.back().setOptions(opts);

            ImageVariables var;
            readVar(var.roll, line);
            readVar(var.pitch, line);
            readVar(var.yaw, line);

            // FIXME support linking.
            readVar(var.HFOV, line);
            readVar(var.a, line);
            readVar(var.b, line);
            readVar(var.c, line);
            readVar(var.d, line);
            readVar(var.e, line);
            variables.push_back(var);
            state = P_IMAGE;

            // FIXME add lens here.



            break;
        }
        case 'v':
            DEBUG_DEBUG("v line: " << line);
            // FIXME add optimize flags to Panorama and parse it here.
            state = P_OPTIMIZE;
            break;
        case 'c':
        {
            DEBUG_DEBUG("c line: " << line);
            int t;
            // read control points
            ControlPoint point;
            getParam(point.image1Nr, line, "n");
            getParam(point.image2Nr, line, "N");
            getParam(point.x1, line, "x");
            getParam(point.x2, line, "X");
            getParam(point.y1, line, "y");
            getParam(point.y2, line, "Y");
            getParam(t, line, "t");
            point.mode = (ControlPoint::OptimizeMode) t;
            ctrlPoints.push_back(point);
            state = P_CP;
            break;
        }
        default:
            // ignore line..
            break;
        }
    }
    return true;
}

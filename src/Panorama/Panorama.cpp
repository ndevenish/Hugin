// -*- c-basic-offset: 4 -*-

/** @file Panorama.cpp
 *
 *  @brief implementation of Panorama Class
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
#include <fstream>
#include <map>
#include <set>

#include <stdio.h>
#include <math.h>

extern "C" {
#include <libexif/exif-data.h>
#include <libexif/exif-utils.h>
#include <jpeglib.h>
}

#include "Panorama.h"
#include "Process.h"
#include "../utils.h"

using namespace PT;
using namespace std;


/// helper functions for parsing of a script line
bool PT::getPTParam(std::string & output, const std::string & line, const std::string & parameter)
{
    std::string::size_type p;
    if ((p=line.find(std::string(" ") + parameter)) == std::string::npos) {
        DEBUG_ERROR("could not find param " << parameter
                    << " in line: " << line);
        return false;
    }
    p += 2;
    output = line.substr(p, line.find(' ',p));
    return true;
}

template <class T>
bool PT::getParam(T & value, const std::string & line, const std::string & name)
{
    std::string s;
    if (!getPTParam(s, line, name)) {
        return false;
    }
    std::istringstream is(s);
    is >> value;
    return true;
}


bool PT::readVar(Variable & var, const std::string & line)
{
    double v;
    if (getParam(v,line, var.getName())) {
        var.setValue(v);
    } else {
        return false;
    }
    return true;
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


//=========================================================================
//=========================================================================


Panorama::Panorama(PanoramaObserver * o)
    : currentProcess(NO_PROCESS),
      optimizerExe("PTOptimizer"),
      stitcherExe("PTStitcher"),
      PTScriptFile("PT_script.txt"),
      observer(o)
{
    cerr << "Panorama obj created" << endl;
/*
    settings.setPath("dangelo","PanoAssistant");
    readSettings();
    process.setCommunication(QProcess::Stdin|
                             QProcess::Stdout|
                             QProcess::Stderr|
                             QProcess::DupStderr);
    connect(&process, SIGNAL(processExited()), this, SLOT(processExited()));
*/
}



Panorama::~Panorama()
{
    reset();
}

void Panorama::reset()
{
    // delete all images and control points.
    ctrlPoints.clear();
    lenses.clear();
    for (ImagePtrVector::iterator it = images.begin(); it != images.end(); ++it) {
        delete *it;
    }
    images.clear();
    variables.clear();
    changeFinished();
}


#if 0
QDomElement Panorama::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("panorama");
    // serialize global options:
    root.appendChild(options.toXML(doc));

    // serialize image
    QDomElement images_ = doc.createElement("images");
    for (ImageVector::iterator it = images.begin(); it != images.end(); ++it) {
        images_.appendChild(it->toXML(doc));
    }
    root.appendChild(images_);

    // control points
    QDomElement cps = doc.createElement("control_points");
    for (CPVector::iterator it = ctrlPoints.begin(); it != ctrlPoints.end(); ++it) {
        cps.appendChild(it->toXML(doc));
    }
    root.appendChild(cps);

    // lenses
    QDomElement lenses_ = doc.createElement("lenses");
    for (std::vector<Lens>::iterator it = lenses.begin(); it != lenses.end(); ++it) {
        lenses_.appendChild(it->toXML(doc));
    }
    root.appendChild(lenses_);

    return root;
}

void Panorama::setFromXML(const QDomNode & elem)
{
    DEBUG_DEBUG("Panorama::setFromXML");
    clear();

    Q_ASSERT(elem.nodeName() == "panorama");
    // read global options
    QDomNode n = elem.namedItem("output");
    Q_ASSERT(!n.isNull);
    options.setFromXML(n);
    n = elem.namedItem("images");
    Q_ASSERT(!n.isNull);
    n = n.firstChild();
    while( !n.isNull() ) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        Q_ASSERT((!e.isNull()) && e.tagName() == "image" );
        images.push_back(PanoImage(*this, e));
        reportAddedImage(images.size() -1);
        n = n.nextSibling();
    }


    n = elem.namedItem("control_points");
    Q_ASSERT(!n.isNull());
    n = n.firstChild();
    while( !n.isNull() ) {
        QDomElement e = n.toElement();// try to convert the node to an element.
        Q_ASSERT((!e.isNull()) && e.tagName() == "control_point" );
        controlPoints.push_back(ControlPoint(*this, e));
        reportAddedCtrlPoint(controlPoints.size() -1);
        n = n.nextSibling();
    }
}


#endif


std::vector<unsigned int> Panorama::getCtrlPointsForImage(unsigned int imgNr) const
{
    std::vector<unsigned int> result;
    unsigned int i = 0;
    for (CPVector::const_iterator it = ctrlPoints.begin(); it != ctrlPoints.end(); ++it) {
        std::cout << "c n" << it->image1Nr
          << " N" << it->image2Nr
          << " x" << it->x1 << " y" << it->y1
          << " X" << it->x2 << " Y" << it->y2
          << " t" << it->mode << std::endl;
        if ((it->image1Nr == imgNr) || (it->image2Nr == imgNr)) {
            result.push_back(i);
        }
        i++;
    }
    return result;
}

const VariablesVector & Panorama::getVariables() const
{
    return variables;
}

const ImageVariables & Panorama::getVariable(unsigned int imgNr) const
{
    assert(imgNr < images.size());
    return variables[imgNr];
}


void Panorama::updateCtrlPoints(const CPVector & cps)
{
    assert(cps.size() == ctrlPoints.size());
    unsigned int nrp = cps.size();
    for (unsigned int i = 0; i < nrp ; i++) {
        ctrlPoints[i].error = cps[i].error;
    }
}

void Panorama::updateVariables(const VariablesVector & vars)
{
    assert(vars.size() == images.size());
    unsigned int i = 0;
    for (VariablesVector::const_iterator it = vars.begin(); it != vars.end(); ++it) {
        updateVariables(i, *it);
        i++;
    }
}

void Panorama::updateVariables(unsigned int imgNr, const ImageVariables & var)
{
    assert(imgNr < images.size());
    variables[imgNr].updateValues(var);
}

unsigned int Panorama::addImage(const std::string & filename)
{
    // create a lens if we don't have one.
    if (lenses.size() < 1) {
        lenses.push_back(Lens());
    }

    // read lens spec from image, if possible
    // FIXME use a lens database (for example the one from PTLens)
    // FIXME to initialize a,b,c etc.
    Lens l;
    if(l.readEXIF(filename)) {
        lenses.back() = l;
    }
    unsigned int nr = images.size();
    images.push_back(new PanoImage(filename));
    ImageOptions opts = images.back()->getOptions();
    opts.lensNr = 0;
    images.back()->setOptions(opts);
    variables.push_back(ImageVariables());
    updateLens(nr);
    adjustVarLinks();
    return nr;
}

void Panorama::removeImage(unsigned int imgNr)
{
    DEBUG_DEBUG("Panorama::removeImage(" << imgNr << ")");
    assert(imgNr < images.size());

    // remove control points
    CPVector::iterator it = ctrlPoints.begin();
    while (it != ctrlPoints.end()) {
        if ((it->image1Nr == imgNr) || (it->image2Nr == imgNr)) {
            // remove point that refernce to imgNr
            it = ctrlPoints.erase(it);
        } else {
            // correct point references
            if (it->image1Nr > imgNr) it->image1Nr--;
            if (it->image2Nr > imgNr) it->image2Nr--;
            ++it;
        }
    }

    // remove Lens if needed
    bool removeLens = true;
    unsigned int lens = images[imgNr]->getLens();
    unsigned int i = 0;
    for (ImagePtrVector::iterator it = images.begin(); it != images.end(); ++it) {
        if ((*it)->getLens() == lens && imgNr != i) {
            removeLens = false;
        }
        i++;
    }
    if (removeLens) {
        for (ImagePtrVector::iterator it = images.begin(); it != images.end(); ++it) {
            if((*it)->getLens() >= lens) {
                (*it)->setLens((*it)->getLens() - 1);
            }
        }
        lenses.erase(lenses.begin() + lens);
    }

    variables.erase(variables.begin() + imgNr);
    delete images[imgNr];
    images.erase(images.begin() + imgNr);
    adjustVarLinks();
}


unsigned int Panorama::addCtrlPoint(const ControlPoint & point )
{
    unsigned int nr = ctrlPoints.size();
    ctrlPoints.push_back(point);
    return nr;
}

void Panorama::removeCtrlPoint(unsigned int pNr)
{
    assert(pNr < ctrlPoints.size());
    ctrlPoints.erase(ctrlPoints.begin() + pNr);
}


void Panorama::changeControlPoint(unsigned int pNr, const ControlPoint & point)
{
    assert(pNr < ctrlPoints.size());
    ctrlPoints[pNr] = point;
}


bool Panorama::runOptimizer(Process & proc, const OptimizeVector & optvars) const
{
    PanoramaOptions opts;
    return runOptimizer(proc, optvars, opts);
}

bool Panorama::runOptimizer(Process & proc, const OptimizeVector & optvars, const PanoramaOptions & options) const
{
    std::ofstream script(PTScriptFile.c_str());
    printOptimizerScript(script, optvars, options);
    script.close();

#ifdef UNIX
    string cmd = optimizerExe + " " + PTScriptFile;
    return proc.open(cmd.c_str());
#else
    DEBUG_FATAL("program execution not ported to windows yet");
    assert(0);
#endif

}


bool Panorama::runStitcher(Process & proc, const PanoramaOptions & target) const
{
    std::ofstream script(PTScriptFile.c_str());
    printStitcherScript(script, target);
    script.close();

#ifdef UNIX
    string cmd = stitcherExe + string(" -o \"") + target.outfile + "\" " + PTScriptFile;
    return proc.open(cmd.c_str());
#else
    DEBUG_FATAL("program execution not ported to windows yet");
    assert(0);
#endif

}


void Panorama::printOptimizerScript(ostream & o,
                                    const OptimizeVector & optvars,
                                    const PanoramaOptions & output) const
{
    o << "# PTOptimizer script, written by hugin" << endl
      << endl;
    // output options..

    output.printScriptLine(o);

    unsigned int i = 0;
    std::set<unsigned int> usedLenses;
    o << endl
      << "# image lines" << endl;
    for (ImagePtrVector::const_iterator it = images.begin(); it != images.end(); ++it) {
        o << "i w" << (*it)->getWidth() << " h" << (*it)->getHeight()
          <<" f" << lenses[(*it)->getLens()].projectionFormat << " ";
        variables[i].print(o, true);
/*
        if (usedLenses.count((*it)->getLens()) == 0) {
            variables[i].print(o, true);
            usedLenses.insert((*it)->getLens());
        } else {
            variables[i].print(o, false);
        }
*/
        o << " u" << (*it)->getOptions().featherWidth
          << ((*it)->getOptions().morph ? " o" : "")
          << " n\"" << (*it)->getFilename() << "\"" << std::endl;
        i++;
    }

    o << endl << endl
      << "# specify variables that should be optimized"
      << endl;

    i = 0;
    for (OptimizeVector::const_iterator it = optvars.begin(); it != optvars.end(); ++it) {
        (*it).printOptimizeLine(o,i);
        i++;
    }
    o << endl << endl
      << "# control points" << endl;
    for (CPVector::const_iterator it = ctrlPoints.begin(); it != ctrlPoints.end(); ++it) {
        o << "c n" << it->image1Nr
          << " N" << it->image2Nr
          << " x" << it->x1 << " y" << it->y1
          << " X" << it->x2 << " Y" << it->y2
          << " t" << it->mode << std::endl;
    }
    o << endl;
}


void Panorama::printStitcherScript(ostream & o,
                                   const PanoramaOptions & target) const
{
    o << "# PTStitcher script, written by hugin" << endl
      << endl;
    // output options..
    target.printScriptLine(o);
    o << endl
      << "# output image lines" << endl;
    unsigned int i=0;
    for (ImagePtrVector::const_iterator it = images.begin(); it != images.end(); ++it) {

        o << "o w" << (*it)->getWidth() << " h" << (*it)->getHeight()
          <<" f" << lenses[(*it)->getLens()].projectionFormat << " ";
        variables[i].print(o,false);
        o << " u" << (*it)->getOptions().featherWidth << " m" << (*it)->getOptions().ignoreFrameWidth
          << ((*it)->getOptions().morph ? " o" : "")
          << " n\"" << (*it)->getFilename() << "\"" << std::endl;
        i++;
    }
    o << endl;
}

void Panorama::readOptimizerOutput(VariablesVector & vars, CPVector & ctrlPoints) const
{
    std::ifstream script(PTScriptFile.c_str());
    if (!script.good()) {
        DEBUG_ERROR("Could not open " << PTScriptFile);
        // throw execption
        return;
    }
    parseOptimizerScript(script, vars, ctrlPoints);
}

void Panorama::parseOptimizerScript(istream & i, VariablesVector & imgVars, CPVector & CPs) const
{
    // 0 = read output (image lines), 1 = read control point distances
    int state = 0;
    string line;
    uint lineNr = 0;
    VariablesVector::iterator varIt = imgVars.begin();
    CPVector::iterator pointIt = CPs.begin();

    while (!i.eof()) {
        std::getline(i, line);
        lineNr++;
        switch (state) {
        case 0:
        {
            // we are reading the output lines:
            // o f3 r0 p0 y0 v89.2582 a-0.027803 b0.059851 c-0.073115 d10.542470 e16.121145 u10 -buf
            if (line.compare("# Control Points: Distance between desired and fitted Position") == 0) {
                // switch to reading the control point distance
                assert(varIt == imgVars.end());
                state = 1;
                break;
            }
            if (line[0] != 'o') continue;
            assert(varIt != imgVars.end());
            // read variables
            readVar(varIt->roll, line);
            readVar(varIt->pitch, line);
            readVar(varIt->yaw, line);

            readVar(varIt->HFOV, line);
            readVar(varIt->a, line);
            readVar(varIt->b, line);
            readVar(varIt->c, line);

            if (!readVar(varIt->d, line)) {
                if (varIt->d.isLinked()) {
                    // set value from link
                    DEBUG_NOTICE("setting from linked variables")
                    varIt->d.setValue(imgVars[varIt->d.getLink()].d.getValue());
                } else {
                    // FIXME throw exception
                    DEBUG_ERROR("variable d not linked, keeping old value");
                }
            }

            if (!readVar(varIt->e, line)) {
                if (varIt->e.isLinked()) {
                    // set value from link
                    varIt->e.setValue(imgVars[varIt->e.getLink()].e.getValue());
                    DEBUG_NOTICE("setting from linked variables")
                } else {
                    // FIXME throw exception
                    DEBUG_ERROR("variable e not linked, keeping old value");
                }
            }

            varIt++;
            break;
        }
        case 1:
        {
            // read ctrl point distances:
            // # Control Point No 0:  0.428994
            if (line[0] == 'C') {
                assert(pointIt == CPs.end());
                state = 2;
                break;
            }
            if (line.find("# Control Point No") != 0) continue;
            string::size_type p;
            if ((p=line.find(':')) == string::npos) assert(0);
            p++;
            (*pointIt).error = atof(line.substr(p, line.find(' ',p)).c_str());
            pointIt++;
            break;
        }
        default:
            // ignore line..
            break;
        }
    }
}


void Panorama::changeFinished()
{
    DEBUG_DEBUG("Panorama changed");
    if (observer) {
        observer->panoramaChanged();
    }
}


const Lens & Panorama::getLens(unsigned int lensNr) const
{
    assert(lensNr < lenses.size());
    return lenses[lensNr];
}


void Panorama::updateLens(unsigned int lensNr, const Lens & lens)
{
    assert(lensNr < lenses.size());
    lenses[lensNr] = lens;
    for ( unsigned int i = 0; i < variables.size(); ++i) {
        if(images[i]->getLens() == lensNr) {
            // set variables
            updateLens(i);
        }
    }
}


void Panorama::setLens(unsigned int imgNr, unsigned int lensNr)
{
    assert(lensNr < lenses.size());
    assert(imgNr < images.size());
    images[imgNr]->setLens(lensNr);
    updateLens(imgNr);
    adjustVarLinks();
}


void Panorama::adjustVarLinks()
{
    DEBUG_DEBUG("Panorama::adjustVarLinks()");
    unsigned int image = 0;
    std::map<unsigned int,unsigned int> usedLenses;
    for (ImagePtrVector::iterator it = images.begin(); it != images.end(); ++it) {
        unsigned int lens = (*it)->getLens();
        if (usedLenses.count(lens) == 1) {
            unsigned int refImg = usedLenses[lens];
            switch ((*it)->getOptions().source) {
                case ImageOptions::DIGITAL_CAMERA:
                variables[image].a.link(refImg);
                variables[image].b.link(refImg);
                variables[image].c.link(refImg);
                variables[image].d.link(refImg);
                variables[image].e.link(refImg);
                break;
            case ImageOptions::SCANNER:
                variables[image].a.link(refImg);
                variables[image].b.link(refImg);
                variables[image].c.link(refImg);
                variables[image].d.unlink();
                variables[image].e.unlink();
            }
        } else {
            variables[image].a.unlink();
            variables[image].b.unlink();
            variables[image].c.unlink();
            variables[image].d.unlink();
            variables[image].e.unlink();
            usedLenses[lens]=image;
        }
        image++;
    }
}

unsigned int Panorama::addLens(const Lens & lens)
{
    lenses.push_back(lens);
    return lenses.size() - 1;
}

void Panorama::updateLens(unsigned int imgNr)
{
    unsigned int lensNr = images[imgNr]->getLens();
    variables[imgNr].HFOV.setValue(lenses[lensNr].HFOV);
    variables[imgNr].a.setValue(lenses[lensNr].a);
    variables[imgNr].b.setValue(lenses[lensNr].b);
    variables[imgNr].c.setValue(lenses[lensNr].c);
    variables[imgNr].d.setValue(lenses[lensNr].d);
    variables[imgNr].e.setValue(lenses[lensNr].e);
}

void Panorama::setMemento(PanoramaMemento & state)
{
    DEBUG_ERROR("setMemento() not implemented yet");
}

PanoramaMemento Panorama::getMemento(void) const
{
    DEBUG_ERROR("getMemento() not implemented yet");
    return PanoramaMemento();
}


void Panorama::setOptions(const PanoramaOptions & opt)
{
    options = opt;
}

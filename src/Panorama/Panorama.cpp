// -*- c-basic-offset: 4 -*-
/** @file Panorama.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <fstream>
#include <cassert>
#include <qpixmap.h>

#include "Panorama.h"

/**
 * @short
 * @author Pablo d'Angelo <pablo@mathematik.uni-ulm.de>
*/

using namespace PT;
using namespace std;


QString PT::getAttrib(QDomNamedNodeMap map, QString name)
{
    return map.namedItem(name).nodeValue();
}


//=========================================================================
//=========================================================================


ControlPoint::ControlPoint(Panorama & pano, const QDomNode & node)
{
    setFromXML(node,pano);
}

void ControlPoint::printScriptLine(std::ostream & o) const
{
    o << "c n" << image1->getNr() << " N" << image2->getNr()
      << " x" << x1 << " y" << y1
      << " X" << x2 << " Y" << y2
      << " t" << mode << std::endl;
}

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
    qDebug("ControlPoint::setFromXML");
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


//=========================================================================
//=========================================================================


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
    qDebug("PanoramaOptions::setFromXML");
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


Panorama::Panorama()
    : currentProcess(NO_PROCESS),
      PTScriptFile("PT_script.txt")
{
    cerr << "Panorama obj created" << endl;
    settings.setPath("dangelo","PanoAssistant");
    readSettings();
    process.setCommunication(QProcess::Stdin|
                             QProcess::Stdout|
                             QProcess::Stderr|
                             QProcess::DupStderr);
    connect(&process, SIGNAL(processExited()), this, SLOT(processExited()));
}

Panorama::~Panorama()
{
    writeSettings();
    for (std::vector<ControlPoint*>::iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        delete (*it);
    }
    for (std::vector<PanoImage*>::iterator it = images.begin(); it != images.end(); ++it) {
        delete (*it);
    }
}

void Panorama::readSettings()
{
    stitcherExe = settings.readEntry( "/PanoramaTools/PTStitcher", "PTStitcher" );
    optimizerExe = settings.readEntry( "/PanoramaTools/PTOptimizer", "PTOptimizer" );
}

void Panorama::writeSettings()
{
    settings.writeEntry( "/PanoramaTools/PTStitcher", stitcherExe );
    settings.writeEntry( "/PanoramaTools/PTOptimizer", optimizerExe );
}

QDomElement Panorama::toXML(QDomDocument & doc)
{
    QDomElement root = doc.createElement("panorama");
    // serialize global options:
    root.appendChild(options.toXML(doc));
    // serialize image
    QDomElement images_ = doc.createElement("images");
    for (std::vector<PanoImage *>::iterator it = images.begin(); it != images.end(); ++it) {
        cerr << "added img" << (*it)->getNr() << endl;
        images_.appendChild((*it)->toXML(doc));
    }
    root.appendChild(images_);
    // control points
    QDomElement cps = doc.createElement("control_points");
    for (std::vector<ControlPoint *>::iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        cerr << "added control point" << endl;
        cps.appendChild((*it)->toXML(doc));
    }
    root.appendChild(cps);
    return root;
}

void Panorama::setFromXML(const QDomNode & elem)
{
    qDebug("Panorama::setFromXML");
    // delete all images and control points.
    for (std::vector<ControlPoint*>::iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        removeControlPoint(*it);
    }
    for (std::vector<PanoImage*>::iterator it = images.begin(); it != images.end(); ++it) {
        removeImage(*it);
    }

    Q_ASSERT(elem.nodeName() == "panorama");
    // read global options
    QDomNode opt = elem.namedItem("output");
    options.setFromXML(opt);
    QDomNode n = elem.namedItem("images");
    n = n.firstChild();
    while( !n.isNull() ) {
        QDomElement e = n.toElement(); // try to convert the node to an element.
        if( (!e.isNull()) && e.tagName() == "image" ) {
            images.push_back(new PanoImage(*this, e));
            reportAddedImage(images.size() -1);
        }
        n = n.nextSibling();
    }


    n = elem.namedItem("control_points");
    n = n.firstChild();
    while( !n.isNull() ) {
        QDomElement e = n.toElement();// try to convert the node to an element.
        if( (!e.isNull()) && e.tagName() == "control_point" ) {
            controlPoints.push_back(new ControlPoint(*this, e));
            reportAddedCtrlPoint(controlPoints.size() -1);
        }
        n = n.nextSibling();
    }
}



PanoImage * Panorama::addImage(const QString & filename)
{
    unsigned int nr = images.size();
    images.push_back(new PanoImage(*this, filename));
    imageData.resize(images.size());
    if (commonLens) {
        // if common lens settings are used
        images.back()->updateLens(images[0]->getLens());
    }
    reportAddedImage(nr);
    return images.back();;
}

void Panorama::removeImage(PanoImage * img)
{
//  std::assert(imgNr < images.size());

    qDebug("Panorama::removeImage: %s\n",img->getFilename().ascii());
    // remove from vector
    vector<PanoImage *>::iterator it = find(images.begin(),
                                            images.end(),
                                            img);
    if (it != images.end()) {

        qDebug("calling reportRemovedImage");
        reportRemovedImage((*it)->getNr());
        // remove control points
        std::vector<ControlPoint *> points = getCtrlPointsForImage(*it);
        for (std::vector<ControlPoint*>::iterator cit = points.begin();
             cit != points.end(); ++cit) {
            removeControlPoint(*cit);
        }
        // move images in vector.
        // free memory
        qDebug("calling deleting image");
        delete (*it);
        it = images.erase(it);
    }
}


void Panorama::optimize()
{
    if (process.isRunning()) {
        qDebug("optimizer still running.. doing nothing");
        return;
    }

    std::ofstream script(PTScriptFile);
    printOptimizerScript(script);
    script.close();

    process.clearArguments();
    process.addArgument(optimizerExe);
    process.addArgument(PTScriptFile);
    currentProcess = OPTIMIZER;
    if ( ! process.launch("")) {
        cerr << "Could not execute: " << optimizerExe << " " << PTScriptFile << endl;
    }
}


void Panorama::stitch(const PanoramaOptions & prop)
{
    if (process.isRunning()) {
        qDebug("optimizer still running.. doing nothing");
        return;
    }

    std::ofstream script(PTScriptFile);
    printStitcherScript(script);
    script.close();

    process.clearArguments();
    process.addArgument(stitcherExe);
    process.addArgument("-o");
    process.addArgument(prop.outfile);
    process.addArgument(PTScriptFile);
    currentProcess = STITCHER;
    if ( ! process.launch("")) {
        cerr << "Could not execute " << stitcherExe << endl;
    }
}



void Panorama::processExited()
{
    if (!process.normalExit()) {
        cerr << "process has exited with error code : " << process.exitStatus()
             << endl;
        while (process.canReadLineStdout()) {
            cerr << process.readLineStdout() << endl;
        }
    } else {
        switch (currentProcess) {
        case OPTIMIZER:
        {
            cerr << "optimizer output:" << endl;
            while (process.canReadLineStdout()) {
                cerr << process.readLineStdout() << endl;
            }
            cerr << "XXXX parse optimizer output" << endl;

            std::ifstream script(PTScriptFile);
            readOptimizerScript(script);
            script.close();

            reportChange();
            break;
        }
        case STITCHER:
            cerr << "stitcher output:" << endl;
            while (process.canReadLineStdout()) {
                cerr << process.readLineStdout() << endl;
            }
            break;
        case NO_PROCESS:
            Q_ASSERT("received processExited(), but no process running.. this shouldn't happen");
            break;
        }
    }
    currentProcess = NO_PROCESS;
}

void Panorama::printOptimizerScript(ostream & o)
{
    o << "# PTOptimizer script, written by hugin" << endl
      << endl;
    // output options..
    options.printScriptLine(o);
    o << endl
      << "# image lines" << endl;
    for (std::vector<PanoImage *>::iterator it = images.begin(); it != images.end(); ++it) {
        (*it)->printImageLine(o);
    }
    o << endl
      << "# specify variables that should be optimized"
      << endl;

    for (std::vector<PanoImage *>::iterator it = images.begin(); it != images.end(); ++it) {
        (*it)->printOptimizeLine(o);
    }
    o << endl;
    for (std::vector<ControlPoint*>::iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        (*it)->printScriptLine(o);
    }
    o << endl;
}

void Panorama::printStitcherScript(ostream & o)
{
    o << "# PTStitcher script, written by hugin" << endl
      << endl;
    // output options..
    options.printScriptLine(o);
    o << endl
      << "# output image lines" << endl;
    for (std::vector<PanoImage *>::iterator it = images.begin(); it != images.end(); ++it) {
        (*it)->printStitchImageLine(o);
    }
    o << endl;
}

void Panorama::readOptimizerScript(istream & i)
{
    cerr << "FIXME: readOptimizer: setting variables directly" << i.good() << endl;
    // FIXME urgh.. we apply the optimizer results to our data..
    // FIXME should copy results to a buffer, so that they
    // FIXME can be applied later
    // FIXME get rid of this fixed buffer.
    char buf[300];
    int state = 0;
    QString line;
    uint lineNr = 0;
    std::vector<PanoImage *>::iterator imgIt = images.begin();
    std::vector<ControlPoint *>::iterator pointIt = controlPoints.begin();

    while (!i.eof()) {
        i.getline(buf,299);
        line = buf;
        lineNr++;
        switch (state) {
        case 0:
        {
            // we are reading the output lines.
            // o f3 r0 p0 y0 v89.2582 a-0.027803 b0.059851 c-0.073115 d10.542470 e16.121145 u10 -buf
            if (line.startsWith("# Control Points: Distance between desired and fitted Position")) {
                // switch to reading the control point distance
                Q_ASSERT(imgIt == images.end());
                state = 1;
                break;
            }
            if (line[0] != 'o') continue;
            Q_ASSERT(imgIt != images.end());
            ImagePosition pos;
            int p;
            // roll
            if ((p=line.find('r')) == -1) Q_ASSERT(0);
            pos.roll = line.mid(p+1).toDouble();
            //pitch
            if ((p=line.find('p')) == -1) Q_ASSERT(0);
            pos.pitch = line.mid(p+1).toDouble();
            // yaw
            if ((p=line.find('y')) == -1) Q_ASSERT(0);
            pos.yaw = line.mid(p+1).toDouble();

            (*imgIt)->setPosition(pos);

            LensSettings lens = (*imgIt)->getLens();
            if ((p=line.find('a')) == -1) Q_ASSERT(0);
            lens.a = line.mid(p+1).toDouble();
            if ((p=line.find('b')) == -1) Q_ASSERT(0);
            lens.b = line.mid(p+1).toDouble();
            if ((p=line.find('c')) == -1) Q_ASSERT(0);
            lens.c = line.mid(p+1).toDouble();
            if ((p=line.find('d')) == -1) Q_ASSERT(0);
            lens.d = line.mid(p+1).toDouble();
            if ((p=line.find('e')) == -1) Q_ASSERT(0);
            lens.e = line.mid(p+1).toDouble();

            (*imgIt)->updateLens(lens);

            imgIt++;
            break;
        }
        case 1:
        {
            // read ctrl point distances.
            // # Control Point No 0:  0.428994
            if (line[0] == 'C') {
                Q_ASSERT(pointIt == controlPoints.end());
                state = 2;
                break;
            }
            if (!line.startsWith("# Control Point No")) continue;
            int p;
            if ((p=line.find(':')) == -1) Q_ASSERT(0);
            bool ok;
            (*pointIt)->error = line.mid(p+1).toDouble(&ok);
            Q_ASSERT(ok);

            pointIt++;
            break;
        }
        default:
            // ignore line..
            break;
        }
    }
}

ControlPoint * Panorama::addControlPoint(const ControlPoint & point)
{
    controlPoints.push_back(new ControlPoint(point));
    reportAddedCtrlPoint(controlPoints.size() - 1);
    return controlPoints.back();
}

void Panorama::changeControlPoint(unsigned int ctrlPointNr, ControlPoint point)
{
    ControlPoint * cp = controlPoints[ctrlPointNr];
    cp->x1 = point.x1;
    cp->y1 = point.y1;
    cp->x2 = point.x2;
    cp->y2 = point.y2;
    cp->error = point.error;
    cp->mode = point.mode;
    reportChange();
}


void Panorama::removeControlPoint(ControlPoint * point)
{
    vector<ControlPoint *>::iterator it = find(controlPoints.begin(),
                                               controlPoints.end(),
                                               point);
    if (it != controlPoints.end()) {
        reportRemovedCtrlPoint(it - controlPoints.begin());
        delete (*it);
        controlPoints.erase(it);
    }
}


/** return control points for a given image */
std::vector<ControlPoint *> Panorama::getCtrlPointsForImage(PanoImage * img) const
{
    std::vector<ControlPoint *> result;
    for (std::vector<ControlPoint*>::const_iterator it = controlPoints.begin(); it != controlPoints.end(); ++it) {
        if (((*it)->image1 == img) || ((*it)->image2 == img) ) {
            result.push_back(*it);
        }
    }
    return result;
}


void Panorama::setCommonLens(bool common)
{
    commonLens = common;
    if (commonLens && images.size() > 0) {
        for (std::vector<PanoImage *>::iterator it = images.begin(); it != images.end(); ++it) {
            (*it)->updateLens(images[0]->getLens());
        }
    }
    reportChange();
}


void Panorama::updateLens(const LensSettings & l, PanoImage * img)
{
    if (commonLens) {
        for (std::vector<PanoImage *>::iterator it = images.begin(); it != images.end(); ++it) {
            (*it)->updateLens(l);
        }
    } else {
        img->updateLens(l);
    }
    reportChange();
}


void Panorama::setOptions(const PanoramaOptions & opt)
{
    options = opt;
    reportChange();
}


void Panorama::reportChange()
{
    dirty = true;
    emit(stateChanged());
}
void Panorama::reportChangedImage(unsigned int img)
{
    emit(imageChanged(img));
}


void Panorama::reportAddedImage(unsigned int img)
{
    qDebug("Image %d added",img);
    emit(imageAdded(img));
}


void Panorama::reportRemovedImage(unsigned int img)
{
    qDebug("Image %d removed",img);
    emit(imageRemoved(img));
}


void Panorama::reportAddedCtrlPoint(unsigned int point)
{
    qDebug("Control Point %d added", point);
    emit(ctrlPointAdded(point));
}


void Panorama::reportRemovedCtrlPoint(unsigned int point)
{
    qDebug("Control Point %d removed", point);
    emit(ctrlPointRemoved(point));
}

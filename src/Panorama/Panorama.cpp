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

#include "PT/Panorama.h"
#include "PT/Process.h"
#include "common/utils.h"

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
    state.ctrlPoints.clear();
    state.lenses.clear();
    for (ImagePtrVector::iterator it = state.images.begin(); it != state.images.end(); ++it) {
        delete *it;
    }
    state.images.clear();
    state.variables.clear();
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
    for (CPVector::iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
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
    for (CPVector::const_iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
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
    return state.variables;
}

const ImageVariables & Panorama::getVariable(unsigned int imgNr) const
{
    assert(imgNr < state.images.size());
    return state.variables[imgNr];
}


void Panorama::updateCtrlPoints(const CPVector & cps)
{
    assert(cps.size() == state.ctrlPoints.size());
    unsigned int nrp = cps.size();
    for (unsigned int i = 0; i < nrp ; i++) {
        state.ctrlPoints[i].error = cps[i].error;
    }
}

void Panorama::updateVariables(const VariablesVector & vars)
{
    assert(vars.size() == state.images.size());
    unsigned int i = 0;
    for (VariablesVector::const_iterator it = vars.begin(); it != vars.end(); ++it) {
        updateVariables(i, *it);
        i++;
    }
}

void Panorama::updateVariables(unsigned int imgNr, const ImageVariables & var)
{
    assert(imgNr < state.images.size());
    state.variables[imgNr].updateValues(var);
}

unsigned int Panorama::addImage(const std::string & filename)
{
    // create a lens if we don't have one.
    if (state.lenses.size() < 1) {
        state.lenses.push_back(Lens());
    }

    // read lens spec from image, if possible
    // FIXME use a lens database (for example the one from PTLens)
    // FIXME to initialize a,b,c etc.
    Lens l;
    if(l.readEXIF(filename)) {
        state.lenses.back() = l;
    }
    unsigned int nr = state.images.size();
    state.images.push_back(new PanoImage(filename));
    ImageOptions opts = state.images.back()->getOptions();
    opts.lensNr = 0;
    state.images.back()->setOptions(opts);
    state.variables.push_back(ImageVariables());
    updateLens(nr);
    adjustVarLinks();
    return nr;
}

void Panorama::removeImage(unsigned int imgNr)
{
    DEBUG_DEBUG("Panorama::removeImage(" << imgNr << ")");
    assert(imgNr < state.images.size());

    // remove control points
    CPVector::iterator it = state.ctrlPoints.begin();
    while (it != state.ctrlPoints.end()) {
        if ((it->image1Nr == imgNr) || (it->image2Nr == imgNr)) {
            // remove point that refernce to imgNr
            it = state.ctrlPoints.erase(it);
        } else {
            // correct point references
            if (it->image1Nr > imgNr) it->image1Nr--;
            if (it->image2Nr > imgNr) it->image2Nr--;
            ++it;
        }
    }

    // remove Lens if needed
    bool removeLens = true;
    unsigned int lens = state.images[imgNr]->getLens();
    unsigned int i = 0;
    for (ImagePtrVector::iterator it = state.images.begin(); it != state.images.end(); ++it) {
        if ((*it)->getLens() == lens && imgNr != i) {
            removeLens = false;
        }
        i++;
    }
    if (removeLens) {
        for (ImagePtrVector::iterator it = state.images.begin(); it != state.images.end(); ++it) {
            if((*it)->getLens() >= lens) {
                (*it)->setLens((*it)->getLens() - 1);
            }
        }
        state.lenses.erase(state.lenses.begin() + lens);
    }

    state.variables.erase(state.variables.begin() + imgNr);
    delete state.images[imgNr];
    state.images.erase(state.images.begin() + imgNr);
    adjustVarLinks();
}


unsigned int Panorama::addCtrlPoint(const ControlPoint & point )
{
    unsigned int nr = state.ctrlPoints.size();
    state.ctrlPoints.push_back(point);
    return nr;
}

void Panorama::removeCtrlPoint(unsigned int pNr)
{
    assert(pNr < state.ctrlPoints.size());
    state.ctrlPoints.erase(state.ctrlPoints.begin() + pNr);
}


void Panorama::changeControlPoint(unsigned int pNr, const ControlPoint & point)
{
    assert(pNr < state.ctrlPoints.size());
    state.ctrlPoints[pNr] = point;
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
    for (ImagePtrVector::const_iterator it = state.images.begin(); it != state.images.end(); ++it) {
        o << "i w" << (*it)->getWidth() << " h" << (*it)->getHeight()
          <<" f" << state.lenses[(*it)->getLens()].projectionFormat << " ";
        state.variables[i].print(o, true);
/*
        if (usedLenses.count((*it)->getLens()) == 0) {
            state.variables[i].print(o, true);
            usedLenses.insert((*it)->getLens());
        } else {
            state.variables[i].print(o, false);
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
    for (CPVector::const_iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
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
    for (ImagePtrVector::const_iterator it = state.images.begin(); it != state.images.end(); ++it) {

        o << "o w" << (*it)->getWidth() << " h" << (*it)->getHeight()
          <<" f" << state.lenses[(*it)->getLens()].projectionFormat << " ";
        state.variables[i].print(o,false);
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
    assert(lensNr < state.lenses.size());
    return state.lenses[lensNr];
}


void Panorama::updateLens(unsigned int lensNr, const Lens & lens)
{
    assert(lensNr < state.lenses.size());
    state.lenses[lensNr] = lens;
    for ( unsigned int i = 0; i < state.variables.size(); ++i) {
        if(state.images[i]->getLens() == lensNr) {
            // set variables
            updateLens(i);
        }
    }
}


void Panorama::setLens(unsigned int imgNr, unsigned int lensNr)
{
    assert(lensNr < state.lenses.size());
    assert(imgNr < state.images.size());
    state.images[imgNr]->setLens(lensNr);
    updateLens(imgNr);
    adjustVarLinks();
}


void Panorama::adjustVarLinks()
{
    DEBUG_DEBUG("Panorama::adjustVarLinks()");
    unsigned int image = 0;
    std::map<unsigned int,unsigned int> usedLenses;
    for (ImagePtrVector::iterator it = state.images.begin(); it != state.images.end(); ++it) {
        unsigned int lens = (*it)->getLens();
        if (usedLenses.count(lens) == 1) {
            unsigned int refImg = usedLenses[lens];
            switch ((*it)->getOptions().source) {
                case ImageOptions::DIGITAL_CAMERA:
                state.variables[image].a.link(refImg);
                state.variables[image].b.link(refImg);
                state.variables[image].c.link(refImg);
                state.variables[image].d.link(refImg);
                state.variables[image].e.link(refImg);
                break;
            case ImageOptions::SCANNER:
                state.variables[image].a.link(refImg);
                state.variables[image].b.link(refImg);
                state.variables[image].c.link(refImg);
                state.variables[image].d.unlink();
                state.variables[image].e.unlink();
            }
        } else {
            state.variables[image].a.unlink();
            state.variables[image].b.unlink();
            state.variables[image].c.unlink();
            state.variables[image].d.unlink();
            state.variables[image].e.unlink();
            usedLenses[lens]=image;
        }
        image++;
    }
}

unsigned int Panorama::addLens(const Lens & lens)
{
    state.lenses.push_back(lens);
    return state.lenses.size() - 1;
}

void Panorama::updateLens(unsigned int imgNr)
{
    unsigned int lensNr = state.images[imgNr]->getLens();
    state.variables[imgNr].HFOV.setValue(state.lenses[lensNr].HFOV);
    state.variables[imgNr].a.setValue(state.lenses[lensNr].a);
    state.variables[imgNr].b.setValue(state.lenses[lensNr].b);
    state.variables[imgNr].c.setValue(state.lenses[lensNr].c);
    state.variables[imgNr].d.setValue(state.lenses[lensNr].d);
    state.variables[imgNr].e.setValue(state.lenses[lensNr].e);
}

void Panorama::setMemento(PanoramaMemento & state)
{
    DEBUG_ERROR("setMemento() not implemented yet");
}

PanoramaMemento Panorama::getMemento(void) const
{
    DEBUG_ERROR("getMemento() not implemented yet");
    return PanoramaMemento(state);
}


void Panorama::setOptions(const PanoramaOptions & opt)
{
    state.options = opt;
}

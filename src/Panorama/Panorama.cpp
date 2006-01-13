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

#include <config.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <locale.h>

#include <stdio.h>
#include <math.h>
#include <limits.h>

#include <vigra/impex.hxx>

#include <common/stl_utils.h>

#include <PT/Panorama.h>
#include <PT/PanoToolsInterface.h>

#include <PT/RemappedPanoImage.h>

//#include "panoinc.h"

using namespace PT;
using namespace std;
using namespace vigra;
using namespace utils;

//const Map::data_type & map_get(const Map &m, const Map::key_type & key)
const Variable & const_map_get(const VariableMap &m, const string & key)
{
    VariableMap::const_iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

// Map::data_type & map_get( Map &m,  Map::key_type & key)
Variable & map_get( VariableMap &m, const std::string & key)
{
    VariableMap::iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

// Map::data_type & map_get( Map &m,  Map::key_type & key)
PT::LensVariable & map_get(PT::LensVarMap &m, const std::string & key)
{
    LensVarMap::iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}

// Map::data_type & map_get( Map &m,  Map::key_type & key)
const PT::LensVariable & const_map_get(const PT::LensVarMap &m, const std::string & key)
{
    LensVarMap::const_iterator it = m.find(key);
    if (it != m.end()) {
        return (*it).second;
    } else {
        DEBUG_WARN("could not find " << key);
        throw std::out_of_range("No such element in vector");
    }
}


/// helper functions for parsing of a script line
bool PT::getPTParam(std::string & output, const std::string & line, const std::string & parameter)
{
    std::string::size_type p;
    if ((p=line.find(std::string(" ") + parameter)) == std::string::npos) {
        DEBUG_INFO("could not find param " << parameter
                    << " in line: " << line);
        return false;
    }
    p += parameter.length() + 1;
    std::string::size_type p2 = line.find(' ',p);
    output = line.substr(p, p2-p);
//    DEBUG_DEBUG("string idex: " << p <<"," << p2 << "  string: \"" << output << "\"");
    return true;
}

bool PT::getPTStringParam(std::string & output, const std::string & line, const std::string & parameter)
{
    std::string::size_type p;
    if ((p=line.find(std::string(" ") + parameter + "\"")) == std::string::npos) {
        DEBUG_INFO("could not find string param " << parameter
                    << " in line: " << line);
        return false;
    }
    p += parameter.length() + 2;
    std::string::size_type e = line.find("\"",p);
    DEBUG_DEBUG("p:" << p << " e:" << e);
    output = line.substr(p,e-p);
    DEBUG_DEBUG("output: ##" << output << "##");
    return true;
}

bool PT::getPTStringParamColon(std::string & output, const std::string & line, const std::string & parameter)
{
    std::string::size_type p;
    if ((p=line.find(std::string(" ") + parameter + ":")) == std::string::npos) {
        DEBUG_INFO("could not find string param " << parameter
                    << " in line: " << line);
        return false;
    }
    p += parameter.length() + 2;
    std::string::size_type e = line.find(" ",p);
    DEBUG_DEBUG("p:" << p << " e:" << e);
    output = line.substr(p,e-p);
    DEBUG_DEBUG("output: ##" << output << "##");
    return true;
}

bool PT::getDoubleParam(double & d, const std::string & line, const std::string & name)
{
    std::string s;
    if (!getPTParam(s, line, name)) {
        return false;
    }
    return stringToDouble(s, d);
}

bool PT::getPTDoubleParam(double & value, int & link,
                      const std::string & line, const std::string & var)
{
    string val;
    if (getPTParam(val,line, var)) {
        DEBUG_ASSERT(line.size() > 0);
        DEBUG_DEBUG(var << ":" <<val);
        if (val[0] == '=') {
            link = utils::lexical_cast<int>(val.substr(1));
        } else {
            link = -1;
            if (!stringToDouble(val, value)) {
                return false;
            }
        }
    } else {
        return false;
    }
    return true;
}

bool PT::readVar(Variable & var, int & link, const std::string & line)
{
    string val;
    if (getPTParam(val,line, var.getName())) {
        DEBUG_ASSERT(line.size() > 0);
        DEBUG_DEBUG(var.getName() << ":" <<val);
        if (val[0] == '=') {
            link = utils::lexical_cast<int>(val.substr(1));
        } else {
            link = -1;
            double dest = 0;
            if (!stringToDouble(val, dest)) {
                return false;
            }
            var.setValue(dest);
        }
    } else {
        return false;
    }
    return true;
}

//=========================================================================
//=========================================================================


Panorama::Panorama()
    : currentProcess(NO_PROCESS),
      optimizerExe("PTOptimizer"),
      stitcherExe("PTStitcher"),
      PTScriptFile("PT_script.txt"),
      m_forceImagesUpdate(false),
      dirty(false)
{
    // init map with ptoptimizer variables.
    m_ptoptimizerVarNames.insert("a");
    m_ptoptimizerVarNames.insert("b");
    m_ptoptimizerVarNames.insert("c");
    m_ptoptimizerVarNames.insert("d");
    m_ptoptimizerVarNames.insert("e");
    m_ptoptimizerVarNames.insert("g");
    m_ptoptimizerVarNames.insert("t");
    m_ptoptimizerVarNames.insert("v");
    m_ptoptimizerVarNames.insert("r");
    m_ptoptimizerVarNames.insert("p");
    m_ptoptimizerVarNames.insert("y");

    cerr << "Panorama obj created" << std::endl;
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
    DEBUG_TRACE("dtor");
    reset();
    changeFinished();
    DEBUG_TRACE("dtor about to finish");
}


void Panorama::reset()
{
    //
    // imageChanged(0);
    // delete all images and control points.
    state.ctrlPoints.clear();
    state.lenses.clear();
    state.images.clear();
    state.variables.clear();
    state.options.reset();
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
        if ((it->image1Nr == imgNr) || (it->image2Nr == imgNr)) {
            result.push_back(i);
        }
        i++;
    }
    return result;
}

const VariableMapVector & Panorama::getVariables() const
{
    return state.variables;
}

const VariableMap & Panorama::getImageVariables(unsigned int imgNr) const
{
    assert(imgNr < state.images.size());
    return state.variables[imgNr];
}


FDiff2D Panorama::calcFOV() const
{
    Size2D panoSize(360,180);

    // remap into minature pano.
    PanoramaOptions opts;
    opts.setHFOV(360);
    opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
    opts.setWidth(360);
    opts.setHeight(180);

    // remap image
    vigra::BImage panoAlpha(panoSize);
    RemappedPanoImage<vigra::BImage, vigra::BImage> remapped;
    UIntSet activeImgs = getActiveImages();
    for (UIntSet::iterator it = activeImgs.begin(); it != activeImgs.end(); ++it) {
//    for (unsigned int imgNr=0; imgNr < getNrOfImages(); imgNr++) {
        remapped.setPanoImage(*this, *it, opts);
        // calculate alpha channel
        remapped.calcAlpha(*this, opts, *it);
        // copy into global alpha channel.
        vigra::copyImageIf(vigra_ext::applyRect(remapped.boundingBox(),
                                              vigra_ext::srcMaskRange(remapped)),
                           vigra_ext::applyRect(remapped.boundingBox(),
                                              vigra_ext::srcMask(remapped)),
                           vigra_ext::applyRect(remapped.boundingBox(),
                                              destImage(panoAlpha)));
    }
//    vigra::ImageExportInfo imge("c:/hugin_calcfov_alpha.png");
//    exportImage(vigra::srcImageRange(panoAlpha), imge);

    // get field of view
    FDiff2D ul,lr;
    ul.x = DBL_MAX;
    ul.y = DBL_MAX;
    lr.x = -DBL_MAX;
    lr.y = -DBL_MAX;
    for (int v=0; v< 180; v++) {
        for (int h=0; h < 360; h++) {
            if (panoAlpha(h,v)) {
                // pixel is valid
                if ( ul.x > h ) {
                    ul.x = h;
                }
                if ( ul.y > v ) {
                    ul.y = v;
                }
                if ( lr.x < h) {
                    lr.x = h;
                }
                if ( lr.y < v) {
                    lr.y = v;
                }
            }
        }
    }

    ul.x = ul.x - 180;
    ul.y = ul.y - 90;
    lr.x = lr.x - 180;
    lr.y = lr.y - 90;
    FDiff2D fov (2*max(fabs(ul.x), fabs(lr.x)), 2*max(fabs(ul.y), fabs(lr.y)));
    return fov;
}

void Panorama::centerHorizontically()
{
    Size2D panoSize(360,180);

    // remap into minature pano.
    PanoramaOptions opts;
    opts.setHFOV(360);
    opts.setProjection(PanoramaOptions::EQUIRECTANGULAR);
    opts.setWidth(360);
    opts.setHeight(180);

    // remap image
    vigra::BImage panoAlpha(panoSize);
    RemappedPanoImage<vigra::BImage, vigra::BImage> remapped;

    // use selected images.
    UIntSet activeImgs = getActiveImages();
    for (UIntSet::iterator it = activeImgs.begin(); it != activeImgs.end(); ++it) {
//    for (unsigned int imgNr=0; imgNr < getNrOfImages(); imgNr++) {
        remapped.setPanoImage(*this, *it, opts);
        // calculate alpha channel
        remapped.calcAlpha(*this, opts, *it);
        // copy into global alpha channel.
        vigra::copyImageIf(vigra_ext::applyRect(remapped.boundingBox(),
                                              vigra_ext::srcMaskRange(remapped)),
                           vigra_ext::applyRect(remapped.boundingBox(),
                                              vigra_ext::srcMask(remapped)),
                           vigra_ext::applyRect(remapped.boundingBox(),
                                              destImage(panoAlpha)));
    }
//    vigra::ImageExportInfo imge("c:/hugin_calcfov_alpha.png");
//    exportImage(vigra::srcImageRange(panoAlpha), imge);

    // get field of view
    std::vector<int> borders;
    bool colOccupied = false;
    for (int h=0; h < 360; h++) {
        bool curColOccupied = false;
        for (int v=0; v< 180; v++) {
            if (panoAlpha(h,v)) {
                // pixel is valid
                curColOccupied = true;
            }
        }
        if (colOccupied && (! curColOccupied) ||
            (!colOccupied) && curColOccupied )
        {
            // change in position, save point.
            borders.push_back(h-180);
            colOccupied = curColOccupied;
        }
    }


    size_t lastidx = borders.size() -1;
    if (lastidx == -1) {
        // empty pano
        return;
    }

    if (colOccupied) {
        // we have reached the right border, and the pano is still valid
        // shift right fragments by 360 deg
        // |11    2222|  -> |      222211     |
        std::vector<int> newBorders;
        newBorders.push_back(borders[lastidx]);
        for (size_t i = 0; i < lastidx; i++) {
            newBorders.push_back(borders[i]+360);
        }
        borders = newBorders;
    }

    double dYaw=(borders[0] + borders[lastidx])/2;

    // apply yaw shift
    unsigned int nImg = getNrOfImages();
    for (unsigned int i=0; i < nImg; i++) {
        Variable & v = map_get(state.variables[i], "y");
        double yaw = v.getValue();
        yaw = yaw - dYaw;
        while (yaw < 180) {
            yaw += 360;
        }
        while (yaw > 180) {
            yaw -= 360;
        }
        v.setValue(yaw);
        imageChanged(i);
    }
}


void Panorama::updateCtrlPointErrors(const CPVector & cps)
{
    assert(cps.size() == state.ctrlPoints.size());
    unsigned int nrp = cps.size();
    for (unsigned int i = 0; i < nrp ; i++) {
        imageChanged(state.ctrlPoints[i].image1Nr);
        imageChanged(state.ctrlPoints[i].image2Nr);
        state.ctrlPoints[i].error = cps[i].error;
    }
}

void Panorama::updateVariables(const VariableMapVector & vars)
{
    assert(vars.size() == state.images.size());
    unsigned int i = 0;
    for (VariableMapVector::const_iterator it = vars.begin(); it != vars.end(); ++it) {
        updateVariables(i, *it);
        i++;
    }
}

void Panorama::updateVariables(unsigned int imgNr, const VariableMap & var)
{
    assert(imgNr < state.images.size());
    for (VariableMap::const_iterator it = var.begin(); it != var.end() ; ++it) {
        updateVariable(imgNr,it->second);
    }
}

void Panorama::updateVariable(unsigned int imgNr, const Variable &var)
{
//    DEBUG_TRACE("image " << imgNr << " variable: " << var.getName());
    DEBUG_ASSERT(imgNr < state.images.size());
    // update a single variable
    // check corrosponding lens if we have to update some other images
    // as well.
    unsigned int lensNr = state.images[imgNr].getLensNr();
    DEBUG_ASSERT(lensNr < state.lenses.size());

    // update value for this image
//    state.variables[imgNr][var.getName()].setValue(var.getValue());
    const std::string & s = var.getName();
    Variable & tvar = map_get(state.variables[imgNr], s);
    tvar.setValue(var.getValue());
    bool lensVar = set_contains(state.lenses[lensNr].variables, var.getName());
    if (lensVar) {
        // special handling for lens variables.
        // if they are inherited, update the value in the lens, and all
        // image variables that use this lens.
        LensVariable & lv = map_get(state.lenses[lensNr].variables,var.getName());
        if (lv.isLinked()) {
//            DEBUG_DEBUG("updating image variable, lens var is linked");
            lv.setValue(var.getValue());
            updateLensVariable(lensNr,lv);
        }
    }
    imageChanged(imgNr);
}

void Panorama::setOptimizeVector(const OptimizeVector & optvec)
{
    DEBUG_ASSERT(optvec.size() == state.images.size());
    state.optvec = optvec;
}


unsigned int Panorama::addImage(const PanoImage &img, const VariableMap & vars)
{
    // the lens must have been already created!
    bool ok = img.getLensNr() < state.lenses.size();
    DEBUG_ASSERT(ok);
    unsigned int nr = state.images.size();
    state.images.push_back(img);
    state.variables.push_back(vars);
    copyLensVariablesToImage(nr);
    imageChanged(nr);
    return nr;
}

#if 0
unsigned int Panorama::addImage(const std::string & filename)
{
    // create a lens if we don't have one.
    if (state.lenses.size() < 1) {
        state.lenses.push_back(Lens());
    }


    // read lens spec from image, if possible
    // FIXME use a lens database (for example the one from PTLens)
    // FIXME to initialize a,b,c etc.


    // searches for the new image for an unused lens , if found takes this
    // if no free lens is available creates a new one
    int unsigned lensNr (0);
    bool lens_belongs_to_image = false;
    int unused_lens = -1;
    while ( unused_lens < 0 ) {
      lens_belongs_to_image = false;
      for (ImageVector::iterator it = state.images.begin();
                                    it != state.images.end()  ; ++it) {
          if ((*it).getLens() == lensNr)
              lens_belongs_to_image = true;
      }
      if ( lens_belongs_to_image == false )
        unused_lens = lensNr;
      else
        lensNr++;
    }
    bool lens_allready_inside = false;
    for ( lensNr = 0 ; lensNr < state.lenses.size(); ++lensNr) {
        if ( (int)lensNr == unused_lens )
          lens_allready_inside = true;
    }
//    DEBUG_INFO ( "lens_allready_inside= "<< lens_allready_inside <<"  new lensNr: " << unused_lens <<"/"<< state.lenses.size() )
    Lens l;
    if ( lens_allready_inside )
        l = state.lenses[unused_lens];
    l.readEXIF(filename);
    if ( lens_allready_inside ) {
        state.lenses[unused_lens] = l;
    } else {
        state.lenses.push_back(l);
        unused_lens = state.lenses.size() - 1;
    }

    unsigned int nr = state.images.size();
    state.images.push_back(PanoImage(filename));
    ImageOptions opts = state.images.back().getOptions();
    opts.lensNr = unused_lens;
    state.images.back().setOptions(opts);
    state.variables.push_back(ImageVariables());
    updateLens(nr);
    adjustVarLinks();
    imageChanged(nr);
    DEBUG_INFO ( "new lensNr: " << unused_lens <<"/"<< state.lenses.size() )
    return nr;
}

#endif

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
    unsigned int lens = state.images[imgNr].getLensNr();
    unsigned int i = 0;
    for (ImageVector::iterator it = state.images.begin(); it != state.images.end(); ++it) {
        if ((*it).getLensNr() == lens && imgNr != i) {
            removeLens = false;
        }
        i++;
    }
    if (removeLens) {
	DEBUG_TRACE("removing lens " << lens);
        for (ImageVector::iterator it = state.images.begin(); it != state.images.end(); ++it) {
            if((*it).getLensNr() >= lens) {
                (*it).setLensNr((*it).getLensNr() - 1);
                imageChanged(it - state.images.begin());
            }
        }
        state.lenses.erase(state.lenses.begin() + lens);
    }

    DEBUG_TRACE("Remove variables and image from panorama state")
    state.variables.erase(state.variables.begin() + imgNr);
    state.images.erase(state.images.begin() + imgNr);

	// check if reference image has been moved
	if (state.options.optimizeReferenceImage >= state.images.size()) {
		state.options.optimizeReferenceImage = 0;
        imageChanged(state.options.optimizeReferenceImage);
	}

	if (state.options.colorReferenceImage >= state.images.size()) {
		state.options.colorReferenceImage = 0;
        imageChanged(state.options.colorReferenceImage);
	}

    // change all other (moved) images
    DEBUG_TRACE("flag moved images as dirty");
    for (unsigned int i=imgNr; i < state.images.size(); i++) {
        imageChanged(i);
    }
    m_forceImagesUpdate = true;
}


void Panorama::setImageFilename(unsigned int i, const std::string & fname)
{
    DEBUG_ASSERT(i < state.images.size());
    state.images[i].setFilename(fname);
    imageChanged(i);
    m_forceImagesUpdate = true;
}

void Panorama::setImageOptions(unsigned int i, const ImageOptions & opts)
{
    DEBUG_ASSERT(i < state.images.size());
    state.images[i].setOptions(opts);
    imageChanged(i);
    m_forceImagesUpdate = true;
}


unsigned int Panorama::addCtrlPoint(const ControlPoint & point )
{
    unsigned int nr = state.ctrlPoints.size();
    state.ctrlPoints.push_back(point);
    imageChanged(point.image1Nr);
    imageChanged(point.image2Nr);
    return nr;
}

void Panorama::removeCtrlPoint(unsigned int pNr)
{
    DEBUG_ASSERT(pNr < state.ctrlPoints.size());
    ControlPoint & point = state.ctrlPoints[pNr];
    unsigned int i1 = point.image1Nr;
    unsigned int i2 = point.image2Nr;
    state.ctrlPoints.erase(state.ctrlPoints.begin() + pNr);
    imageChanged(i1);
    imageChanged(i2);
}


void Panorama::changeControlPoint(unsigned int pNr, const ControlPoint & point)
{
    assert(pNr < state.ctrlPoints.size());

    // change notify for all involved images
    imageChanged(state.ctrlPoints[pNr].image1Nr);
    imageChanged(state.ctrlPoints[pNr].image2Nr);
    imageChanged(point.image1Nr);
    imageChanged(point.image2Nr);

    state.ctrlPoints[pNr] = point;
}

void Panorama::printPanoramaScript(ostream & o,
                                   const OptimizeVector & optvars,
                                   const PanoramaOptions & output,
                                   const UIntSet & imgs,
                                   bool forPTOptimizer,
                                   const std::string & stripPrefix)
{
#ifdef __unix__
    // set numeric locale to C, for correct number output
    char * t = setlocale(LC_NUMERIC,NULL);
    char * old_locale = (char*) malloc(strlen(t)+1);
    strcpy(old_locale, t);
    setlocale(LC_NUMERIC,"C");
#endif

    if (forPTOptimizer) {
        o << "# PTOptimizer script, written by hugin" << std::endl
          << std::endl;
    } else {
        o << "# hugin project file, version 1" << std::endl;
    }
    // output options..

    output.printScriptLine(o);

    std::map<unsigned int, unsigned int> linkAnchors;
    // map from script img nr -> pano image nr
    std::map<unsigned int, unsigned int> imageNrMap;
    o << std::endl
      << "# image lines" << std::endl;
    unsigned int ic = 0;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end();
         ++imgNrIt)
    {
	unsigned int imgNr = *imgNrIt;
        imageNrMap[imgNr] = ic;
	PanoImage & img = state.images[imgNr];
        unsigned int lensNr = img.getLensNr();
        Lens & lens = state.lenses[lensNr];
        const VariableMap & vars = state.variables[imgNr];

        o << "i w" << img.getWidth() << " h" << img.getHeight()
          <<" f" << lens.getProjection() << " ";

        // print variables with links
        for (VariableMap::const_iterator vit = vars.begin();
             vit != vars.end(); ++vit)
        {
            bool ptoptvar = set_contains(m_ptoptimizerVarNames,vit->first);
            if (!ptoptvar && forPTOptimizer) {
                continue;
            }
            // print links if needed
            if (set_contains(lens.variables,vit->first)
                && map_get(lens.variables, vit->first).isLinked())
            {
                if (set_contains(linkAnchors, lensNr)
                    && linkAnchors[lensNr] != imageNrMap[imgNr])
                {
                    // print link
                    DEBUG_DEBUG("printing link: " << vit->first);
                    // print link, anchor variable was already printed
                    map_get(lens.variables,vit->first).printLink(o,linkAnchors[lensNr]) << " ";
                } else {
                    DEBUG_DEBUG("printing value for linked var " << vit->first);
                    // first time, print value
                    linkAnchors[lensNr] = imageNrMap[imgNr];
                    vit->second.print(o) << " ";
                }
            } else {
                // simple variable, just print
                vit->second.print(o) << " ";
            }
        }

        ImageOptions iopts = img.getOptions();
        if (iopts.docrop) {
            // print crop parameters
            vigra::Rect2D c = iopts.cropRect;
            o << " S" << c.left() << "," << c.right() << "," << c.top() << "," << c.bottom();
        }

        if (!forPTOptimizer) {

            if (iopts.m_vigCorrMode != ImageOptions::VIGCORR_NONE) {
                o << " Vm" << iopts.m_vigCorrMode;
            }

            if (iopts.m_flatfield.size() > 0) {
                o << " Vf\"" << iopts.m_flatfield << "\"";
            }
        }
        
        o << " u" << output.featherWidth
          << (img.getOptions().morph ? " o" : "");
        string fname = img.getFilename();
        if (stripPrefix.size() > 0) {
            // strip prefix from image names.
            // check if the prefix is acutally the same
            string tmp = fname.substr(0,stripPrefix.size());
            if (tmp.compare(stripPrefix) == 0) {
                DEBUG_DEBUG("striping " << stripPrefix << " from " << fname);
                fname = fname.erase(0,stripPrefix.size());
                DEBUG_DEBUG("after stripping: " <<  fname);
            } else {
                DEBUG_DEBUG(stripPrefix << " does not match " << fname);
            }
        }
        o << " n\"" << fname << "\"" << std::endl;
        ic++;
    }

    o << std::endl << std::endl
      << "# specify variables that should be optimized" << std::endl
      << "v ";

    int optVarCounter=0;
    // be careful. linked variables should not be specified multiple times.
    vector<set<string> > linkvars(state.lenses.size());

    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end();
         ++imgNrIt)
    {
        unsigned int i = *imgNrIt;

        unsigned int lensNr = state.images[i].getLensNr();
        const Lens & lens = state.lenses[lensNr];
        const set<string> & optvar = optvars[i];
        for (set<string>::const_iterator sit = optvar.begin();
             sit != optvar.end(); ++sit )
        {
            if (set_contains(lens.variables,*sit)) {
                // it is a lens variable
                if (const_map_get(lens.variables,*sit).isLinked()) {
                    if (! set_contains(linkvars[lensNr], *sit))
                    {
                        // print only once
                        o << *sit << imageNrMap[i] << " ";
                        linkvars[lensNr].insert(*sit);
                        optVarCounter++;
                    }
                } else {
                    // unlinked lens variable, print as usual
                    o << *sit << imageNrMap[i] << " ";
                    optVarCounter++;
                }
            } else {
                // not a lens variable, print multiple times
                o << *sit << imageNrMap[i] << " ";
                optVarCounter++;
            }
        }
        // insert line break after 10 variables
        if (optVarCounter > 0) {
            o << std::endl << "v ";
            optVarCounter = 0;
        }
    }
    o << std::endl << std::endl
      << "# control points" << std::endl;
    for (CPVector::const_iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
		if (set_contains(imgs, it->image1Nr) && set_contains(imgs, it->image2Nr)) {
	        o << "c n" << imageNrMap[it->image1Nr]
		      << " N" << imageNrMap[it->image2Nr]
			  << " x" << it->x1 << " y" << it->y1
        	  << " X" << it->x2 << " Y" << it->y2
              << " t" << it->mode << std::endl;
        }
    }
    o << std::endl;

    // special line with hugins options.
    o << "#hugin_options r" << output.optimizeReferenceImage;
    switch (output.blendMode) {
    case PanoramaOptions::NO_BLEND:
        o << " e0";
        break;
	case PanoramaOptions::WEIGHTED_BLEND:
        o << " e1";
        break;
    case PanoramaOptions::SPLINE_BLEND:
        o << " e2";
	break;
    case PanoramaOptions::CHESSBOARD_BLEND:
        o << " e3";
	break;
    }
	o << std::endl;

#ifdef __unix__
    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
#endif
}


void Panorama::printStitcherScript(ostream & o,
                                   const PanoramaOptions & target,
                                   const UIntSet & imgs) const
{
#ifdef __unix__
    // set numeric locale to C, for correct number output
    char * t = setlocale(LC_NUMERIC,NULL);
    char * old_locale = (char*) malloc(strlen(t)+1);
    strcpy(old_locale, t);
    setlocale(LC_NUMERIC,"C");
#endif

    o << "# PTStitcher script, written by hugin" << std::endl
      << std::endl;
    // output options..
    target.printScriptLine(o);
    o << std::endl
      << "# output image lines" << std::endl;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end(); ++imgNrIt) {
        unsigned int imgNr = *imgNrIt;
        const PanoImage & img = state.images[imgNr];
        unsigned int lensNr = img.getLensNr();
        const Lens & lens = state.lenses[lensNr];
        const VariableMap & vars = state.variables[imgNr];

        o << "o w" << img.getWidth() << " h" << img.getHeight()
          <<" f" << state.lenses[lensNr].getProjection() << " ";
        // print variables, without links
        VariableMap::const_iterator vit;
        for(vit = vars.begin(); vit != vars.end();  ++vit)
        {
            if (!set_contains(m_ptoptimizerVarNames,vit->first)) {
                continue;
            }
            vit->second.print(o) << " ";
        }
        o << " u" << target.featherWidth << " m" << img.getOptions().ignoreFrameWidth
          << (img.getOptions().morph ? " o" : "")
          << " n\"" << img.getFilename() << "\"";
        if (img.getOptions().docrop) {
            // print crop parameters
            vigra::Rect2D c = img.getOptions().cropRect;
            o << " C" << c.left() << "," << c.right() << "," << c.top() << "," << c.bottom();
        }
        o << std::endl;
    }
    o << std::endl;
#ifdef __unix__
    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
#endif

}

void Panorama::readOptimizerOutput(const UIntSet & imgs, VariableMapVector & vars, CPVector & ctrlPoints) const
{
    std::ifstream script(PTScriptFile.c_str());
    if (!script.good()) {
        DEBUG_ERROR("Could not open " << PTScriptFile);
        // throw execption
        return;
    }
    parseOptimizerScript(script, imgs, vars, ctrlPoints);
}

void Panorama::parseOptimizerScript(istream & i, const UIntSet & imgs,
                                    VariableMapVector & imgVars, CPVector & CPs) const
{
    DEBUG_TRACE("");
#ifdef __unix__
    // set numeric locale to C, for correct number output
    char * t = setlocale(LC_NUMERIC,NULL);
    char * old_locale = (char*) malloc(strlen(t)+1);
    strcpy(old_locale, t);
    setlocale(LC_NUMERIC,"C");
#endif

    unsigned int ic=0;
    std::map<unsigned int, unsigned int> script2ImgMap;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end();
         ++imgNrIt)
    {
		unsigned int imgNr = *imgNrIt;
        script2ImgMap[ic] = imgNr;
        ic++;
    }
    ic = 0;
    unsigned int sc = 0;
    std::map<unsigned int, unsigned int> script2CPMap;
    for (CPVector::const_iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
        if (set_contains(imgs, it->image1Nr) && set_contains(imgs, it->image2Nr)) {
            script2CPMap[sc] = ic;
            sc++;
        }
        ic++;
    }



    // 0 = read output (image lines), 1 = read control point distances
    int state = 0;
    string line;
    unsigned int lineNr = 0;
    unsigned int scriptImgCounter = 0;
    unsigned int scriptCPCounter = 0;
//    VariableMapVector::iterator varIt = imgVars.begin();
//    CPVector::iterator pointIt = CPs.begin();

    int pnr=0;

    while (!i.eof()) {
        std::getline(i, line);
        lineNr++;
        switch (state) {
        case 0:
        {
            // we are reading the output lines:
            // o f3 r0 p0 y0 v89.2582 a-0.027803 b0.059851 c-0.073115 d10.542470 e16.121145 u10 -buf
            if ((line.compare("# Control Points: Distance between desired and fitted Position") == 0 )
             || (line.compare("# Control Points: Distance between desired and fitted Position (in Pixels)") == 0 )
             || (line.compare("# Control Points: Distance between desired and fitted Position (in \"Pixels\")") == 0 )) {
		
                // switch to reading the control point distance
                if (scriptImgCounter != imgs.size()) {
                    DEBUG_ERROR("Read only " << scriptImgCounter << " images from PTOptimizer file");
                }
                DEBUG_DEBUG("Changing state to read control point distances");
                state = 1;
                break;
            }
            if (line[0] != 'o') continue;
            // select variables of the image
            VariableMap & var = imgVars[script2ImgMap[scriptImgCounter]];
            DEBUG_DEBUG("reading image variables for image:" << scriptImgCounter);
            // read position variables
            int link;
            readVar(map_get(var, "r"), link, line);
            DEBUG_ASSERT(link == -1);
            readVar(map_get(var, "p"), link, line);
            DEBUG_ASSERT(link == -1);
            readVar(map_get(var, "y"), link, line);
            DEBUG_ASSERT(link == -1);

            DEBUG_DEBUG("yaw: " << map_get(var, "y").getValue()
                        << " pitch " << map_get(var, "p").getValue()
                        << " roll " << map_get(var, "r").getValue());
            // read lens variables

            for (char **c = Lens::variableNames; *c != 0; ++c) {
                Variable & curVar = map_get(var, *c);
                if (!readVar(curVar, link, line)) {
                    DEBUG_ERROR("Could not read "<< *c << " at script line " << lineNr);
                }
                // linking in output forbidden
                DEBUG_ASSERT(link == -1);
            }
            scriptImgCounter++;
            break;
        }
        case 1:
        {
            // read ctrl point distances:
            // # Control Point No 0:  0.428994
            if (line[0] == 'C') {
//                DEBUG_DEBUG(CPs.size() << " points, read: " << pnr);
                state = 2;
                break;
            }
            if (line.find("# Control Point No") != 0) continue;
            DEBUG_DEBUG("reading cp dist line: " << line);
            string::size_type p;
            if ((p=line.find(':')) == string::npos) assert(0);
            p++;
            DEBUG_DEBUG("parsing point " << scriptCPCounter << " (idx:" << p << "): " << line.substr(p));
            double err = -1;

            utils::stringToDouble(line.substr(p), err);
            CPs[script2CPMap[scriptCPCounter]].error = err;
            DEBUG_DEBUG("read CP distance " << err);
            scriptCPCounter++;
            break;
        }
        default:
            // ignore line..
            break;
        }
    }
#ifdef __unix__
    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
#endif

}

void Panorama::changeFinished(bool keepDirty)
{
    if (state.images.size() == 0) {
        // force an empty update if all images have been
        // removed
        DEBUG_DEBUG("forcing images update, with no images");
        m_forceImagesUpdate = true;
    }
    // remove change notification for nonexisting images from set.
    UIntSet::iterator uB = changedImages.lower_bound(state.images.size());
    changedImages.erase(uB,changedImages.end());

    stringstream t;
    copy(changedImages.begin(), changedImages.end(),
         ostream_iterator<unsigned int>(t, " "));
    DEBUG_TRACE("changed image(s) " << t.str() << " begin");
    std::set<PanoramaObserver *>::iterator it;
    for(it = observers.begin(); it != observers.end(); ++it) {
        DEBUG_TRACE("notifying listener");
        if (changedImages.size() > 0 || m_forceImagesUpdate) {
            (*it)->panoramaImagesChanged(*this, changedImages);
        }
        (*it)->panoramaChanged(*this);
    }
    // reset changed images
    changedImages.clear();
    m_forceImagesUpdate = false;
    if (!keepDirty) {
        dirty = true;
    }
    DEBUG_TRACE("end");
}

const Lens & Panorama::getLens(unsigned int lensNr) const
{
    assert(lensNr < state.lenses.size());
    return state.lenses[lensNr];
}


void Panorama::updateLens(unsigned int lensNr, const Lens & lens)
{
    DEBUG_TRACE(lensNr << " contains " << lens.variables.size() << " variables");
    assert(lensNr < state.lenses.size());
    state.lenses[lensNr].update(lens);
//    DEBUG_DEBUG("after update: " << lens.variables.size() << " variables");
    // copy changes to images
    for (LensVarMap::const_iterator it = state.lenses[lensNr].variables.begin();
         it != state.lenses[lensNr].variables.end();
         ++it)
    {
        DEBUG_DEBUG("updating " << it->second.getName() << " (key: " << it->first << ")");
        updateLensVariable(lensNr, it->second);
    }
}

void Panorama::updateLensVariable(unsigned int lensNr, const LensVariable &var)
{
    DEBUG_TRACE("lens " << lensNr << " variable: " << var.getName());
    DEBUG_ASSERT(lensNr < state.lenses.size());

    std::string varname = var.getName();
    LensVariable & realvar = map_get(state.lenses[lensNr].variables, varname);
    realvar = var;
    unsigned int nImages = state.images.size();
    for (unsigned int i=0; i<nImages; i++) {
        if (state.images[i].getLensNr() == lensNr) {
            // FIXME check for if really changed?
            imageChanged(i);
            map_get(state.variables[i], var.getName()).setValue(var.getValue());

            // check if the crop area should be automatically centered
            if ( var.getName() == "d" ) {
                ImageOptions opts = state.images[i].getOptions();
                if (opts.docrop && opts.autoCenterCrop) {
                    // horizontally center crop area.

                    double center = state.images[i].getWidth() / 2.0 + var.getValue();
                    int left = roundi(center - opts.cropRect.width() / 2.0);
                    int right = roundi(center + opts.cropRect.width() / 2.0);
                    opts.cropRect.setUpperLeft(Point2D(left, opts.cropRect.top()));
                    opts.cropRect.setLowerRight(Point2D(right, opts.cropRect.bottom()));
                    state.images[i].setOptions(opts);
                }
            }

            if ( var.getName() == "e" ) {
                ImageOptions opts = state.images[i].getOptions();
                if (opts.docrop && opts.autoCenterCrop) {
                    // horizontally center crop area.

                    double center = state.images[i].getHeight() / 2.0 + var.getValue();
                    int top = roundi(center - opts.cropRect.height() / 2.0);
                    int bottom = roundi(center + opts.cropRect.height() / 2.0);
                    opts.cropRect.setUpperLeft(Point2D(opts.cropRect.left(), top));
                    opts.cropRect.setLowerRight(Point2D(opts.cropRect.right(), bottom));
                    state.images[i].setOptions(opts);
                }
            }

        }
    }
}

void Panorama::setLens(unsigned int imgNr, unsigned int lensNr)
{
    DEBUG_TRACE("img: " << imgNr << "  lens:" << lensNr);
    assert(lensNr < state.lenses.size());
    assert(imgNr < state.images.size());
    state.images[imgNr].setLensNr(lensNr);
    imageChanged(imgNr);
    // copy the whole lens settings into the image
    copyLensVariablesToImage(imgNr);
    // FIXME: check if we overwrote the last instance of another lens
    removeUnusedLenses();
}

void Panorama::swapImages(unsigned int img1, unsigned int img2)
{
    DEBUG_TRACE("swapping images " << img1 << ", " << img2);
    DEBUG_ASSERT(img1 < state.images.size());
    DEBUG_ASSERT(img2 < state.images.size());

    // first, swap image struct
    PanoImage pimg1 = state.images[img1];
    state.images[img1] = state.images[img2];
    state.images[img2] = pimg1;

    // swap variables
    VariableMap vars1 = state.variables[img1];
    state.variables[img1] = state.variables[img2];
    state.variables[img2] = vars1;

    // update control points
    for (CPVector::iterator it=state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
        int n1 = (*it).image1Nr;
        int n2 = (*it).image2Nr;
        if ((*it).image1Nr == img1) {
            n1 = img2;
        } else if ((*it).image1Nr == img2) {
            n1 = img1;
        }
        if ((*it).image2Nr == img1) {
            n2 = img2;
        } else if ((*it).image2Nr == img2) {
            n2 = img1;
        }
        (*it).image1Nr = n1;
        (*it).image2Nr = n2;
    }

    // update panorama options
    if (state.options.colorReferenceImage == img1) {
        state.options.colorReferenceImage = img2;
    } else if (state.options.colorReferenceImage == img2) {
        state.options.colorReferenceImage = img1;
    }
    if (state.options.optimizeReferenceImage == img1) {
        state.options.optimizeReferenceImage = img2;
    } else if (state.options.optimizeReferenceImage == img2) {
        state.options.optimizeReferenceImage = img1;
    }


    imageChanged(img1);
    imageChanged(img2);
}

void Panorama::removeUnusedLenses()
{
    for (unsigned int lNr=0; lNr < state.lenses.size(); lNr++) {
        // check if this lens is lonely
        int n=0;
        for (unsigned int iNr=0; iNr < state.images.size(); iNr++) {
            if (state.images[iNr].getLensNr() == lNr) {
                n++;
            }
        }
        if (n == 0) {
            // not used by any image, remove from vector
            LensVector::iterator it = state.lenses.begin();
            it = it + lNr;
            state.lenses.erase(it);
            // adjust lens numbers inside images
            for (unsigned int iNr=0; iNr < state.images.size(); iNr++) {
                unsigned int imgLensNr = state.images[iNr].getLensNr();
                assert(imgLensNr != lNr);
                if ( imgLensNr > lNr) {
                    state.images[iNr].setLensNr(imgLensNr-1);
                    imageChanged(iNr);
                }
            }
        }
    }
}

void Panorama::removeLens(unsigned int lensNr)
{
    DEBUG_ASSERT(lensNr < state.lenses.size());
    // it is an error to remove all lenses.
    DEBUG_ASSERT(state.images.size() == 0 || lensNr > 0);
    for (unsigned int i = 0; i < state.images.size(); i++) {
        if (state.images[i].getLensNr() == lensNr) {
            state.images[i].setLensNr(0);
            copyLensVariablesToImage(i);
            imageChanged(i);
        }
    }
}

unsigned int Panorama::addLens(const Lens & lens)
{
    state.lenses.push_back(lens);
    return state.lenses.size() - 1;
}

void Panorama::setMemento(PanoramaMemento & memento)
{
    DEBUG_TRACE("");
    // remove old content.
    reset();
    DEBUG_DEBUG("nr of images in memento:" << memento.images.size());

    state = memento;
    unsigned int nNewImages = state.images.size();
    DEBUG_DEBUG("nNewImages:" << nNewImages);

    // send changes for all images
    for (unsigned int i = 0; i < nNewImages; i++) {
        imageChanged(i);
    }
}

PanoramaMemento Panorama::getMemento(void) const
{
    return PanoramaMemento(state);
}


void Panorama::setOptions(const PanoramaOptions & opt)
{
    if (state.options.optimizeReferenceImage != opt.optimizeReferenceImage) {
        imageChanged(opt.optimizeReferenceImage);
        imageChanged(state.options.optimizeReferenceImage);
    }

    if (state.options.colorReferenceImage != opt.colorReferenceImage) {
        imageChanged(opt.colorReferenceImage);
        imageChanged(state.options.colorReferenceImage);
    }

    state.options = opt;
}

int Panorama::addImageAndLens(const std::string & filename, double HFOV)
{
    // load image
    vigra::ImageImportInfo img(filename.c_str());
    // FIXME.. check for grayscale / color

    Lens lens;
    lens.setImageSize(vigra::Size2D(img.width(), img.height()));
    map_get(lens.variables,"v").setValue(HFOV);

    double cropFactor = 0;
    lens.initFromFile(filename, cropFactor);

    int matchingLensNr=-1;
    // FIXME: check if the exif information
    // indicates other camera parameters
    for (unsigned int lnr=0; lnr < getNrOfLenses(); lnr++) {
        const Lens & l = getLens(lnr);

        // use a lens if hfov and ratio are the same
        // should add a check for exif camera information as
        // well.
        if ((l.getAspectRatio() == lens.getAspectRatio()) &&
            (const_map_get(l.variables,"v").getValue() == const_map_get(lens.variables,"v").getValue()) &&
            (l.getSensorSize() == lens.getSensorSize()))
        {
            matchingLensNr= lnr;
        }
    }

    if (matchingLensNr == -1) {
        matchingLensNr = addLens(lens);
    }

    VariableMap vars;
    fillVariableMap(vars);

    DEBUG_ASSERT(matchingLensNr >= 0);
    PanoImage pimg(filename, img.width(), img.height(), (unsigned int) matchingLensNr);
    return addImage(pimg, vars);
}


void Panorama::addObserver(PanoramaObserver * o)
{
    observers.insert(o);
}

bool Panorama::removeObserver(PanoramaObserver * o)
{
    return observers.erase(o) > 0;
}

void Panorama::clearObservers()
{
    observers.clear();
}

void Panorama::imageChanged(unsigned int imgNr)
{
//    DEBUG_TRACE("adding image " << imgNr);
    changedImages.insert(imgNr);
    assert(changedImages.find(imgNr) != changedImages.end());
}

void Panorama::activateImage(unsigned int imgNr, bool active)
{
    assert(imgNr < state.images.size());
	ImageOptions o = getImage(imgNr).getOptions();
	if (o.active != active) {
		o.active = active;
        state.images[imgNr].setOptions(o);
		imageChanged(imgNr);
	}
}

UIntSet Panorama::getActiveImages() const
{
	UIntSet activeImgs;

    for (unsigned int i = 0; i < state.images.size(); i++) {
        if (state.images[i].getOptions().active) {
			activeImgs.insert(i);
        }
    }
	return activeImgs;
}

//==== internal function for variable & lens management

// update the variables of a Lens, when it has been changed.

void Panorama::copyLensVariablesToImage(unsigned int imgNr)
{
    unsigned int nImages = state.images.size();
    unsigned int nLenses = state.lenses.size();
    const PanoImage &img = state.images[imgNr];
    unsigned int lensNr = img.getLensNr();
    DEBUG_DEBUG("imgNr: " << imgNr << " of " << nImages);
    DEBUG_DEBUG("img lens nr: " << img.getLensNr() << " nr of lenses: " << state.lenses.size());
    DEBUG_ASSERT(imgNr < state.images.size());
    DEBUG_ASSERT(lensNr < nLenses);
    const Lens & lens = state.lenses[lensNr];
    for (LensVarMap::const_iterator it = lens.variables.begin();
         it != lens.variables.end();++it)
    {
        map_get(state.variables[imgNr], it->first).setValue(it->second.getValue());
    }
}




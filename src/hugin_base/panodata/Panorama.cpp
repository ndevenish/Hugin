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

#include "Panorama.h"

#include "PTScriptParsing.h"
#include "ImageVariableTranslate.h"
#include "StandardImageVariableGroups.h"
#include <panotools/PanoToolsInterface.h>
#include <algorithms/basic/CalculateOverlap.h>
#include <panodata/OptimizerSwitches.h>

#include <fstream>
#include <typeinfo>
#include <vigra/impex.hxx>


namespace HuginBase {

using namespace hugin_utils;

Panorama::Panorama()
    : //currentProcess(NO_PROCESS),
      //optimizerExe("PTOptimizer"),
      //stitcherExe("PTStitcher"),
      //PTScriptFile("PT_script.txt"),
      dirty(false),
      m_forceImagesUpdate(false)
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
    m_ptoptimizerVarNames.insert("TrX");
    m_ptoptimizerVarNames.insert("TrY");
    m_ptoptimizerVarNames.insert("TrZ");
    m_ptoptimizerVarNames.insert("Tpy");
    m_ptoptimizerVarNames.insert("Tpp");

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
    DEBUG_TRACE("dtor about to finish");
}


void Panorama::reset()
{
    //
    // imageChanged(0);
    // delete all images and control points.
    state.ctrlPoints.clear();
    state.deleteAllImages();
    state.options.reset();
    state.optvec.clear();
    state.optSwitch=0;
    state.optPhotoSwitch=0;
    state.needsOptimization = false;
    AppBase::DocumentData::setDirty(false);
    dirty=false;
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

CPointVector Panorama::getCtrlPointsVectorForImage(unsigned int imgNr) const
{
    CPointVector result;
    for(unsigned int i=0;i<state.ctrlPoints.size();i++)
    {
        ControlPoint point=state.ctrlPoints[i];
        if(point.image1Nr==imgNr)
        {
            result.push_back(std::make_pair(i,point));
        }
        else
        {
            if(point.image2Nr==imgNr)
            {
                point.mirror();
                result.push_back(std::make_pair(i,point));
            };
        };
    };
    return result;
};

VariableMapVector Panorama::getVariables() const
{
    VariableMapVector map;
    for (size_t i = 0; i < state.images.size(); i++)
    {
        map.push_back(state.images[i]->getVariableMap());
    }
    return map;
}

const VariableMap Panorama::getImageVariables(unsigned int imgNr) const
{
    assert(imgNr < state.images.size());
    return state.images[imgNr]->getVariableMap();
}


void Panorama::updateCtrlPointErrors(const UIntSet & imgs, const CPVector & cps)
{
    unsigned sc = 0;
    unsigned ic = 0;
    std::map<unsigned int, unsigned int> script2CPMap;
    for (CPVector::const_iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
        if (set_contains(imgs, it->image1Nr) && set_contains(imgs, it->image2Nr)) {
            script2CPMap[sc] = ic;
            sc++;
        }
        ic++;
    }

    // need to have same number of control points!
    assert(cps.size() == script2CPMap.size());
    unsigned i=0;
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); ++it) {
        imageChanged(script2CPMap[it->image1Nr]);
        imageChanged(script2CPMap[it->image2Nr]);
        state.ctrlPoints[script2CPMap[i]].error = it->error;
        i++;
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

void Panorama::updateVariables(const UIntSet & imgs, const VariableMapVector & vars)
{
    VariableMapVector::const_iterator v_it = vars.begin();
    for (UIntSet::const_iterator it = imgs.begin(); it != imgs.end(); ++it) {
        assert(*it < state.images.size());
        updateVariables(*it, *v_it);
        ++v_it;
    }
}

void Panorama::updateVariables(unsigned int imgNr, const VariableMap & var)
{
    if (imgNr > state.images.size())
        return;
    for (VariableMap::const_iterator it = var.begin(); it != var.end() ; ++it) {
        updateVariable(imgNr,it->second);
    }
}

void Panorama::updateVariable(unsigned int imgNr, const Variable &var)
{
    if (imgNr > state.images.size())
        return;
    // update a single variable, and everything linked to it.
    state.images[imgNr]->setVar(var.getName(), var.getValue());
    // call imageChanged for the images affected by this.
#define image_variable( name, type, default_value ) \
    if (PTOVariableConverterFor##name::checkApplicability(var.getName())) \
        {\
            for (std::size_t i = 0; i < getNrOfImages(); i++)\
            {\
                if (state.images[imgNr]->name##isLinkedWith(*state.images[i]))\
                {\
                    imageChanged(i);\
                }\
            }\
        }\
    else 
#include "image_variables.h"
#undef image_variable
    {// this is for the final else.
        DEBUG_ERROR("Unknown variable " << var.getName());
    }
    state.needsOptimization = true;
}

void Panorama::UpdateFocalLength(UIntSet imgs, double newFocalLength)
{
    for(UIntSet::const_iterator it=imgs.begin();it!=imgs.end();it++)
    {
        state.images[*it]->updateFocalLength(newFocalLength);
        imageChanged(*it);
    };
    //search for images with linked HFOV and mark these also as changed
    for(UIntSet::const_iterator it=imgs.begin();it!=imgs.end();it++)
    {
        SrcPanoImage *img=state.images[*it];
        if(state.images[*it]->HFOVisLinked())
        {
            for(unsigned int j=0;j<getNrOfImages();j++)
            {
                if(*it!=j)
                {
                    if(state.images[*it]->HFOVisLinkedWith(*img))
                    {
                        imageChanged(j);
                    };
                };
            };
        };
    };
};

void Panorama::UpdateCropFactor(UIntSet imgs, double newCropFactor)
{
    //store list of focal length, so we can keep the focal length unchanged and change hfov instead
    std::vector<double> focalLengthVector(getNrOfImages(),0.0);
    for(unsigned i=0;i<getNrOfImages();i++)
    {
        focalLengthVector[i]=state.images[i]->calcFocalLength(state.images[i]->getProjection(),
            state.images[i]->getHFOV(),state.images[i]->getExifCropFactor(),state.images[i]->getSize());
    };
    for(UIntSet::const_iterator it=imgs.begin();it!=imgs.end();it++)
    {
        state.images[*it]->updateCropFactor(focalLengthVector[*it],newCropFactor);
        imageChanged(*it);
    };
};

// What counts as changed in terms of the image variable links?
// Should I call imageChanged on every image linked to sourceImg and destImg?
// destImg's variable will change value to sourceImg's, so the images linked to
// destImg should be linked.
/// @todo call imageChanged on those images changed by the linking.
#define image_variable( name, type, default_value )\
void Panorama::linkImageVariable##name(unsigned int sourceImgNr, unsigned int destImgNr)\
{\
    state.images[destImgNr]->link##name(state.images[sourceImgNr]);\
    imageChanged(destImgNr);\
    imageChanged(sourceImgNr);\
    state.needsOptimization = true;\
}
#include "image_variables.h"
#undef image_variable

/// @todo call imageChanged on those images changed by the unlinking.
#define image_variable( name, type, default_value )\
void Panorama::unlinkImageVariable##name(unsigned int imgNr)\
{\
    state.images[imgNr]->unlink##name();\
    imageChanged(imgNr);\
    state.needsOptimization = true;\
}
#include "image_variables.h"
#undef image_variable

void Panorama::setOptimizeVector(const OptimizeVector & optvec)
{
    DEBUG_ASSERT(optvec.size() == state.images.size());
    state.optvec = optvec;
}

void Panorama::setOptimizerSwitch(const int newSwitch)
{
    if(state.optSwitch!=newSwitch)
    {
        state.optSwitch=newSwitch;
    };
};

void Panorama::setPhotometricOptimizerSwitch(const int newSwitch)
{
    if(state.optPhotoSwitch!=newSwitch)
    {
        state.optPhotoSwitch=newSwitch;
    };
};

unsigned int Panorama::addImage(const SrcPanoImage &img)
{
    unsigned int nr = state.images.size();
    state.images.push_back(new SrcPanoImage(img));
    // create empty optimisation vector
    state.optvec.push_back(std::set<std::string>());
    imageChanged(nr);
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

    DEBUG_TRACE("Remove variables and image from panorama state")
    delete state.images[imgNr];
    state.images.erase(state.images.begin() + imgNr);
    state.optvec.erase(state.optvec.begin() + imgNr);

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
    state.images[i]->setFilename(fname);
    imageChanged(i);
    m_forceImagesUpdate = true;
}

unsigned int Panorama::addCtrlPoint(const ControlPoint & point )
{
    unsigned int nr = state.ctrlPoints.size();
    state.ctrlPoints.push_back(point);
    imageChanged(point.image1Nr);
    imageChanged(point.image2Nr);
    state.needsOptimization = true;
    return nr;
}

void Panorama::removeCtrlPoint(unsigned int pNr)
{
    DEBUG_ASSERT(pNr < state.ctrlPoints.size());
    ControlPoint & point = state.ctrlPoints[pNr];
    unsigned int i1 = point.image1Nr;
    unsigned int i2 = point.image2Nr;
    state.ctrlPoints.erase(state.ctrlPoints.begin() + pNr);

    // update line control points
    updateLineCtrlPoints();
    imageChanged(i1);
    imageChanged(i2);
    state.needsOptimization = true;
}

void Panorama::removeDuplicateCtrlPoints()
{
    std::set<std::string> listOfCPs;
    std::set<unsigned int> duplicateCPs;
    for(unsigned int i=0; i<state.ctrlPoints.size();i++)
    {
        std::string s=state.ctrlPoints[i].getCPString();
        std::pair<std::set<std::string>::iterator,bool> it=listOfCPs.insert(s);
        if(it.second==false)
        {
            duplicateCPs.insert(i);
        };
    }
    //now remove duplicate control points, mark affected images as changed
    if(duplicateCPs.size()>0)
    {
        for(std::set<unsigned int>::reverse_iterator it=duplicateCPs.rbegin();it!=duplicateCPs.rend();it++)
        {
            ControlPoint cp=state.ctrlPoints[*it];
            imageChanged(cp.image1Nr);
            imageChanged(cp.image2Nr);
            removeCtrlPoint(*it);
        };
    };
    updateLineCtrlPoints();
}


void Panorama::changeControlPoint(unsigned int pNr, const ControlPoint & point)
{
    assert(pNr < state.ctrlPoints.size());

    // change notify for all involved images
    imageChanged(state.ctrlPoints[pNr].image1Nr);
    imageChanged(state.ctrlPoints[pNr].image2Nr);
    imageChanged(point.image1Nr);
    imageChanged(point.image2Nr);
    state.needsOptimization = true;

    state.ctrlPoints[pNr] = point;
    updateLineCtrlPoints();
}

void Panorama::setCtrlPoints(const CPVector & points)
{
    for (CPVector::const_iterator it = state.ctrlPoints.begin();
         it != state.ctrlPoints.end(); it++)
    {
        imageChanged(it->image1Nr);
        imageChanged(it->image2Nr);
    }

    state.ctrlPoints = points;

    for (CPVector::const_iterator it = state.ctrlPoints.begin();
         it != state.ctrlPoints.end(); it++)
    {
        imageChanged(it->image1Nr);
        imageChanged(it->image2Nr);
    }
    state.needsOptimization = true;
    updateLineCtrlPoints();
}

// close holes in line control points
void Panorama::updateLineCtrlPoints()
{
    // sort all line control points
    std::map<int, int> lines;
    for (CPVector::const_iterator it = state.ctrlPoints.begin();
         it != state.ctrlPoints.end(); it++)
    {
        if (it->mode > 2)
            lines[it->mode] = 0;
    }
    int i=3;
    for (std::map<int,int >::iterator it = lines.begin(); it != lines.end(); ++it) 
    {
        (*it).second = i;
        i++;
    }

    for (CPVector::iterator it = state.ctrlPoints.begin();
         it != state.ctrlPoints.end(); it++)
    {
        if (it->mode > 2) {
            int newmode = lines[it->mode];
            if (it->mode != newmode) {
                it->mode = newmode;
                imageChanged(it->image1Nr);
                imageChanged(it->image2Nr);
            }
        }
    }
}


void Panorama::printPanoramaScript(std::ostream & o,
                                   const OptimizeVector & optvars,
                                   const PanoramaOptions & output,
                                   const UIntSet & imgs,
                                   bool forPTOptimizer,
                                   const std::string & stripPrefix) const
{
    using namespace std;
    
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
        o << "# hugin project file" << std::endl;
        o << "#hugin_ptoversion 2" << std::endl;
    }
    // output options..

    output.printScriptLine(o, forPTOptimizer);

    std::map<unsigned int, unsigned int> linkAnchors;
    // map from script img nr -> pano image nr
    std::map<unsigned int, unsigned int> imageNrMap;
    o << std::endl
      << "# image lines" << std::endl;
    
    // somewhere to store the v lines, which give the variables to be optimised.
    std::stringstream vlines;
    std::stringstream masklines;
    unsigned int ic = 0;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end();
         ++imgNrIt)
    {
        unsigned int imgNr = *imgNrIt;
        imageNrMap[imgNr] = ic;
        const SrcPanoImage & img = *state.images[imgNr];
        VariableMap vars;

        // print special comment line with hugin GUI data
        o << "#-hugin ";
        if (img.getCropMode() != BaseSrcPanoImage::NO_CROP) {
            if (img.getAutoCenterCrop())
                o << " autoCenterCrop=1";
        }
        o << " cropFactor=" << img.getExifCropFactor() ;
        if (! img.getActive()) {
            o << " disabled";
        }
        o << std::endl;

        o << "i w" << img.getSize().width() << " h" << img.getSize().height()
          <<" f" << img.getProjection() << " ";

        // print variables with links
/* Individually do all the variables specified by each SrcPanoImg variable.
 * Clear the list after each SrcPanoImg variable.
 * If there is any links to previous images, write that in the script.
 * If there are no links to previous images, write the value instead.
 * Each variable in SrcPanoImg may produce any number of variables in the map,
 *      but the linking properties are shared.
 * Additionally, when we are writing variables by value which are set to be
 * optimised, we should remember them so we can write them later as 'v' lines.
 */
#define image_variable( name, type, default_value )\
        PTOVariableConverterFor##name::addToVariableMap(state.images[imgNr]->get##name##IV(), vars);\
        if (!vars.empty())\
        {\
            bool linking = false;\
            std::size_t link_target;\
            if (ic!=0)\
            {\
                if (state.images[imgNr]->name##isLinked())\
                {\
                    for (link_target = 0; link_target < imgNr; link_target++)\
                    {\
                        if (set_contains(imgs,link_target) && state.images[imgNr]->name##isLinkedWith(*state.images[link_target]))\
                        {\
                            linking = true;\
                            break;\
                        }\
                    }\
                }\
            }\
            for (VariableMap::const_iterator vit = vars.begin();\
             vit != vars.end(); ++vit)\
            {\
                if (forPTOptimizer && !set_contains(m_ptoptimizerVarNames,vit->first))\
                    continue;\
                else if (linking)\
                {\
                    o << vit->first << "=" << imageNrMap[link_target] << " ";\
                } else {\
                    if (set_contains(optvars[imgNr], vit->first))\
                    {\
                        vlines << "v " << vit->first << imageNrMap[imgNr] << std::endl;\
                    }\
                    if (((vit->first == "a" && set_contains(optvars[imgNr], "a") )|| \
                                (vit->first == "b" && set_contains(optvars[imgNr], "b") )|| \
                                (vit->first == "c" && set_contains(optvars[imgNr], "c") )|| \
                                (vit->first == "TrX" && set_contains(optvars[imgNr], "TrX") )|| \
                                (vit->first == "TrY" && set_contains(optvars[imgNr], "TrY") )|| \
                                (vit->first == "TrZ" && set_contains(optvars[imgNr], "TrZ") )|| \
                                (vit->first == "Tpy" && set_contains(optvars[imgNr], "Tpy") )|| \
                                (vit->first == "Tpp" && set_contains(optvars[imgNr], "Tpp") )\
                               )\
                               && forPTOptimizer && vit->second.getValue() == 0.0) \
                    {\
                        o << vit->first << 1e-5 << " ";\
                    } else if (( (vit->first == "r" && set_contains(optvars[imgNr], "r") ) || \
                                 (vit->first == "p" && set_contains(optvars[imgNr], "p") ) || \
                                 (vit->first == "y" && set_contains(optvars[imgNr], "y") ) \
                               )\
                               && forPTOptimizer && fabs(vit->second.getValue()) < 1e-13)\
                    {\
                        o << vit->first << 0 << " ";\
                    } else {\
                        vit->second.print(o) << " ";\
                    }\
                }\
            }\
        }\
        vars.clear();
#include "image_variables.h"
#undef image_variable

        if (img.getCropMode()!=SrcPanoImage::NO_CROP) {
            // print crop parameters
            vigra::Rect2D c = img.getCropRect();
            o << " S" << c.left() << "," << c.right() << "," << c.top() << "," << c.bottom();
        }

        if (!forPTOptimizer) {

            if (img.getVigCorrMode() != SrcPanoImage::VIGCORR_NONE) {
                o << " Vm" << img.getVigCorrMode();
            }

            if (img.getFlatfieldFilename().size() > 0) {
                o << " Vf\"" << img.getFlatfieldFilename() << "\"";
            }
            if (img.getResponseType() > 0) {
                o << " Rt" << img.getResponseType();
            }

            if(img.hasMasks())
                img.printMaskLines(masklines,ic);
        }

#if 0
//panotools paramters, currently not used
        o << " u" << output.featherWidth
          << (img.getMorph() ? " o" : "");
#endif
        std::string fname = img.getFilename();
        if (stripPrefix.size() > 0) {
            // strip prefix from image names.
            // check if the prefix is acutally the same
            std::string tmp = fname.substr(0,stripPrefix.size());
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
      << vlines.str()
      << "v" << std::endl; // empty v line to work around libpano13 bug.
    
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

    if(masklines.str().length()>0)
        o << "# masks" << std::endl
            << masklines.str()
            << std::endl;

    // special line with hugins options.
    o << "#hugin_optimizeReferenceImage " << output.optimizeReferenceImage << std::endl;
    o << "#hugin_blender ";
    switch (output.blendMode) {
        case PanoramaOptions::NO_BLEND:
            o << "none" << endl;
            break;
        case PanoramaOptions::PTBLENDER_BLEND:
            o << "PTblender" << endl;
            break;
        case PanoramaOptions::SMARTBLEND_BLEND:
            o << "smartblend" << endl;
            break;
        case PanoramaOptions::PTMASKER_BLEND:
            o << "PTmasker" << endl;
            break;
        default:
        case PanoramaOptions::ENBLEND_BLEND:
            o << "enblend" << endl;
            break;
    }
    
    o << "#hugin_remapper ";
    switch (output.remapper) {
        case PanoramaOptions::PTMENDER:
            o << "PTmender" << endl;
            break;
        default:
        case PanoramaOptions::NONA:
            o << "nona" << endl;
            break;
    }
    
    o << "#hugin_enblendOptions " << output.enblendOptions << endl;
    o << "#hugin_enfuseOptions " << output.enfuseOptions << endl;
    o << "#hugin_hdrmergeOptions " << output.hdrmergeOptions << endl;

    o << "#hugin_outputLDRBlended " << (output.outputLDRBlended ? "true" : "false") << endl;
    o << "#hugin_outputLDRLayers " << (output.outputLDRLayers ? "true" : "false") << endl;
    o << "#hugin_outputLDRExposureRemapped " << (output.outputLDRExposureRemapped ? "true" : "false") << endl;
    o << "#hugin_outputLDRExposureLayers " << (output.outputLDRExposureLayers ? "true" : "false") << endl;
    o << "#hugin_outputLDRExposureBlended " << (output.outputLDRExposureBlended ? "true" : "false") << endl;
    o << "#hugin_outputLDRStacks " << (output.outputLDRStacks ? "true" : "false") << endl;
    o << "#hugin_outputLDRExposureLayersFused " << (output.outputLDRExposureLayersFused ? "true" : "false") << endl;
    o << "#hugin_outputHDRBlended " << (output.outputHDRBlended ? "true" : "false") << endl;
    o << "#hugin_outputHDRLayers " << (output.outputHDRLayers ? "true" : "false") << endl;
    o << "#hugin_outputHDRStacks " << (output.outputHDRStacks ? "true" : "false") << endl;    

    o << "#hugin_outputLayersCompression " << output.outputLayersCompression << endl;
    o << "#hugin_outputImageType " << output.outputImageType << endl;
    o << "#hugin_outputImageTypeCompression " << output.outputImageTypeCompression << endl;
    o << "#hugin_outputJPEGQuality " << output.quality << endl;
    o << "#hugin_outputImageTypeHDR " << output.outputImageTypeHDR << endl;
    o << "#hugin_outputImageTypeHDRCompression " << output.outputImageTypeHDRCompression << endl;

    o << "#hugin_outputStacksMinOverlap " << setprecision(3) << output.outputStacksMinOverlap << endl;
    o << "#hugin_outputLayersExposureDiff " << setprecision(2) << output.outputLayersExposureDiff << endl;

    if(optvars==getOptimizeVector())
    {
        o << "#hugin_optimizerMasterSwitch " << getOptimizerSwitch() << endl;
        o << "#hugin_optimizerPhotoMasterSwitch " << getPhotometricOptimizerSwitch() << endl;
    }
    else
    {
        o << "#hugin_optimizerMasterSwitch 0" << endl;
        o << "#hugin_optimizerPhotoMasterSwitch 0" << endl;
    };

#ifdef __unix__
    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);
#endif
}


void Panorama::printStitcherScript(std::ostream & o,
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
    target.printScriptLine(o, true);
    o << std::endl
      << "# output image lines" << std::endl;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end(); ++imgNrIt) {
        unsigned int imgNr = *imgNrIt;
        const SrcPanoImage & img = *state.images[imgNr];
// DGSW FIXME - Unreferenced
//		const Lens & lens = state.lenses[lensNr];
        const VariableMap & vars = state.images[imgNr]->getVariableMap();

        o << "o w" << img.getSize().width() << " h" << img.getSize().height()
          <<" f" << img.getProjection() << " ";
        // print variables, without links
        VariableMap::const_iterator vit;
        for(vit = vars.begin(); vit != vars.end();  ++vit)
        {
            if (!set_contains(m_ptoptimizerVarNames,vit->first)) {
                continue;
            }
            vit->second.print(o) << " ";
        }
#if 0
// panotools parameters, currently not used 
        o << " u" << img.getFeatureWidth() 
          << (img.getMorph() ? " o" : "");
#endif
        o << " n\"" << img.getFilename() << "\"";
        if (img.getCropMode()!=SrcPanoImage::NO_CROP) {
            // print crop parameters
            vigra::Rect2D c = img.getCropRect();
            o << " S" << c.left() << "," << c.right() << "," << c.top() << "," << c.bottom();
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

void Panorama::parseOptimizerScript(std::istream & i, const UIntSet & imgs,
                                    VariableMapVector & imgVars, CPVector & CPs) const
{
    using namespace std;
    using namespace PTScriptParsing;
    
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

// DGSW FIXME - Unreferenced
//	int pnr=0;

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

            readVar(map_get(var, "TrX"), link, line);
            DEBUG_ASSERT(link == -1);
            readVar(map_get(var, "TrY"), link, line);
            DEBUG_ASSERT(link == -1);
            readVar(map_get(var, "TrZ"), link, line);
            DEBUG_ASSERT(link == -1);
            readVar(map_get(var, "Tpy"), link, line);
            DEBUG_ASSERT(link == -1);
            readVar(map_get(var, "Tpp"), link, line);
            DEBUG_ASSERT(link == -1);

            DEBUG_DEBUG("X: " << map_get(var, "TrX").getValue()
                        << " Y " << map_get(var, "TrY").getValue()
                        << " Z " << map_get(var, "TrZ").getValue());
            // read lens variables


            for (const char **c = Lens::variableNames; *c != 0; ++c) {
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

            hugin_utils::stringToDouble(line.substr(p), err);
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

    std::stringstream t;
    copy(changedImages.begin(), changedImages.end(),
         std::ostream_iterator<unsigned int>(t, " "));
    DEBUG_TRACE("changed image(s) " << t.str() << " begin");
    //force update of crops
    if(changedImages.size()>0)
    {
        for(UIntSet::iterator it=changedImages.begin();it!=changedImages.end();it++)
        {
            //if the projection was changed, we need to update the crop mode
            updateCropMode(*it);
            //now center the crop if user requested it
            if(state.images[*it]->getAutoCenterCrop())
            {
                centerCrop(*it);
            };
        };
    };
    //update masks
    updateMasks();
    updateOptimizeVector();
    std::list<PanoramaObserver *>::iterator it;
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
        AppBase::DocumentData::setDirty(dirty);
    }
    DEBUG_TRACE("end");
}

void Panorama::updateMasksForImage(unsigned int imgNr, MaskPolygonVector newMasks)
{
    DEBUG_ASSERT(imgNr < state.images.size());
    state.images[imgNr]->setMasks(newMasks);
    imageChanged(imgNr);
    m_forceImagesUpdate = true;
};

void Panorama::transferMask(MaskPolygon mask,unsigned int imgNr, const UIntSet targetImgs)
{
    if(targetImgs.size()==0)
    {
        return;
    };
    MaskPolygon transformedMask=mask;
    // clip positive mask to image boundaries or clip region
    vigra::Rect2D clipRect=vigra::Rect2D(0,0,state.images[imgNr]->getWidth(),state.images[imgNr]->getHeight());
    if(mask.isPositive())
    {
        //clip to crop region only positive masks
        switch(state.images[imgNr]->getCropMode())
        {
            case BaseSrcPanoImage::CROP_RECTANGLE:
                clipRect=clipRect & state.images[imgNr]->getCropRect();
                if(clipRect.isEmpty())
                {
                    return;
                };
                if(clipRect.width()>10 && clipRect.height()>10)
                {
                    clipRect.addBorder(-2);
                };
                break;
            case BaseSrcPanoImage::CROP_CIRCLE:
                {
                    vigra::Rect2D cropRect=state.images[imgNr]->getCropRect();
                    FDiff2D center=FDiff2D((cropRect.left()+cropRect.right())/2.0,(cropRect.top()+cropRect.bottom())/2.0);
                    double radius=((cropRect.width()<cropRect.height())?cropRect.width():cropRect.height())/2.0;
                    if(radius>10)
                    {
                        radius-=2;
                    };
                    if(!transformedMask.clipPolygon(center,radius))
                    {
                        return;
                    };
                };
                break;
            default:
                if(clipRect.width()>10 && clipRect.height()>10)
                {
                    clipRect.addBorder(-2);
                };
                break;
        };
    };
    int origWindingNumber=transformedMask.getTotalWindingNumber();
    if(transformedMask.clipPolygon(clipRect))
    {
        //increase resolution of positive mask to get better transformation
        //of vertices, especially for fisheye images
        transformedMask.subSample(20);
        //transform polygon to panorama space
        HuginBase::PTools::Transform trans;
        trans.createInvTransform(getImage(imgNr),getOptions());
        transformedMask.transformPolygon(trans);
        for(UIntSet::const_iterator it=targetImgs.begin();it!=targetImgs.end();it++)
        {
            if(imgNr==(*it))
            {
                continue;
            };
            MaskPolygon targetMask;
            if(state.images[imgNr]->YawisLinkedWith(*(state.images[*it])))
            {
                //if yaw is linked, we simply copy the mask
                targetMask=mask;
            }
            else
            {
                targetMask=transformedMask;
                PTools::Transform targetTrans;
                targetTrans.createTransform(getImage(*it),getOptions());
                targetMask.transformPolygon(targetTrans);
                //check if transformation has produced invalid polygon
                if(targetMask.getMaskPolygon().size()<3)
                {
                    continue;
                };
                //check if mask was inverted - outside became inside and vice versa
                //if so, invert mask
                int newWindingNumber=targetMask.getTotalWindingNumber();
                targetMask.setInverted(origWindingNumber * newWindingNumber < 0);
            };
            //now clip polygon to image rectangle, add mask only when polygon is inside image
            if(targetMask.clipPolygon(vigra::Rect2D(-maskOffset,-maskOffset,
                                      state.images[*it]->getWidth()+maskOffset,state.images[*it]->getHeight()+maskOffset)))
            {
                targetMask.setMaskType(MaskPolygon::Mask_negative);
                targetMask.setImgNr(*it);
                state.images[*it]->addActiveMask(targetMask);
            };
        };
    };        
};

void Panorama::updateMasks(bool convertPosMaskToNeg)
{
    // update masks
    UIntSet imgWithPosMasks;
    for(unsigned int i=0;i<state.images.size();i++)
    {
        state.images[i]->clearActiveMasks();
        if(state.images[i]->hasPositiveMasks())
        {
            imgWithPosMasks.insert(i);
        };
    };
    CalculateImageOverlap overlap(this);
    overlap.limitToImages(imgWithPosMasks);
    overlap.calculate(10);
    ConstStandardImageVariableGroups variable_groups(*this);
    ConstImageVariableGroup & lenses = variable_groups.getLenses();
    for(unsigned int i=0;i<state.images.size();i++)
    {
        if(state.images[i]->hasMasks())
        {
            MaskPolygonVector masks=state.images[i]->getMasks();
            for(unsigned int j=0;j<masks.size();j++)
            {
                if(convertPosMaskToNeg)
                {
                    //this is used for masking in the cp finder, we are consider
                    //all masks as negative masks, because at this moment
                    //the final position of the images is not known
                    switch(masks[j].getMaskType())
                    {
                        case MaskPolygon::Mask_negative:
                        case MaskPolygon::Mask_positive:
                            masks[j].setImgNr(i);
                            masks[j].setMaskType(MaskPolygon::Mask_negative);
                            state.images[i]->addActiveMask(masks[j]);
                            break;
                        case MaskPolygon::Mask_Stack_negative:
                        case MaskPolygon::Mask_Stack_positive:
                            {
                                //copy mask to all images of the same stack
                                UIntSet imgStack;
                                for(unsigned int k=0;k<getNrOfImages();k++)
                                {
                                    if(i!=k)
                                    {
                                        if(state.images[i]->StackisLinkedWith(*(state.images[k])))
                                        {
                                            imgStack.insert(k);
                                        };
                                    };
                                };
                                masks[j].setImgNr(i);
                                masks[j].setMaskType(MaskPolygon::Mask_negative);
                                state.images[i]->addActiveMask(masks[j]);
                                transferMask(masks[j],i,imgStack);
                            };
                            break;
                        case MaskPolygon::Mask_negative_lens:
                            {
                                unsigned int lensNr=lenses.getPartNumber(i);
                                //copy masks to all image of the same lens
                                UIntSet imgLens;
                                for(unsigned int k=0;k<getNrOfImages();k++)
                                {
                                    if(lenses.getPartNumber(k)==lensNr)
                                    {
                                        masks[j].setImgNr(k);
                                        masks[j].setMaskType(MaskPolygon::Mask_negative_lens);
                                        state.images[k]->addActiveMask(masks[j]);
                                    };
                                };
                            };
                            break;
                    };
                }
                else
                {
                    switch(masks[j].getMaskType())
                    {
                        case MaskPolygon::Mask_negative:
                            //negative mask, simply copy mask to active mask
                            masks[j].setImgNr(i);
                            state.images[i]->addActiveMask(masks[j]);
                            break;
                        case MaskPolygon::Mask_positive:
                            //propagate positive mask only if image is active
                            if(state.images[i]->getActive())
                            {
                                UIntSet overlapImgs=overlap.getOverlapForImage(i);
                                transferMask(masks[j],i,overlapImgs);
                            };
                            break;
                        case MaskPolygon::Mask_Stack_negative:
                            {
                                //search all images of the stack
                                UIntSet imgStack;
                                for(unsigned int k=0;k<getNrOfImages();k++)
                                {
                                    if(i!=k)
                                    {
                                        if(state.images[i]->StackisLinkedWith(*(state.images[k])))
                                        {
                                            imgStack.insert(k);
                                        };
                                    };
                                };
                                //copy mask also to the image which contains the mask
                                masks[j].setImgNr(i);
                                masks[j].setMaskType(MaskPolygon::Mask_negative);
                                state.images[i]->addActiveMask(masks[j]);
                                transferMask(masks[j],i,imgStack);
                            };
                            break;
                        case MaskPolygon::Mask_Stack_positive:
                            {
                                //remove all images from the stack from the set
                                UIntSet imgStack;
                                fill_set(imgStack,0,getNrOfImages()-1);
                                imgStack.erase(i);
                                for(unsigned int k=0;k<getNrOfImages();k++)
                                {
                                    if(i!=k)
                                    {
                                        if(state.images[i]->StackisLinkedWith(*(state.images[k])))
                                        {
                                            imgStack.erase(k);
                                        };
                                    };
                                };
                                //only leave overlapping images in set
                                UIntSet imgOverlap=overlap.getOverlapForImage(i);
                                UIntSet imgs;
                                std::set_intersection(imgStack.begin(),imgStack.end(),imgOverlap.begin(),imgOverlap.end(),inserter(imgs,imgs.begin()));
                                //now transfer mask
                                transferMask(masks[j],i,imgs);
                            };
                            break;
                        case MaskPolygon::Mask_negative_lens:
                            {
                                unsigned int lensNr=lenses.getPartNumber(i);
                                //copy masks to all image of the same lens
                                UIntSet imgLens;
                                for(unsigned int k=0;k<getNrOfImages();k++)
                                {
                                    if(lenses.getPartNumber(k)==lensNr)
                                    {
                                        masks[j].setImgNr(k);
                                        masks[j].setMaskType(MaskPolygon::Mask_negative_lens);
                                        state.images[k]->addActiveMask(masks[j]);
                                    };
                                };
                            };
                            break;
                    };
                };
            };
        };
    };
};

void Panorama::updateCropMode(unsigned int imgNr)
{
    vigra::Rect2D r=state.images[imgNr]->getCropRect();
    if(r.isEmpty() || r==vigra::Rect2D(state.images[imgNr]->getSize()))
    {
        state.images[imgNr]->setCropMode(SrcPanoImage::NO_CROP);
    }
    else
    {
        if (state.images[imgNr]->isCircularCrop())
        {
            state.images[imgNr]->setCropMode(SrcPanoImage::CROP_CIRCLE);
        }
        else
        {
            state.images[imgNr]->setCropMode(SrcPanoImage::CROP_RECTANGLE);
        };
    };
};

vigra::Rect2D Panorama::centerCropImage(unsigned int imgNr)
{
    vigra::Rect2D cropRect;
    if(state.images[imgNr]->getCropMode()==SrcPanoImage::NO_CROP)
    {
        return cropRect;
    };
    int dx = roundi(state.images[imgNr]->getRadialDistortionCenterShift().x);
    int dy = roundi(state.images[imgNr]->getRadialDistortionCenterShift().y);
    vigra::Point2D center = vigra::Point2D(state.images[imgNr]->getSize().width()/2 + dx, state.images[imgNr]->getSize().height()/2 + dy);

    vigra::Diff2D d(state.images[imgNr]->getCropRect().width() / 2, state.images[imgNr]->getCropRect().height() / 2);
    cropRect.setUpperLeft( center - d);
    cropRect.setLowerRight( center + d);
    return cropRect;
};

void Panorama::centerCrop(unsigned int imgNr)
{
    vigra::Rect2D cropRect;
    if(  state.images[imgNr]->getCropMode()!=SrcPanoImage::NO_CROP &&
         state.images[imgNr]->getAutoCenterCrop() && 
       !(state.images[imgNr]->getCropRect().isEmpty())
       )
    {
        cropRect=centerCropImage(imgNr);
        if(!cropRect.isEmpty())
        {
            state.images[imgNr]->setCropRect(cropRect);
            imageChanged(imgNr);
        };
    };
    for (std::size_t i = 0; i < getNrOfImages(); i++)
    {
        if(i==imgNr)
        {
            continue;
        };
        if (state.images[imgNr]->RadialDistortionCenterShiftisLinkedWith(*state.images[i]))
        {
            if(  state.images[i]->getCropMode()!=SrcPanoImage::NO_CROP &&
                 state.images[i]->getAutoCenterCrop() && 
               !(state.images[i]->getCropRect().isEmpty())
               )
            {
                cropRect=centerCropImage(i);
                if(!cropRect.isEmpty())
                {
                    state.images[i]->setCropRect(cropRect);
                    imageChanged(i);
                };
            };
        };
    };
};

void UpdateOptVectorSet(std::set<std::string>& imgVar, const std::string var, const bool opt)
{
    if(opt)
    {
        imgVar.insert(var);
    }
    else
    {
        imgVar.erase(var);
    };
};

std::set<size_t> Panorama::getRefImages()
{
    unsigned int refImg = getOptions().optimizeReferenceImage;
    std::set<size_t> refImgs;
    refImgs.insert(refImg);
    const HuginBase::SrcPanoImage & refImage = getImage(refImg);
    for (size_t imgNr = 0; imgNr < getNrOfImages(); imgNr++)
    {
        if(imgNr!=refImg)
        {
            const HuginBase::SrcPanoImage & compImage = getImage(imgNr);
            if (refImage.YawisLinkedWith(compImage))
            {
                refImgs.insert(imgNr);
            };
        };
    };
    return refImgs;
};

void Panorama::checkRefOptStatus(bool& linkRefImgsYaw, bool& linkRefImgsPitch, bool& linkRefImgsRoll)
{
    // count number of vertical/horizontal control points
    int nHCP = 0;
    int nVCP = 0;
    const CPVector & cps = getCtrlPoints();
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); it++)
    {
        // control points
        if (it->mode == ControlPoint::X)
        {
            nVCP++;
        }
        else
        {
            if (it->mode == ControlPoint::Y)
            {
                nHCP++;
            }
        };
    };

    // try to select sensible position optimisation parameters,
    // dependent on output projection
    linkRefImgsYaw=false;
    linkRefImgsPitch=false;
    linkRefImgsRoll=false;
    switch (getOptions().getProjection())
    {
        case PanoramaOptions::RECTILINEAR:
            linkRefImgsRoll = nVCP + nHCP >= 1;
            linkRefImgsYaw = nVCP + nHCP >= 3 && nVCP >= 1 && nHCP >= 1;
            linkRefImgsPitch = nVCP + nHCP >= 2;
            break;
        case PanoramaOptions::CYLINDRICAL:
        case PanoramaOptions::EQUIRECTANGULAR:
            linkRefImgsPitch =  nHCP + nVCP > 1;
            linkRefImgsRoll = nHCP + nVCP >= 1;
            break;
        default:
            break;
    };
};

void Panorama::updateOptimizeVector()
{
    if(state.images.size()==0)
    {
        return;
    };
    if(state.optSwitch!=0)
    {
        std::set<size_t> refImgs=getRefImages();
        bool linkRefImgsYaw=false;
        bool linkRefImgsPitch=false;
        bool linkRefImgsRoll=false;
        checkRefOptStatus(linkRefImgsYaw, linkRefImgsPitch, linkRefImgsRoll);

        for(size_t i=0;i<getNrOfImages();i++)
        {
            if(state.optSwitch & OPT_PAIR || state.optSwitch & OPT_POSITION || state.optSwitch & OPT_ALL)
            {
                if(set_contains(refImgs,i))
                {
                    UpdateOptVectorSet(state.optvec[i],"y",linkRefImgsYaw);
                    UpdateOptVectorSet(state.optvec[i],"p",linkRefImgsPitch);
                    UpdateOptVectorSet(state.optvec[i],"r",linkRefImgsRoll);
                    //don't optimize translation parameters of anchor
                    UpdateOptVectorSet(state.optvec[i],"TrX",false);
                    UpdateOptVectorSet(state.optvec[i],"TrY",false);
                    UpdateOptVectorSet(state.optvec[i],"TrZ",false);
                }
                else
                {
                    UpdateOptVectorSet(state.optvec[i],"y",true);
                    UpdateOptVectorSet(state.optvec[i],"p",true);
                    UpdateOptVectorSet(state.optvec[i],"r",true);
                    UpdateOptVectorSet(state.optvec[i],"TrX",(state.optSwitch & OPT_TRANSLATION)>0);
                    UpdateOptVectorSet(state.optvec[i],"TrY",(state.optSwitch & OPT_TRANSLATION)>0);
                    UpdateOptVectorSet(state.optvec[i],"TrZ",(state.optSwitch & OPT_TRANSLATION)>0);
                };
            }
            else
            {
                UpdateOptVectorSet(state.optvec[i],"y",false);
                UpdateOptVectorSet(state.optvec[i],"p",false);
                UpdateOptVectorSet(state.optvec[i],"r",false);
                UpdateOptVectorSet(state.optvec[i],"Trx",false);
                UpdateOptVectorSet(state.optvec[i],"Try",false);
                UpdateOptVectorSet(state.optvec[i],"Trz",false);
            };
            UpdateOptVectorSet(state.optvec[i],"v",state.optSwitch & OPT_VIEW || state.optSwitch & OPT_ALL);
            UpdateOptVectorSet(state.optvec[i],"a",(state.optSwitch & OPT_ALL)>0);
            UpdateOptVectorSet(state.optvec[i],"b",state.optSwitch & OPT_BARREL || state.optSwitch & OPT_ALL);
            UpdateOptVectorSet(state.optvec[i],"c",(state.optSwitch & OPT_ALL)>0);
            UpdateOptVectorSet(state.optvec[i],"d",(state.optSwitch & OPT_ALL)>0);
            UpdateOptVectorSet(state.optvec[i],"e",(state.optSwitch & OPT_ALL)>0);
            //shear and translation plane not include in master switches
            UpdateOptVectorSet(state.optvec[i],"g",false);
            UpdateOptVectorSet(state.optvec[i],"t",false);
            UpdateOptVectorSet(state.optvec[i],"Tpy", false);
            UpdateOptVectorSet(state.optvec[i],"Tpp", false);
        };
    };
    if(state.optPhotoSwitch!=0)
    {
        for(size_t i=0;i<getNrOfImages();i++)
        {
            UpdateOptVectorSet(state.optvec[i],"Eev",state.optPhotoSwitch & OPT_EXPOSURE && i!=state.options.colorReferenceImage);
            UpdateOptVectorSet(state.optvec[i],"Er", state.optPhotoSwitch & OPT_WHITEBALANCE && i!=state.options.colorReferenceImage);
            UpdateOptVectorSet(state.optvec[i],"Eb", state.optPhotoSwitch & OPT_WHITEBALANCE && i!=state.options.colorReferenceImage);
            UpdateOptVectorSet(state.optvec[i],"Vb", (state.optPhotoSwitch & OPT_VIGNETTING)>0);
            UpdateOptVectorSet(state.optvec[i],"Vc", (state.optPhotoSwitch & OPT_VIGNETTING)>0);
            UpdateOptVectorSet(state.optvec[i],"Vd", (state.optPhotoSwitch & OPT_VIGNETTING)>0);
            UpdateOptVectorSet(state.optvec[i],"Vx", (state.optPhotoSwitch & OPT_VIGNETTING_CENTER)>0);
            UpdateOptVectorSet(state.optvec[i],"Vy", (state.optPhotoSwitch & OPT_VIGNETTING_CENTER)>0);
            UpdateOptVectorSet(state.optvec[i],"Ra", (state.optPhotoSwitch & OPT_RESPONSE)>0);
            UpdateOptVectorSet(state.optvec[i],"Rb", (state.optPhotoSwitch & OPT_RESPONSE)>0);
            UpdateOptVectorSet(state.optvec[i],"Rc", (state.optPhotoSwitch & OPT_RESPONSE)>0);
            UpdateOptVectorSet(state.optvec[i],"Rd", (state.optPhotoSwitch & OPT_RESPONSE)>0);
            UpdateOptVectorSet(state.optvec[i],"Re", (state.optPhotoSwitch & OPT_RESPONSE)>0);
        };
    };
};

void Panorama::swapImages(unsigned int img1, unsigned int img2)
{
    DEBUG_TRACE("swapping images " << img1 << ", " << img2);
    DEBUG_ASSERT(img1 < state.images.size());
    DEBUG_ASSERT(img2 < state.images.size());

    // first, swap image pointers in the list.
    SrcPanoImage * pimg1 = state.images[img1];
    state.images[img1] = state.images[img2];
    state.images[img2] = pimg1;
    
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

void Panorama::moveImage(size_t img1, size_t img2)
{
    //generate a vector with the translated image numbers
    std::vector<size_t> imgList(getNrOfImages(),-1);
    for(size_t i=0; i<getNrOfImages(); i++)
    {
        imgList[i]=i;
    };
    imgList.erase(imgList.begin()+img1);
    if(img2<imgList.size())
    {
        imgList.insert(imgList.begin()+img2, img1);
    }
    else
    {
        imgList.push_back(img1);
    };
    //generate map for translation of old -> new image numbers
    std::map<size_t,size_t> imgMap;
    for(size_t i=0; i<imgList.size(); i++)
    {
        imgMap[imgList[i]]=i;
    };
    // now generate the new images list
    std::vector<SrcPanoImage *> new_images(getNrOfImages());
    for(size_t i=0; i<imgList.size(); i++)
    {
        new_images[i]=state.images[imgList[i]];
        if(i!=imgList[i])
        {
            imageChanged(imgList[i]);
        };
    };
    state.images=new_images;

    // update optimize vector
    OptimizeVector newOptVec;
    for(size_t i=0; i<state.optvec.size(); i++)
    {
        newOptVec.push_back(state.optvec[imgList[i]]);
    };
    state.optvec=newOptVec;

    // update control points
    for (CPVector::iterator it=state.ctrlPoints.begin(); it != state.ctrlPoints.end(); it++)
    {
        (*it).image1Nr = imgMap[(*it).image1Nr];
        (*it).image2Nr = imgMap[(*it).image2Nr];
    }

    // update panorama options
    state.options.colorReferenceImage=imgMap[state.options.colorReferenceImage];
    state.options.optimizeReferenceImage=imgMap[state.options.optimizeReferenceImage];
};

bool Panorama::setMementoToCopyOf(const PanoramaDataMemento* memento)
{
    if(memento==NULL)
        return false;
    
    const PanoramaMemento* mymemento;
        
    try {
        
        mymemento = dynamic_cast<const PanoramaMemento*>(memento);
        
    } catch (std::bad_cast e) {
//        std::cerr << "Incompatible memento type." << std::endl;
        DEBUG_DEBUG("Incompatible memento type.");
        return false;
    }
    
    setMemento(PanoramaMemento(*mymemento));
    return true;
}


/// set the internal state
void Panorama::setMemento(const PanoramaMemento& memento)
{
    DEBUG_TRACE("");
    
    // remove old content.
    reset();
    DEBUG_DEBUG("nr of images in memento:" << memento.images.size());
    
    state = memento;
    updateMasks();
    unsigned int nNewImages = state.images.size();
    DEBUG_DEBUG("nNewImages:" << nNewImages);
    
    // send changes for all images
    for (unsigned int i = 0; i < nNewImages; i++) {
        imageChanged(i);
    }
}

PanoramaDataMemento* Panorama::getNewMemento() const
{
    return new PanoramaMemento(getMemento());
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

void Panorama::addObserver(PanoramaObserver * o)
{
    observers.push_back(o);
}

bool Panorama::removeObserver(PanoramaObserver * o)
{
    size_t oldCount=observers.size();
    observers.remove(o);
    return observers.size()!=oldCount;
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
    if (state.images[imgNr]->getActive() != active)
    {
        state.images[imgNr]->setActive(active);
        imageChanged(imgNr);
    }
}

UIntSet Panorama::getActiveImages() const
{
	UIntSet activeImgs;

    for (unsigned int i = 0; i < state.images.size(); i++) {
        if (state.images[i]->getActive())
        {
            activeImgs.insert(i);
        }
    }
	return activeImgs;
}

//==== internal function for variable management

SrcPanoImage Panorama::getSrcImage(unsigned imgNr) const
{
    DEBUG_ASSERT(imgNr < state.images.size());
    return *state.images[imgNr];
}

void Panorama::setSrcImage(unsigned int imgNr, const SrcPanoImage & img)
{
    DEBUG_ASSERT(imgNr < state.images.size());
    
    /* Copy the variables. We don't assign directly so we can do the changes to
     * any linked variables.
     */
    SrcPanoImage *dest = state.images[imgNr];
    #define image_variable( name, type, default_value ) \
    dest->set##name (img.get##name());
    #include "image_variables.h"
    #undef image_variable
    
    // mark the potentially changed images.
#define image_variable( name, type, default_value ) \
    for (std::size_t i = 0; i < getNrOfImages(); i++)\
    {\
        if(state.images[imgNr]->name##isLinkedWith(*state.images[i]))\
        {\
            imageChanged(i);\
        }\
    }
#include "image_variables.h"
#undef image_variable
}


Panorama Panorama::duplicate() const
{
    Panorama pano(*this);
    pano.observers.clear();
    return pano;
}

Panorama Panorama::getSubset(const UIntSet & imgs) const
{
    Panorama subset;
    // copy data except for listners
    
    // bits that don't change in the subset.
    subset.imgFilePrefix = imgFilePrefix;
    subset.dirty = dirty;
    subset.state.options = state.options;
    subset.state.optSwitch=0;
    subset.state.optPhotoSwitch=0;
    subset.state.needsOptimization = state.needsOptimization;
    subset.changedImages = changedImages;
    subset.m_forceImagesUpdate = m_forceImagesUpdate;
    subset.m_ptoptimizerVarNames = m_ptoptimizerVarNames;
    
    // create image number map.
    std::map<unsigned int, unsigned int> imageNrMap;

    // copy image information
    unsigned int ic = 0;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end();
         ++imgNrIt)
    {
        subset.state.images.push_back(new SrcPanoImage(*state.images[*imgNrIt]));
        subset.state.optvec.push_back(state.optvec[*imgNrIt]);
        imageNrMap[*imgNrIt] = ic;
        ic++;
    }
    
    // recreate links between image variables.
    ic = 0;
    for (UIntSet::const_iterator i = imgs.begin(); i != imgs.end(); ++i)
    {
        unsigned int jc = ic + 1;
        UIntSet::const_iterator j = i;
        for (++j; j != imgs.end(); ++j)
        {
            /** @todo It should be possible to speed this up by not linking
             * things that have been already linked to something previously
             * linked to the target.
             */
#define image_variable( name, type, default_value )\
            if (state.images[*i]->name##isLinkedWith(*state.images[*j]))\
            {\
                subset.state.images[ic]->link##name(subset.state.images[jc]);\
            }
#include "image_variables.h"
#undef image_variable
            jc++;
        }
        ic++;
    }

    // select and translate control points.
    subset.state.ctrlPoints.clear();
    for (CPVector::const_iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it) {
        if (set_contains(imgs, it->image1Nr) && set_contains(imgs, it->image2Nr)) {
            ControlPoint pnt = *it;
            pnt.image1Nr = imageNrMap[pnt.image1Nr];
            pnt.image2Nr = imageNrMap[pnt.image2Nr];
            subset.state.ctrlPoints.push_back(pnt);
        }
    }

    //update optimizeReferenceImage and colorReferenceImage number
    unsigned int newRefImg=0;
    std::map<unsigned int, unsigned int>::iterator it=imageNrMap.find(state.options.optimizeReferenceImage);
    if(it!=imageNrMap.end())
    {
        newRefImg=it->second;
    };
    it=imageNrMap.find(state.options.colorReferenceImage);
    subset.state.options.optimizeReferenceImage=newRefImg;
    newRefImg=0;
    if(it!=imageNrMap.end())
    {
        newRefImg=it->second;
    }
    subset.state.options.colorReferenceImage=newRefImg;
    return subset;
}

void Panorama::mergePanorama(const Panorama &newPano)
{
    if(newPano.getNrOfImages()>0)
    {
        std::vector<unsigned int> new_image_nr(newPano.getNrOfImages());
        unsigned int oldNrOfImage=getNrOfImages();
        HuginBase::OptimizeVector optVec=getOptimizeVector();
        HuginBase::OptimizeVector optVecNew=newPano.getOptimizeVector();
        //add only new images
        for(unsigned int i=0;i<newPano.getNrOfImages();i++)
        {
            std::string filename=newPano.getImage(i).getFilename();
            bool found=false;
            for(unsigned int j=0;j<getNrOfImages();j++)
            {
                if(getImage(j).getFilename()==filename)
                {
                    //image is already in panorama, we remember the image nr
                    found=true;
                    new_image_nr[i]=j;
                    // now check if we have to update the masks
                    HuginBase::MaskPolygonVector masksOld=getImage(j).getMasks();
                    HuginBase::MaskPolygonVector masksNew=newPano.getImage(i).getMasks();
                    if(masksNew.size()>0)
                    {
                        for(unsigned int k=0;k<masksNew.size();k++)
                        {
                            bool usedMasks=false;
                            unsigned int l=0;
                            while((!usedMasks) && l<masksOld.size())
                            {
                                usedMasks=(masksNew[k]==masksOld[l]);
                                l++;
                            };
                            if(!usedMasks)
                                masksOld.push_back(masksNew[k]);
                        };
                        updateMasksForImage(j,masksOld);
                    };
                    break;
                };
            };
            if(!found)
            {
                //new image found, add it
                new_image_nr[i]=addImage(newPano.getImage(i));
                //copy also optimise vector
                optVec.push_back(optVecNew[i]);
            };
        };
        setOptimizeVector(optVec);
        // recreate links between image variables.
        for (unsigned int i=0; i<newPano.getNrOfImages(); i++)
        {
            for(unsigned int j=i+1;j<newPano.getNrOfImages();j++)
            {
                const HuginBase::SrcPanoImage &img=newPano.getImage(i);
#define image_variable( name, type, default_value )\
                if(img.name##isLinkedWith(newPano.getImage(j)))\
                {\
                    linkImageVariable##name(new_image_nr[i],new_image_nr[j]);\
                };
#include "panodata/image_variables.h"
#undef image_variable
            }
        }
        //now translate cp
        CPVector cps=newPano.getCtrlPoints();
        for(unsigned int i=0;i<cps.size();i++)
        {
            HuginBase::ControlPoint cp(new_image_nr[cps[i].image1Nr], cps[i].x1, cps[i].y1,
                new_image_nr[cps[i].image2Nr],cps[i].x2, cps[i].y2, cps[i].mode);
            addCtrlPoint(cp);
        };
        removeDuplicateCtrlPoints();
    };
};

int Panorama::getNextCPTypeLineNumber() const
{
    int t=0;
    for (CPVector::const_iterator it = state.ctrlPoints.begin(); it != state.ctrlPoints.end(); ++it)
    {
        t = std::max(t, it->mode);
    }
    if (t <= 2) {
        t=2;
    }
    return t+1;
}


Panorama::ReadWriteError Panorama::readData(std::istream& dataInput, std::string documentType)
{
    // [TODO] check the document type, return INCOMPATIBLE_TYPE
    
    if(!dataInput.good() || dataInput.eof())
    {
        DEBUG_WARN("Failed to read from dataInput.");
        return INVALID_DATA;
    }
    
    PanoramaMemento newPano;
    int ptoVersion;
    if (newPano.loadPTScript(dataInput, ptoVersion, getFilePrefix())) {
        
        this->setMemento(newPano);
        return SUCCESSFUL;
        
    } else {
        DEBUG_FATAL("Could not parse the data input successfully.");
        return PARCER_ERROR;
    }
}

///
Panorama::ReadWriteError Panorama::writeData(std::ostream& dataOutput, std::string documentType)
{
    UIntSet all;
    
    if (getNrOfImages() > 0)
        fill_set(all, 0, getNrOfImages()-1);
    
    printPanoramaScript(dataOutput, getOptimizeVector(), getOptions(), all, false, getFilePrefix());
    
    return SUCCESSFUL;
}

void Panorama::updateWhiteBalance(double redFactor, double blueFactor)
{
    UIntSet modified_images;
    for(unsigned int i=0;i<getNrOfImages();i++)
    {
        if(!set_contains(modified_images,i))
        {
            state.images[i]->setWhiteBalanceRed(redFactor * state.images[i]->getWhiteBalanceRed());
            state.images[i]->setWhiteBalanceBlue(blueFactor * state.images[i]->getWhiteBalanceBlue());
            modified_images.insert(i);
            imageChanged(i);
            //check linked images and remember for later
            if(state.images[i]->WhiteBalanceRedisLinked())
            {
                if(i+1<getNrOfImages())
                {
                    for(unsigned int j=i+1;j<getNrOfImages();j++)
                    {
                        if(state.images[i]->WhiteBalanceRedisLinkedWith(*(state.images[j])))
                        {
                            modified_images.insert(j);
                            imageChanged(j);
                        };
                    };
                };
            };
        };
    };
};

PanoramaMemento::PanoramaMemento(const PanoramaMemento & data)
{
    // Use the assignment operator to get the work done: see the next function.
    *this = data;
}

PanoramaMemento & PanoramaMemento::operator=(const PanoramaMemento & data)
{
    // Copy the PanoramaMemento.
    
    // Don't do anything in the case of self assignment. This is important as we
    // are about to delete the image information.
    if (&data == this)
    {
        return *this;
    }
    
    // Remove any images we currently had.
    deleteAllImages();
    // copy image variables
    for (std::vector<SrcPanoImage *>::const_iterator it = data.images.begin();
         it != data.images.end(); it++)
    {
        images.push_back(new SrcPanoImage(*(*it)));
    }
    // Copies of SrcPanoImage's variables aren't linked, so we have to create
    // new links in the same pattern.
    /** @todo This is quite inefficent, maybe we should store the links as a
     * vector of sets of image numbers for each variable to speed up this?
     * Links / unlinks should all go through the Panorama object, so we
     * could keep track of them easily.
     */
    std::size_t num_imgs = images.size();
    for (std::size_t i = 0; i < num_imgs; i++)
    {
        // copy this image's links.
        // links to lower numbered images will have already been spotted, since
        // they are bi-directional.
        for (std::size_t j = i + 1; j < num_imgs; j++)
        {
#define image_variable( name, type, default_value )\
            if (data.images[i]->name##isLinkedWith(*data.images[j]))\
            {\
                images[i]->link##name(images[j]);\
            }
#include "image_variables.h"
#undef image_variable
        }
    }
    
    ctrlPoints = data.ctrlPoints;
    
    options = data.options;
    
    optSwitch = data.optSwitch;
    optPhotoSwitch = data.optPhotoSwitch;
    optvec = data.optvec;

    needsOptimization = data.needsOptimization;
    
    return *this;
}

PanoramaMemento::~PanoramaMemento()
{
    deleteAllImages();
}

void PanoramaMemento::deleteAllImages()
{
    // delete all the images pointed to by the images vector.
    for (std::vector<SrcPanoImage *>::iterator it = images.begin();
         it != images.end(); it++)
    {
        delete *it;
    }
    // now clear the pointers themselves.
    images.clear();
}

bool PanoramaMemento::loadPTScript(std::istream &i, int & ptoVersion, const std::string &prefix)
{
    using namespace std;
    using namespace PTScriptParsing;
    
    DEBUG_TRACE("");
    // set numeric locale to C, for correct number output
    char * p = setlocale(LC_NUMERIC,NULL);
    char * old_locale = strdup(p);
    setlocale(LC_NUMERIC,"C");
    PTParseState state;
    string line;

    // vector with the different information lines about images
    vector<ImgInfo> oImgInfo;
    vector<ImgInfo> iImgInfo;
    // strange comment informations.
    vector<ImgInfo> cImgInfo;
    // hugin additional information
    vector<ImgInfo> huginImgInfo;
    // vector with readed masks
    MaskPolygonVector ImgMasks;

    // indicate lines that should be skipped for whatever reason
    bool skipNextLine = false;

    bool PTGUIScriptFile = false;
    int PTGUIScriptVersion = 0;
    // PTGui lens line detected
    int ctrlPointsImgNrOffset = 0;
    bool PTGUILensLine = false;

    bool PTGUILensLoaded = false;
    ImgInfo PTGUILens;

    // set new options to some sensible default.
    options.reset();
    options.tiff_saveROI = false;

    ptoVersion = 1;

    bool firstOptVecParse = true;
    unsigned int lineNr = 0;    
    while (i.good()) {
        std::getline(i, line);
        lineNr++;
        DEBUG_DEBUG(lineNr << ": " << line);
        if (skipNextLine) {
            skipNextLine = false;
            continue;
        }
        //skip emtpy lines
        if(line.empty())
            continue;
        // check for a known line
        switch(line[0]) {
        case 'p':
        {
            DEBUG_DEBUG("p line: " << line);
            int i;
            if (getIntParam(i,line,"f"))
                options.setProjection( (PanoramaOptions::ProjectionFormat) i );
            unsigned int w;
            if (getIntParam(w, line, "w"))
                options.setWidth(w);
            double v;
            if (getDoubleParam(v, line, "v"))
                options.setHFOV(v, false);
            int height;
            if (getIntParam(height, line, "h"))
                options.setHeight(height);

            double newE;
            if (getDoubleParam(newE, line, "E"))
                options.outputExposureValue = newE;
            int ar=0;
            if (getIntParam(ar, line, "R"))
                options.outputMode = (PanoramaOptions::OutputMode) ar;

            string format;
            if (getPTParam(format,line,"T"))
                options.outputPixelType = format;

            if ( getPTParam(format, line, "S") ) {
                int left, right, top, bottom;
                int n = sscanf(format.c_str(), "%d,%d,%d,%d", &left, &right, &top, &bottom);
                if (n == 4) {
                    options.setROI(vigra::Rect2D(left, top, right, bottom));
                } else {
                    DEBUG_WARN("Could not parse crop string: " << format);
                }
            }

            // parse projection parameters
            if (getPTParam(format,line,"P")) {
                char * tstr = strdup(format.c_str());
                std::vector<double> projParam;
                char * b = strtok(tstr, " \"");
                if (b != NULL) {
                    while (b != NULL) {
                    double tempDbl;
                    if (sscanf(b, "%lf", &tempDbl) == 1) {
                        projParam.push_back(tempDbl);
                        b = strtok(NULL, " \"");
                    }
                    }
                }
                free(tstr);
                // only set projection parameters, if the have the right size.
                if (projParam.size() == options.getProjectionParameters().size()) {
                    options.setProjectionParameters(projParam);
                }
            }

            // this is fragile.. hope nobody adds additional whitespace
            // and other arguments than q...
            // n"JPEG q80"
            if (getPTParam(format,line,"n")) {
                int t = format.find(' ');
                options.outputFormat = options.getFormatFromName(format.substr(0,t));

                // parse output format options.
                switch (options.outputFormat)
                {
                    case PanoramaOptions::JPEG:
                    {
                        // "parse" jpg quality
                        int q;
                        if (getIntParam(q, format, "q") ) {
                        options.quality = (int) q;
                        }
                    }
                    break;
                    case PanoramaOptions::TIFF_m:
                    {
                        int coordImgs = 0;
                        if (getIntParam(coordImgs, format, "p"))
                        if (coordImgs)
                            options.saveCoordImgs = true;
                    }
                    case PanoramaOptions::TIFF:
                    case PanoramaOptions::TIFF_mask:
                    case PanoramaOptions::TIFF_multilayer:
                    case PanoramaOptions::TIFF_multilayer_mask:
                    {
                        // parse tiff compression mode
                        std::string comp;
                        if (getPTParam(comp, format, "c:")) {
                            if (comp == "NONE" || comp == "LZW" ||
                                comp == "PACKBITS" || comp == "DEFLATE")
                            {
                                options.tiffCompression = comp;
                            } else {
                                DEBUG_WARN("No valid tiff compression found");
                            }
                        }
                        // read tiff roi
                        if (getPTParam(comp, format, "r:")) {
                            if (comp == "CROP") {
                                options.tiff_saveROI = true;
                            } else {
                                options.tiff_saveROI = false;
                            }
                        }
                    }
                break;
                default:
                break;
                }
            }

            int cRefImg = 0;
            if (getIntParam(cRefImg, line,"k")) {
                options.colorCorrection = PanoramaOptions::BRIGHTNESS_COLOR;
            } else if (getIntParam(cRefImg, line,"b")) {
                options.colorCorrection = PanoramaOptions::BRIGHTNESS;
            } else if (getIntParam(cRefImg, line,"d")) {
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
            if (getIntParam(i,line,"i"))
		options.interpolator = (vigra_ext::Interpolator) i;
            (void)getDoubleParam(options.gamma,line,"g");

            if (getIntParam(i,line,"f")) {
                switch(i) {
                case 0:
                    options.remapAcceleration = PanoramaOptions::MAX_SPEEDUP;
                    break;
                case 1:
                    options.remapAcceleration = PanoramaOptions::MEDIUM_SPEEDUP;
                    break;
                default:
                    options.remapAcceleration = PanoramaOptions::NO_SPEEDUP;
                    break;
                }
            } else {
                options.remapAcceleration = PanoramaOptions::NO_SPEEDUP;
            }

            break;
        }
        case 'v':
        {
            DEBUG_DEBUG("v line: " << line);
            if (!PTGUIScriptFile) {
                if (firstOptVecParse) {
                    int nImg = max(iImgInfo.size(), oImgInfo.size());
                    DEBUG_DEBUG("nImg: " << nImg);
                    optvec = OptimizeVector(nImg);
                    firstOptVecParse = false;
                }
                std::stringstream optstream;
                optstream << line.substr(1);
                string var;
                while (!(optstream >> std::ws).eof()) {
                    optstream >> var;
                    if (var.length() == 1) {
                        // special case for PTGUI
                        var += "0";
                    }
                    // find first numerical character
                    std::string::size_type np = var.find_first_of("0123456789");
                    if (np == std::string::npos) {
                        // invalid, continue
                        continue;
                    }
                    std::string name=var.substr(0,np);
                    std::string number = var.substr(np);
                    unsigned int nr = hugin_utils::lexical_cast<unsigned int>(number);
                    DEBUG_ASSERT(nr < optvec.size());
                    if(nr < optvec.size())
                    {
                        optvec[nr].insert(name);
                        DEBUG_DEBUG("parsing opt: >" << var << "< : var:" << name << " image:" << nr);
                    };
                }
            }
            break;
        }
        case 'c':
        {
            DEBUG_DEBUG("c line: " << line);
            int t;
            // read control points
            ControlPoint point;
	    // TODO - should verify that line syntax is correct
            getIntParam(point.image1Nr, line, "n");
            point.image1Nr += ctrlPointsImgNrOffset;
            getIntParam(point.image2Nr, line, "N");
            point.image2Nr += ctrlPointsImgNrOffset;
            getDoubleParam(point.x1, line, "x");
            getDoubleParam(point.x2, line, "X");
            getDoubleParam(point.y1, line, "y");
            getDoubleParam(point.y2, line, "Y");
            if (!getIntParam(t, line, "t") ){
                t = 0;
            }

            point.mode = t;
            ctrlPoints.push_back(point);
            state = P_CP;
            break;
        }

        // handle the complicated part.. the image & lens settings.
        // treat i and o lines the same.. however, o lines have priority
        // over i lines.(i lines often do not contain link information!)
        case 'i':
        {
            if (PTGUILensLine) {
                PTGUILensLine = false;
                PTGUILensLoaded = true;
                PTGUILens.parse(line);
            } else {
                iImgInfo.push_back(ImgInfo(line));
            }
            break;
        }
        case 'o':
        {
            if (PTGUILensLine) {
                PTGUILensLine = false;
                PTGUILensLoaded = true;
                PTGUILens.parse(line);
            } else {
                oImgInfo.push_back(ImgInfo(line));
            }
            break;
        }

        case 'k':
        {
            unsigned int param;
            if (getIntParam(param,line,"i"))
            {
                MaskPolygon newPolygon;
                newPolygon.setImgNr(param);
                if (getIntParam(param,line,"t"))
                    newPolygon.setMaskType((HuginBase::MaskPolygon::MaskType)param);
                std::string format;
                if (getPTParam(format,line,"p"))
                {
                    if(newPolygon.parsePolygonString(format))
                        ImgMasks.push_back(newPolygon);
                };
            };
            break;
        }

        case '#':
        {
            // parse special comments...
            if (line.substr(0,20) == string("# ptGui project file")) {
                PTGUIScriptFile = true;
            }
            if (line.substr(0,12) == "#-dummyimage") {
                PTGUILensLine = true;
            }
            if (PTGUIScriptFile) {
                // parse special PTGUI stuff.
                if (sscanf(line.c_str(), "#-fileversion %d", &PTGUIScriptVersion) > 0) {
                    DEBUG_DEBUG("Detected PTGUI script version: " << PTGUIScriptVersion);
                    switch (PTGUIScriptVersion) {
                        case 0:
                            break;
                        case 1:
                            break;
                        case 2:
                            break;
                        case 3:
                            break;
                        case 4:
                            break;
                        case 5:
                            break;
                        case 6:
                            break;
                        case 7:
                            break;
                        default:
                            ctrlPointsImgNrOffset = -1;
                            // latest known version is 8
                            break;
                    }
                }
            }

            if (line.substr(0,8) == "#-hugin ") {
                // read hugin image line
                ImgInfo info;
                info.autoCenterCrop = (line.find("autoCenterCrop=1") != std::string::npos);
                size_t pos = line.find("cropFactor=");
                if (pos > 0 && pos < line.length()) {
                    double cropFactor=1;
                    const char * s = line.c_str() + pos;
                    sscanf(s,"cropFactor=%lf", & cropFactor);
                    if(cropFactor<0.01 || cropFactor > 100)
                        cropFactor=1;
                    info.cropFactor = cropFactor;
                }
                pos = line.find("disabled");
                if (pos > 0 && pos < line.length()) {
                    info.enabled = false;
                }
                huginImgInfo.push_back(info);
            }

            // PTGui and PTAssember project files:
            // #-imgfile 960 1280 "D:\data\bruno\074-098\087.jpg"
            if (line.substr(0,10) == "#-imgfile ") {

                // arghhh. I like string processing without regexps.
                int b = line.find_first_not_of(" ",9);
                int e = line.find_first_of(" ",b);
                DEBUG_DEBUG(" width:" << line.substr(b,e-b)<<":")
                int nextWidth = hugin_utils::lexical_cast<int,string>(line.substr(b,e-b));
                DEBUG_DEBUG("next width " << nextWidth);
                b = line.find_first_not_of(" ",e);
                e = line.find_first_of(" ",b);
                DEBUG_DEBUG(" height:" << line.substr(b,e-b)<<":")
                int nextHeight = hugin_utils::lexical_cast<int, string>(line.substr(b,e-b));
                DEBUG_DEBUG("next height " << nextHeight);

                string nextFilename;
                try {
                    b = line.find_first_not_of(" \"",e);
                    e = line.find_first_of("\"",b);
                    nextFilename = line.substr(b,e-b);
                } catch (std::out_of_range& e) {
                    DEBUG_ERROR("ERROR PARSING INPUT FILE" << e.what( ));
                    return false;
                }
                DEBUG_DEBUG("next filename " << nextFilename);

                ImgInfo info;
                info.width  = nextWidth;
                info.height = nextHeight;
                info.filename = nextFilename;
                cImgInfo.push_back(info);
            }


            // parse hugin properties
            if (line.substr(0,7) == "#hugin_") {
                istringstream is(line);
                string var,value;
                is >> var >> value;
                if (!is.fail()) {
                    if (var == "#hugin_ptoversion") {
                        ptoVersion = atoi(value.c_str());
                    }

                    if (var == "#hugin_optimizeReferenceImage") {
                        options.optimizeReferenceImage = atoi(value.c_str());
                    } else if (var == "#hugin_remapper") {
                        if (value == "nona") {
                            options.remapper = PanoramaOptions::NONA;
                        } else if (value == "PTmender") {
                            options.remapper = PanoramaOptions::PTMENDER;
                        }
                    } else if (var == "#hugin_blender") {
                        if (value == "none") {
                            options.blendMode = PanoramaOptions::NO_BLEND;
                        } else if (value == "PTblender") {
                            options.blendMode = PanoramaOptions::PTBLENDER_BLEND;
                        } else if (value == "enblend") {
                            options.blendMode = PanoramaOptions::ENBLEND_BLEND;
                        } else if (value == "PTmasker") {
                            options.blendMode = PanoramaOptions::PTMASKER_BLEND;
                        } else if (value == "smartblend") {
                            options.blendMode = PanoramaOptions::SMARTBLEND_BLEND;
                        }

                    } else if (var == "#hugin_enblendOptions") {
                        options.enblendOptions = value;
                        while (!is.eof()) {
                            is >> value;
                            if (value.length() > 0) {
                                options.enblendOptions += " ";
                                options.enblendOptions += value;
                            }
                        }
                    } else if (var == "#hugin_enfuseOptions") {
                        options.enfuseOptions = value;
                        while (!is.eof()) {
                            is >> value;
                            if (value.length() > 0) {
                                options.enfuseOptions += " ";
                                options.enfuseOptions += value;
                            }
                        }
                    } else if (var == "#hugin_hdrmergeOptions") {
                        options.hdrmergeOptions = value;
                        while (!is.eof()) {
                            is >> value;
                            if (value.length() > 0) {
                                options.hdrmergeOptions += " ";
                                options.hdrmergeOptions += value;
                            }
                        }

                    } else if (var == "#hugin_outputLDRBlended") {
                        options.outputLDRBlended = (value == "true");
                    } else if (var == "#hugin_outputLDRLayers") {
                        options.outputLDRLayers = (value == "true");
                    } else if (var == "#hugin_outputLDRExposureRemapped") {
                        options.outputLDRExposureRemapped = (value == "true");
                    } else if (var == "#hugin_outputLDRExposureLayers") {
                        options.outputLDRExposureLayers = (value == "true");
                    } else if (var == "#hugin_outputLDRExposureBlended") {
                        options.outputLDRExposureBlended = (value == "true");
                    } else if (var == "#hugin_outputLDRExposureLayersFused") {
                        options.outputLDRExposureLayersFused = (value == "true");
                    } else if (var == "#hugin_outputLDRStacks") {
                        options.outputLDRStacks = (value == "true");
                    } else if (var == "#hugin_outputHDRBlended") {
                        options.outputHDRBlended = (value == "true");
                    } else if (var == "#hugin_outputHDRLayers") {
                        options.outputHDRLayers = (value == "true");
                    } else if (var == "#hugin_outputHDRStacks") {
                        options.outputHDRStacks = (value == "true");

                    } else if (var == "#hugin_outputStacksMinOverlap") {
                        double val=atof(value.c_str());
                        if(val>0 && val <= 1)
                        {
                            options.outputStacksMinOverlap = val;
                        };
                    } else if (var == "#hugin_outputLayersExposureDiff") {
                        double val=atof(value.c_str());
                        if(val>0.01)
                        {
                            options.outputLayersExposureDiff = val;
                        }

                    } else if (var == "#hugin_outputLayersCompression") {
                        options.outputLayersCompression = value;
                    } else if (var == "#hugin_outputImageType") {
                        options.outputImageType = value;
                    } else if (var == "#hugin_outputImageTypeCompression") {
                        options.outputImageTypeCompression = value;
                    } else if (var == "#hugin_outputJPEGQuality") {
                        options.quality = atoi(value.c_str());
                    } else if (var == "#hugin_outputImageTypeHDR") {
                        options.outputImageTypeHDR = value;
                    } else if (var == "#hugin_outputImageTypeHDRCompression") {
                        options.outputImageTypeHDRCompression = value;
                    } else if (var == "#hugin_optimizerMasterSwitch") {
                        optSwitch = atoi(value.c_str());
                    } else if (var == "#hugin_optimizerPhotoMasterSwitch") {
                        optPhotoSwitch = atoi(value.c_str());
                    };

                }
            }
            break;
        }

        } // case
    }

    // assemble images from the information read before..

/** @todo What is the PTGUI special case? What images use the lens created here?
 */
#if 0
    // handle PTGUI special case
    if (PTGUILensLoaded) {
        // create lens with dummy info
        Lens l;
        for (const char **v = Lens::variableNames; *v != 0; v++) {
            map_get(l.variables, *v).setValue(PTGUILens.vars[*v]);
        }
        l.setImageSize(vigra::Size2D(PTGUILens.width, PTGUILens.height));
        l.setCropFactor(1);
        l.setProjection((Lens::LensProjectionFormat) PTGUILens.f);
        lenses.push_back(l);
    }
#endif

/*
    // ugly hack to load PTGui script files
    if (ptGUIDummyImage) {
        DEBUG_DEBUG("loading default PTGUI line: " << line);
            Lens l;
            // skip ptgui's dummy image
            // load parameters into default lens...
            for (LensVarMap::iterator it = l.variables.begin();
             it != l.variables.end();
             ++it)
            {
                DEBUG_DEBUG("reading default lens variable " << it->first);
                int link;
                bool ok = readVar(it->second, link, line);
                DEBUG_ASSERT(ok);
                DEBUG_ASSERT(link == -1);
            }
            lenses.push_back(l);

            ptGUIDummyImage = false;
            break;
        }
*/

    // merge image info from the 3 different lines...
    // i lines are the main reference.

    int nImgs = iImgInfo.size();
    int nOLines = oImgInfo.size();
    int nCLines = cImgInfo.size();

    if (nImgs < nOLines) {
        // no, or less i lines found. scrap i lines.
        DEBUG_DEBUG("throwing away " << nImgs << " i lines");
        iImgInfo = oImgInfo;
        nImgs = nOLines;
    }
    if (nOLines < nImgs) {
        oImgInfo = iImgInfo;
    }

    // merge o lines and i lines into i lines.
    for (int i=0; i < nImgs; i++) {

        // move parameters from o lines -> i (only if it isn't given in the
        // i lines. or it is linked on the o lines)

        // ordinary variables
        for (const char ** v = ImgInfo::varnames; *v ; v++) {

            if (iImgInfo[i].links[*v] == -2 && oImgInfo[i].links[*v] != -2 || iImgInfo[i].links[*v] == -1 && oImgInfo[i].links[*v] >=0) {
                DEBUG_DEBUG(*v << ": o -> i");
                iImgInfo[i].vars[*v] = oImgInfo[i].vars[*v];
                iImgInfo[i].links[*v] = oImgInfo[i].links[*v];
            }
        }

        if (iImgInfo[i].filename == "" && oImgInfo[i].filename != "") {
            DEBUG_DEBUG("filename: o -> i");
            iImgInfo[i].filename = oImgInfo[i].filename;
        }

        if (iImgInfo[i].crop.isEmpty() && !oImgInfo[i].crop.isEmpty()) {
            DEBUG_DEBUG("crop: o -> i");
            iImgInfo[i].crop = oImgInfo[i].crop;
        }

        if (iImgInfo[i].width <= 0 && oImgInfo[i].width > 0) {
            DEBUG_DEBUG("width: o -> i");
            iImgInfo[i].width = oImgInfo[i].width;
        }

        if (iImgInfo[i].height <= 0 && oImgInfo[i].height > 0) {
            DEBUG_DEBUG("height: o -> i");
            iImgInfo[i].height = oImgInfo[i].height;
        }

        if (iImgInfo[i].f < 0 && oImgInfo[i].f > 0) {
            DEBUG_DEBUG("f: o -> i");
            iImgInfo[i].f = oImgInfo[i].f;
        }

        if (nCLines == nImgs) {
            // img file & size in clines
            if (cImgInfo[i].filename != "" && cImgInfo[i].width > 0) {
                DEBUG_DEBUG("filename, width, height: c -> i");
                iImgInfo[i].filename = cImgInfo[i].filename;
                iImgInfo[i].width = cImgInfo[i].width;
                iImgInfo[i].height = cImgInfo[i].height;
            }
        }
        if (huginImgInfo.size() == (size_t)nImgs) {
            iImgInfo[i].cropFactor = huginImgInfo[i].cropFactor;
            iImgInfo[i].autoCenterCrop = huginImgInfo[i].autoCenterCrop;
            iImgInfo[i].enabled= huginImgInfo[i].enabled;
        }
    }

    // create images.
    for (int i=0; i < nImgs; i++) {

        DEBUG_DEBUG("i line: " << i);
        // read the variables
        VariableMap vars;
        int link = -2;
        fillVariableMap(vars);

        for (const char ** v = ImgInfo::varnames; *v != 0; v++) {
            std::string name(*v);
            double val = iImgInfo[i].vars[*v];
            map_get(vars,name).setValue(val);
            if (iImgInfo[i].links[*v] >= 0) {
                link = iImgInfo[i].links[*v];
            }
        }
        
        string file = iImgInfo[i].filename;
        // add prefix if only a relative path.
#ifdef WIN32
        bool absPath = ( (file[1]==':' && file[2]=='\\') || (file[1]==':' && file[2]=='/') || (file[0] == '\\' && file[1] == '\\'));
#else
        bool absPath = file[0] == '/';
#endif
        if (!absPath) {
            file.insert(0, prefix);
        }
        DEBUG_DEBUG("filename: " << file);
        
        // Make a new SrcPanoImage in this list for expressing this one.
        SrcPanoImage * new_img_p = new SrcPanoImage();
        images.push_back(new_img_p);
        // and make a reference to it so we dont keep using images.back(),
        SrcPanoImage & new_img = *new_img_p;
        new_img.setFilename(file);
        double focal=0;
        double crop=0;
        new_img.readEXIF(focal,crop,false,false);
        new_img.setSize(vigra::Size2D(iImgInfo[i].width, iImgInfo[i].height));
        
        // Panotools Script variables for the current SrcPanoImage variable.
        // We just need the names.
        VariableMap vars_for_name;
        
        // A dummy SrcPanoImage to use to fill vars_for_name.
        SrcPanoImage name_src;
        
/* Set image variables in new_img using the PanoTools Script names, with
 * linking where specified in the file.
 * 
 * We create a list of PanoTools script variable names for each variable in
 * SrcPanoImg (vars_for_name). It may have multiple variables or none at all.
 * If a link is found between any of the variables inside it we can link them up
 * in new_img.
 * 
 * The Panorama Tools script format makes it possible to link parts of the same
 * SrcPanoImage variable differently (e.g. you can link the horizontal component
 * of shearing to a different target to the vertical component, not link the
 * vertical component at all). SrcPanoImage cannot handle this, so we end up
 * linking all components of the same variable to anything specified in the PTO
 * script for one of the components: most often we specify the same link
 * multiple times.
 */
/** @todo Warn the user when the script links variables in a way not expressable
 * by SrcPanoImage.
 */
#define RESET_LOCALE setlocale(LC_NUMERIC,old_locale); free(old_locale);
#define image_variable( name, type, default_value )\
        PTOVariableConverterFor##name::addToVariableMap(name_src.get##name##IV(), vars_for_name);\
        for (VariableMap::iterator vit = vars_for_name.begin();\
             vit != vars_for_name.end(); vit++)\
        {\
            if (link >= 0 && iImgInfo[i].links[vit->first] >= 0)\
            {\
                if (   !(PTGUILensLoaded && link == 0)\
                    && (int) images.size() < link && (!PTGUILensLoaded))\
                {\
                    DEBUG_ERROR("variables must be linked to an image with a lower number" << endl\
                                << "number links: " << link << " images: " << images.size() << endl\
                                << "error on line " << lineNr << ":" << endl\
                                << line);\
                    RESET_LOCALE\
                    return false;\
                }\
                DEBUG_DEBUG("anchored to image " << iImgInfo[i].links[vit->first]);\
                new_img.link##name(images[iImgInfo[i].links[vit->first]]);\
            } else {\
                double val = map_get(vars, vit->first).getValue();\
                new_img.setVar(vit->first, val);\
            }\
        }\
        vars_for_name.clear();
#include "image_variables.h"
#undef image_variable
        new_img.setProjection((SrcPanoImage::Projection) iImgInfo[i].f);

        // check, if stacks are correctly linked
#define check_stack_link(name) \
        if(!new_img.YawisLinked() && new_img.name##isLinked())\
        {\
            new_img.unlink##name();\
        };\
        if(new_img.YawisLinked() && !new_img.name##isLinked())\
        {\
            for(size_t j=0; j<i; j++)\
            {\
                if(new_img.YawisLinkedWith(*images[j]))\
                {\
                    new_img.link##name(images[j]);\
                    break;\
                };\
            };\
        }
        check_stack_link(Pitch);
        check_stack_link(Roll);
        check_stack_link(X);
        check_stack_link(Y);
        check_stack_link(Z);
        check_stack_link(TranslationPlaneYaw);
        check_stack_link(TranslationPlanePitch);
#undef check_stack_link

#if 0
        new_img.setFeatherWidth((unsigned int) iImgInfo[i].blend_radius);
#endif
        
        // is this right?
        new_img.setExifCropFactor(iImgInfo[i].cropFactor);
        new_img.setVigCorrMode(iImgInfo[i].vigcorrMode);
        new_img.setFlatfieldFilename(iImgInfo[i].flatfieldname);
        new_img.setResponseType((SrcPanoImage::ResponseType)iImgInfo[i].responseType);
        new_img.setAutoCenterCrop(iImgInfo[i].autoCenterCrop);
        new_img.setActive(iImgInfo[i].enabled);
        
        if (!iImgInfo[i].crop.isEmpty()) {
            if (new_img.isCircularCrop())
            {
                new_img.setCropMode(SrcPanoImage::CROP_CIRCLE);
            } else {
                new_img.setCropMode(SrcPanoImage::CROP_RECTANGLE);
            }
            new_img.setCropRect(iImgInfo[i].crop);
        }

        //now fill the mask
        for(unsigned int j=0;j<ImgMasks.size();j++)
            if(ImgMasks[j].getImgNr()==i)
                //now clip mask to image size + offset
                if(ImgMasks[j].clipPolygon(vigra::Rect2D(-0.9*maskOffset,-0.9*maskOffset,
                    new_img.getWidth()+0.9*maskOffset,new_img.getHeight()+0.9*maskOffset)))
                {
                    new_img.addMask(ImgMasks[j]);
                };
    }
    
    // if we haven't found a v line in the project file
    if (optvec.size() != images.size()) {
        optvec = OptimizeVector(images.size());
    }

    // reset locale
    setlocale(LC_NUMERIC,old_locale);
    free(old_locale);

    return true;
}

} // namespace

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

#include <fstream>
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
    state.lenses.clear();
    state.images.clear();
    state.variables.clear();
    state.options.reset();
    state.needsOptimization = false;
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
    state.needsOptimization = true;
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
    // create empty optimisation vector
    state.optvec.push_back(std::set<std::string>());
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
    // TODO: recenter crop, if required.
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
        o << "# hugin project file, version 1" << std::endl;
    }
    // output options..

    output.printScriptLine(o, forPTOptimizer);

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
        PanoImage  img = state.images[imgNr];
        unsigned int lensNr = img.getLensNr();
        Lens lens = state.lenses[lensNr];
        const VariableMap & vars = state.variables[imgNr];
        ImageOptions iopts = img.getOptions();

        // print special comment line with hugin GUI data
        o << "#-hugin ";
        if (iopts.docrop) {
            if (iopts.autoCenterCrop)
                o << " autoCenterCrop=1";
        }
        o << " cropFactor=" << lens.getCropFactor() << std::endl;
        
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
//                    DEBUG_DEBUG("printing link: " << vit->first);
                    // print link, anchor variable was already printed
                    map_get(lens.variables,vit->first).printLink(o,linkAnchors[lensNr]) << " ";
                } else {
//                    DEBUG_DEBUG("printing value for linked var " << vit->first);
                    // first time, print value
                    linkAnchors[lensNr] = imageNrMap[imgNr];

                    if ( ( (vit->first == "a" && set_contains(optvars[imgNr], "a") )|| 
                           (vit->first == "b" && set_contains(optvars[imgNr], "b") )|| 
                           (vit->first == "c" && set_contains(optvars[imgNr], "c") )
                         )
                        && forPTOptimizer && vit->second.getValue() == 0.0)
                    {
                        // work around a bug in PTOptimizer, a,b,c values will only be 
                        // optmized if nonzero
                        o << vit->first << 1e-5 << " ";
                    } else {
                        vit->second.print(o) << " ";
                    }
                }
            } else {
                if (( (vit->first == "a" && set_contains(optvars[imgNr], "a") )|| 
                      (vit->first == "b" && set_contains(optvars[imgNr], "b") )|| 
                      (vit->first == "c" && set_contains(optvars[imgNr], "c") )
                    )
                    && forPTOptimizer && vit->second.getValue() == 0.0) 
                {
                    // work around a bug in PTOptimizer, a,b,c values will only be optmized
                    // if nonzero
                    o << vit->first << 1e-5 << " ";
                } else {
                    vit->second.print(o) << " ";
                }
            }
        }

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
            if (iopts.responseType > 0) {
                o << " Rt" << iopts.responseType;
            }
        }

        o << " u" << output.featherWidth
          << (img.getOptions().morph ? " o" : "");
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
      << "v ";

    int optVarCounter=0;
    // be careful. linked variables should not be specified multiple times.
    std::vector<std::set<std::string> > linkvars(state.lenses.size());

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

    o << "#hugin_outputLDRBlended " << (output.outputLDRBlended ? "true" : "false") << endl;
    o << "#hugin_outputLDRLayers " << (output.outputLDRLayers ? "true" : "false") << endl;
    o << "#hugin_outputLDRExposureRemapped " << (output.outputLDRExposureRemapped ? "true" : "false") << endl;
    o << "#hugin_outputLDRExposureLayers " << (output.outputLDRExposureLayers ? "true" : "false") << endl;
    o << "#hugin_outputLDRExposureBlended " << (output.outputLDRExposureBlended ? "true" : "false") << endl;
    o << "#hugin_outputHDRBlended " << (output.outputHDRBlended ? "true" : "false") << endl;
    o << "#hugin_outputHDRLayers " << (output.outputHDRLayers ? "true" : "false") << endl;
    o << "#hugin_outputHDRStacks " << (output.outputHDRStacks ? "true" : "false") << endl;    

    o << "#hugin_outputImageType " << output.outputImageType << endl;
    o << "#hugin_outputImageTypeCompression " << output.outputImageTypeCompression << endl;
    o << "#hugin_outputImageTypeHDR " << output.outputImageTypeHDR << endl;
    o << "#hugin_outputImageTypeHDRCompression " << output.outputImageTypeHDRCompression << endl;

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
    target.printScriptLine(o);
    o << std::endl
      << "# output image lines" << std::endl;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end(); ++imgNrIt) {
        unsigned int imgNr = *imgNrIt;
        const PanoImage & img = state.images[imgNr];
        unsigned int lensNr = img.getLensNr();
// DGSW FIXME - Unreferenced
//		const Lens & lens = state.lenses[lensNr];
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
        AppBase::DocumentData::setDirty(dirty);
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
    using namespace hugin_utils;
    
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
                    opts.cropRect.setUpperLeft(vigra::Point2D(left, opts.cropRect.top()));
                    opts.cropRect.setLowerRight(vigra::Point2D(right, opts.cropRect.bottom()));
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
                    opts.cropRect.setUpperLeft(vigra::Point2D(opts.cropRect.left(), top));
                    opts.cropRect.setLowerRight(vigra::Point2D(opts.cropRect.right(), bottom));
                    state.images[i].setOptions(opts);
                }
            }

        }
    }
    state.needsOptimization = true;
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
    removeUnusedLenses();
    state.needsOptimization = true;
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
    state.needsOptimization = true;
}

unsigned int Panorama::addLens(const Lens & lens)
{
    state.lenses.push_back(lens);
    return state.lenses.size() - 1;
}

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
    
    state = PanoramaMemento(memento);
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

/*
int Panorama::addImageAndLens(const std::string & filename, double HFOV)
{
    // load image
    vigra::ImageImportInfo img(filename.c_str());

    SrcPanoImage img(filename);

    double 
    int matchingLensNr=-1;
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
    map_get(vars,"r").setValue(roll);

    DEBUG_ASSERT(matchingLensNr >= 0);
    PanoImage pimg(filename, img.width(), img.height(), (unsigned int) matchingLensNr);
    return addImage(pimg, vars);
}
*/

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

SrcPanoImage Panorama::getSrcImage(unsigned imgNr) const
{
    DEBUG_ASSERT(imgNr < state.images.size());
    const PanoImage & img = state.images[imgNr];
    const ImageOptions & opts = img.getOptions();
    const Lens & lens = state.lenses[img.getLensNr()];
    const VariableMap & vars = getImageVariables(imgNr);
    SrcPanoImage ret;
    ret.setFilename(img.getFilename());
    ret.setSize(vigra::Size2D(img.getWidth(), img.getHeight()));
    ret.setLensNr(img.getLensNr());
    ret.setProjection((SrcPanoImage::Projection) lens.getProjection());
    ret.setExifCropFactor(lens.getCropFactor());
    ret.setExifFocalLength(lens.getFocalLength());
    ret.setHFOV(const_map_get(vars,"v").getValue());
    ret.setRoll(const_map_get(vars,"r").getValue());
    ret.setPitch(const_map_get(vars,"p").getValue());
    ret.setYaw(const_map_get(vars,"y").getValue());

    // geometrical distortion correction
    std::vector<double> radialDist(4);
    radialDist[0] = const_map_get(vars,"a").getValue();
    radialDist[1] = const_map_get(vars,"b").getValue();
    radialDist[2] = const_map_get(vars,"c").getValue();
    radialDist[3] = 1 - radialDist[0] - radialDist[1] - radialDist[2];
    ret.setRadialDistortion(radialDist);
    FDiff2D t;
    t.x = const_map_get(vars,"d").getValue();
    t.y = const_map_get(vars,"e").getValue();
    ret.setRadialDistortionCenterShift(t);
    t.x = const_map_get(vars,"g").getValue();
    t.y = const_map_get(vars,"t").getValue();
    ret.setShear(t);

    // vignetting
    ret.setVigCorrMode(opts.m_vigCorrMode);
    ret.setFlatfieldFilename(opts.m_flatfield);
    std::vector<double> vigCorrCoeff(4);
    vigCorrCoeff[0] = const_map_get(vars,"Va").getValue();
    vigCorrCoeff[1] = const_map_get(vars,"Vb").getValue();
    vigCorrCoeff[2] = const_map_get(vars,"Vc").getValue();
    vigCorrCoeff[3] = const_map_get(vars,"Vd").getValue();
    ret.setRadialVigCorrCoeff(vigCorrCoeff);
    t.x = const_map_get(vars,"Vx").getValue();
    t.y = const_map_get(vars,"Vy").getValue();
    ret.setRadialVigCorrCenterShift(t);

    // exposure and white balance parameters
    ret.setExposureValue(const_map_get(vars,"Eev").getValue());
    ret.setWhiteBalanceRed(const_map_get(vars,"Er").getValue());
    ret.setWhiteBalanceBlue(const_map_get(vars,"Eb").getValue());

    // camera response parameters
    DEBUG_DEBUG("opts.resp: " << ((SrcPanoImage::ResponseType)opts.responseType));
    ret.setResponseType((SrcPanoImage::ResponseType) opts.responseType);
    DEBUG_DEBUG("ret.resp (after set): " << ret.getResponseType());

    std::vector<float> ep(5);
    ep[0] = const_map_get(vars,"Ra").getValue();
    ep[1] = const_map_get(vars,"Rb").getValue();
    ep[2] = const_map_get(vars,"Rc").getValue();
    ep[3] = const_map_get(vars,"Rd").getValue();
    ep[4] = const_map_get(vars,"Re").getValue();
    ret.setEMoRParams(ep);

    // crop
    if (!opts.docrop) {
        ret.setCropMode(SrcPanoImage::NO_CROP);
    } else if (ret.getProjection() == SrcPanoImage::CIRCULAR_FISHEYE) {
        ret.setCropMode(SrcPanoImage::CROP_CIRCLE);
        ret.setCropRect(opts.cropRect);
    } else {
        ret.setCropMode(SrcPanoImage::CROP_RECTANGLE);
        ret.setCropRect(opts.cropRect);
    }

    ret.setGamma(state.options.gamma);

    return ret;
}

void Panorama::setSrcImage(unsigned int imgNr, const SrcPanoImage & img)
{
    using namespace std;
    
    // get variable map vector
    VariableMap vars;
    DEBUG_ASSERT(imgNr < state.images.size());
    PanoImage & pimg = state.images[imgNr];
    ImageOptions opts = pimg.getOptions();
    Lens & lens = state.lenses[pimg.getLensNr()];

    
    // fill variable map
    // position
    vars.insert(make_pair("v", Variable("v", img.getHFOV())));
    vars.insert(make_pair("r", Variable("r", img.getRoll())));
    vars.insert(make_pair("p", Variable("p", img.getPitch())));
    vars.insert(make_pair("y", Variable("y", img.getYaw())));
    // distortion
    vars.insert(make_pair("a", Variable("a", img.getRadialDistortion()[0])));
    vars.insert(make_pair("b", Variable("b", img.getRadialDistortion()[1])));
    vars.insert(make_pair("c", Variable("c", img.getRadialDistortion()[2])));
    vars.insert(make_pair("d", Variable("d", img.getRadialDistortionCenterShift().x)));
    vars.insert(make_pair("e", Variable("e", img.getRadialDistortionCenterShift().y)));
    vars.insert(make_pair("g", Variable("g", img.getShear().x)));
    vars.insert(make_pair("t", Variable("t", img.getShear().y)));

    // vignetting correction
    vars.insert(make_pair("Va", Variable("Va", img.getRadialVigCorrCoeff()[0])));
    vars.insert(make_pair("Vb", Variable("Vb", img.getRadialVigCorrCoeff()[1])));
    vars.insert(make_pair("Vc", Variable("Vc", img.getRadialVigCorrCoeff()[2])));
    vars.insert(make_pair("Vd", Variable("Vd", img.getRadialVigCorrCoeff()[3])));
    vars.insert(make_pair("Vx", Variable("Vx", img.getRadialVigCorrCenterShift().x)));
    vars.insert(make_pair("Vy", Variable("Vy", img.getRadialVigCorrCenterShift().y)));

    // exposure and white balance
    vars.insert(make_pair("Eev", Variable("Eev", img.getExposureValue())));
    vars.insert(make_pair("Er",  Variable("Er",  img.getWhiteBalanceRed())));
    vars.insert(make_pair("Eb",  Variable("Eb",  img.getWhiteBalanceBlue())));

    // camera response parameters
    opts.responseType = img.getResponseType();
    vars.insert(make_pair("Ra", Variable("Ra", img.getEMoRParams()[0])));
    vars.insert(make_pair("Rb", Variable("Rb", img.getEMoRParams()[1])));
    vars.insert(make_pair("Rc", Variable("Rc", img.getEMoRParams()[2])));
    vars.insert(make_pair("Rd", Variable("Rd", img.getEMoRParams()[3])));
    vars.insert(make_pair("Re", Variable("Re", img.getEMoRParams()[4])));

    // set variables
    updateVariables(imgNr, vars);

    // update lens
    lens.setProjection((Lens::LensProjectionFormat)img.getProjection());
    lens.setImageSize(img.getSize());
    lens.setCropFactor(img.getExifCropFactor());

    // update image
    pimg.setFilename(img.getFilename());
    pimg.setSize(img.getSize());
    pimg.setLensNr(img.getLensNr());

    // update image options
    if (img.getCropMode() == SrcPanoImage::NO_CROP) {
        opts.docrop = false;
    } else {
        opts.docrop = true;
    }
    opts.cropRect = img.getCropRect();
    opts.m_vigCorrMode = img.getVigCorrMode();
    opts.m_flatfield = img.getFlatfieldFilename();
    opts.responseType = img.getResponseType();
    setImageOptions(imgNr, opts);
    imageChanged(imgNr);
}


Panorama Panorama::duplicate() const
{
    Panorama pano(*this);
    pano.observers.clear();
    return pano;
}

Panorama Panorama::getSubset(const UIntSet & imgs) const
{
    Panorama subset(*this);
    // clear listeners!
    subset.observers.clear();

    // create image number map.
    std::map<unsigned int, unsigned int> imageNrMap;

    // copy images variables and lenses..
    subset.state.images.clear();
    subset.state.variables.clear();
    subset.state.optvec.clear();

    unsigned int ic = 0;
    for (UIntSet::const_iterator imgNrIt = imgs.begin(); imgNrIt != imgs.end();
         ++imgNrIt)
    {
        subset.state.images.push_back(state.images[*imgNrIt]);
        subset.state.variables.push_back(state.variables[*imgNrIt]);
        subset.state.optvec.push_back(state.optvec[*imgNrIt]);
        imageNrMap[*imgNrIt] = ic;
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

    return subset;
}

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
    if (newPano.loadPTScript(dataInput, getFilePrefix())) {
        
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




bool PanoramaMemento::loadPTScript(std::istream &i, const std::string &prefix)
{
    using namespace std;
    using namespace PTScriptParsing;
    
    DEBUG_TRACE("");
#ifdef __unix__
    // set numeric locale to C, for correct number output
    char * old_locale = setlocale(LC_NUMERIC,NULL);
    setlocale(LC_NUMERIC,"C");
#endif
    PTParseState state;
    string line;

    // vector with the different information lines about images
    vector<ImgInfo> oImgInfo;
    vector<ImgInfo> iImgInfo;
    // strange comment informations.
    vector<ImgInfo> cImgInfo;
    // hugin additional information
    vector<ImgInfo> huginImgInfo;

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
    options.blendMode = PanoramaOptions::ENBLEND_BLEND;
    options.remapper = PanoramaOptions::NONA;
    options.outputLDRBlended = true;
    options.outputLDRLayers = false;
    options.outputLDRExposureLayers = false;
    options.outputHDRBlended = false;
    options.outputHDRLayers = false;
    options.outputHDRStacks = false;

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
        // check for a known line
        switch(line[0]) {
        case 'p':
        {
            DEBUG_DEBUG("p line: " << line);
            int i=0;
            getIntParam(i,line,"f");
            options.setProjection( (PanoramaOptions::ProjectionFormat) i );
            unsigned int w=800;
            getIntParam(w, line, "w");
            options.setWidth(w);
            double v=50;
            getDoubleParam(v, line, "v");
            options.setHFOV(v, false);
            int height=600;
            getIntParam(height, line, "h");
            options.setHeight(height);

            double newE=0;
            getDoubleParam(newE, line, "E");
            options.outputExposureValue = newE;
            int ar=0;
            getIntParam(ar, line, "R");
            options.outputMode = (PanoramaOptions::OutputMode) ar;

            string format;
            getPTStringParam(format,line,"T");
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
            if (getPTStringParam(format,line,"P")) {
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
            if (getPTStringParam(format,line,"n")) {
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
                        getIntParam(coordImgs, format, "p");
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
                        if (getPTStringParamColon(comp, format, "c")) {
                            if (comp == "NONE" || comp == "LZW" ||
                                comp == "DEFLATE") 
                            {
                                options.tiffCompression = comp;
                            } else {
                                DEBUG_WARN("No valid tiff compression found");
                            }
                        }
                        // read tiff roi
                        if (getPTStringParamColon(comp, format, "r")) {
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
            int i=0;
            getIntParam(i,line,"i");
            options.interpolator = (vigra_ext::Interpolator) i;
            getDoubleParam(options.gamma,line,"g");

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
                    optvec[nr].insert(name);
                    DEBUG_DEBUG("parsing opt: >" << var << "< : var:" << name << " image:" << nr);
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
                    info.cropFactor = cropFactor;
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
                b = line.find_first_not_of(" \"",e);
                e = line.find_first_of("\"",b);
                string nextFilename = line.substr(b,e-b);
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
                    } else if (var == "#hugin_outputHDRBlended") {
                        options.outputHDRBlended = (value == "true");
                    } else if (var == "#hugin_outputHDRLayers") {
                        options.outputHDRLayers = (value == "true");
                    } else if (var == "#hugin_outputHDRStacks") {
                        options.outputHDRStacks = (value == "true");

                    } else if (var == "#hugin_outputImageType") {
                        options.outputImageType = value;
                    } else if (var == "#hugin_outputImageTypeCompression") {
                        options.outputImageTypeCompression = value;
                    } else if (var == "#hugin_outputImageTypeHDR") {
                        options.outputImageTypeHDR = value;
                    } else if (var == "#hugin_outputImageTypeHDRCompression") {
                        options.outputImageTypeHDRCompression = value;
                    }
                }
            }
            break;
        }

        } // case
    }

    // assemble images & lenses from the information read before..

    // handle PTGUI special case
    if (PTGUILensLoaded) {
        // create lens with dummy info
        Lens l;
        for (char **v = Lens::variableNames; *v != 0; v++) {
            map_get(l.variables, *v).setValue(PTGUILens.vars[*v]);
        }
        l.setImageSize(vigra::Size2D(PTGUILens.width, PTGUILens.height));
        l.setCropFactor(1);
        l.setProjection((Lens::LensProjectionFormat) PTGUILens.f);
        lenses.push_back(l);
    }

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
        for (char ** v = ImgInfo::varnames; *v ; v++) {

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
        }
    }

    // create image and lens.
    for (int i=0; i < nImgs; i++) {

        DEBUG_DEBUG("i line: " << i);
        // read the variables & decide if to create a new lens or not
        VariableMap vars;
        int link = -2;
        fillVariableMap(vars);

        for (char ** v = ImgInfo::varnames; *v != 0; v++) {
            std::string name(*v);
            double val = iImgInfo[i].vars[*v];
            map_get(vars,name).setValue(val);
            DEBUG_ASSERT(link <0  || iImgInfo[i].links[*v] < 0|| link == iImgInfo[i].links[*v]);
            if (iImgInfo[i].links[*v] >= 0) {
                link = iImgInfo[i].links[*v];
            }
        }

        int width = iImgInfo[i].width;
        int height = iImgInfo[i].height;

        string file = iImgInfo[i].filename;
        // add prefix if only a relative path.
#ifdef WIN32
        bool absPath = (file[1]==':' && file[2]=='\\');
#else
        bool absPath = file[0] == '/';
#endif
        if (!absPath) {
            file.insert(0, prefix);
        }
        DEBUG_DEBUG("filename: " << file);

        Lens l;

        l.setImageSize(vigra::Size2D(iImgInfo[i].width, iImgInfo[i].height));
        l.setCropFactor(iImgInfo[i].cropFactor);

        int anchorImage = -1;
        int lensNr = -1;
        for (LensVarMap::iterator it = l.variables.begin();
            it != l.variables.end();
            ++it)
        {
            std::string varname = it->first;
            // default to unlinked variables, overwrite later, if found in script
            (*it).second.setLinked(false);

            DEBUG_DEBUG("reading variable " << varname << " link:" << link );
            if (link >=0 && iImgInfo[i].links[varname]>= 0) {
                // linked variable

                if (PTGUILensLoaded && link == 0) {
                    anchorImage = link;
                    // set value from lens variable
                    lensNr = 0;
                } else if ((int) images.size() <= link && (!PTGUILensLoaded)) {
                    DEBUG_ERROR("variables must be linked to an image with a lower number" << endl
                                << "number links: " << link << " images: " << images.size() << endl
                                << "error on line " << lineNr << ":" << endl
                                << line);
#ifdef __unix__
                    // reset locale
                    setlocale(LC_NUMERIC,old_locale);
#endif
                    return false;
                } else {
                    DEBUG_DEBUG("anchored to image " << link);
                    anchorImage = link;
                    // existing lens
                    lensNr = images[anchorImage].getLensNr();
                    DEBUG_DEBUG("using lens nr " << lensNr);
                }
                DEBUG_ASSERT(lensNr >= 0);
                // get variable value of the link target
                double val = map_get(lenses[lensNr].variables, varname).getValue();
                map_get(vars, varname).setValue(val);
                map_get(lenses[lensNr].variables, varname).setLinked(true);
                it->second.setValue(val);
            } else {
                DEBUG_DEBUG("image " << i << " not linked, link: " << link);
                // not linked
                // copy value to lens variable.
                double val = map_get(vars,varname).getValue();
                it->second.setValue(val);
            }
        }
        variables.push_back(vars);

        DEBUG_DEBUG("lensNr after scanning " << lensNr);
        //l.projectionFormat = (Lens::LensProjectionFormat) iImgInfo[i].f;
        l.setProjection((Lens::LensProjectionFormat) iImgInfo[i].f);

        if (lensNr != -1) {
    //                lensNr = images[anchorImage].getLensNr();
            if (l.getProjection() != lenses[lensNr].getProjection()) {
                DEBUG_ERROR("cannot link images with different projections");
    #ifdef __unix__
                // reset locale
                setlocale(LC_NUMERIC,old_locale);
    #endif
                return false;
            }
        }

        if (lensNr == -1) {
            // no links -> create a new lens
            // create a new lens.
            lenses.push_back(l);
            lensNr = lenses.size()-1;
        } else {
            // check if the lens uses landscape as well..
            if (lenses[(unsigned int) lensNr].isLandscape() != l.isLandscape()) {
                DEBUG_ERROR("Landscape and portrait images can't share a lens" << endl
                            << "error on script line " << lineNr << ":" << line);
            }
            // check if the ratio is equal
        }

        DEBUG_ASSERT(lensNr >= 0);
        DEBUG_DEBUG("adding image with lens " << lensNr);
        images.push_back(PanoImage(file,width, height, (unsigned int) lensNr));

        ImageOptions opts = images.back().getOptions();
        opts.featherWidth = (unsigned int) iImgInfo[i].blend_radius;
        if (!iImgInfo[i].crop.isEmpty()) {
            opts.docrop = true;
            opts.cropRect = iImgInfo[i].crop;
        }
        opts.m_vigCorrMode = iImgInfo[i].vigcorrMode;
        opts.m_flatfield = iImgInfo[i].flatfieldname;
        opts.responseType = iImgInfo[i].responseType;
        opts.autoCenterCrop = iImgInfo[i].autoCenterCrop;
        images.back().setOptions(opts);
    }

    // if we haven't found a v line in the project file
    if (optvec.size() != images.size()) {
        optvec = OptimizeVector(images.size());
    }
    return true;
}

} // namespace

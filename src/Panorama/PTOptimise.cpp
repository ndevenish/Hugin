// -*- c-basic-offset: 4 -*-

/** @file PTOptimise.cpp
 *
 *  @brief optimisation functions (wrappers around PTOptimizer)
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
#include <fstream>
#include <sstream>

#include "common/stl_utils.h"
#include "PT/PTOptimise.h"
#include "PT/ImageGraph.h"

#include <boost/graph/graphviz.hpp>
#include <boost/property_map.hpp>
#include <boost/graph/graph_utility.hpp>

#define DEBUG_WRITE_OPTIM_OUTPUT
#define DEBUG_WRITE_OPTIM_OUTPUT_FILE "hugin_debug_optim_results.txt"

using namespace std;
using namespace PT;
using namespace PTools;
using namespace boost;
using namespace utils;

// missing prototype in filter.h
extern "C" {
int CheckParams( AlignInfo *g );
}

/*
void PTools::optimize_PT(const Panorama & pano,
                               const PT::UIntVector &imgs,
                               const OptimizeVector & optvec,
                               VariableMapVector & vars,
                               CPVector & cps,
                               int maxIter)
{
//    VariableMapVector res;
    // setup data structures
    aPrefs    aP;
    OptInfo   opt;

    SetAdjustDefaults(&aP);
    AlignInfoWrap aInfo;
    // copy pano information into libpano data structures
    if (aInfo.setInfo(pano, imgs, vars, cps, optvec)) {
        aInfo.setGlobal();

        opt.numVars 		= aInfo.gl.numParam;
        opt.numData 		= aInfo.gl.numPts;
        opt.SetVarsToX		= SetLMParams;
        opt.SetXToVars		= SetAlignParams;
        opt.fcn			= aInfo.gl.fcn;
        *opt.message		= 0;

        DEBUG_DEBUG("starting optimizer");
        ::RunLMOptimizer( &opt );
        std::ostringstream oss;
        oss << "optimizing images";
        for (UIntVector::const_iterator it = imgs.begin(); it != imgs.end(); ++it) {
            if (it + 1 != imgs.end()) {
                oss << *it << ",";
            } else {
                oss << *it;
            }
        }
        oss << "\n" << opt.message;
        DEBUG_DEBUG("optimizer finished:" << opt.message);

        vars = aInfo.getVariables();
        cps = aInfo.getCtrlPoints();
    }
}
*/

void PTools::optimize(Panorama & pano,
                      const char * userScript)
{
    char * script = 0;

    if (userScript == 0) {
        ostringstream scriptbuf;
        UIntSet allImg;
        fill_set(allImg,0, unsigned(pano.getNrOfImages()-1));
        pano.printPanoramaScript(scriptbuf, pano.getOptimizeVector(), pano.getOptions(), allImg, true);
        script = strdup(scriptbuf.str().c_str());
    } else {
        script = const_cast<char *>(userScript);
    }

    OptInfo		opt;
	AlignInfo	ainf;

    if (ParseScript( script, &ainf ) == 0)
	{
		if( CheckParams( &ainf ) == 0 )
		{
			ainf.fcn	= fcnPano;
			
			SetGlobalPtr( &ainf ); 
			
			opt.numVars 		= ainf.numParam;
			opt.numData 		= ainf.numPts;
			opt.SetVarsToX		= SetLMParams;
			opt.SetXToVars		= SetAlignParams;
			opt.fcn			= ainf.fcn;
			*opt.message		= 0;

			RunLMOptimizer( &opt );
			ainf.data		= opt.message;
            // get results from align info.
#ifdef DEBUG_WRITE_OPTIM_OUTPUT
            fullPath path;
            StringtoFullPath(&path, DEBUG_WRITE_OPTIM_OUTPUT_FILE );

		    ainf.data		= opt.message;
            WriteResults( script, &path, &ainf, distSquared, 0);
#endif
            pano.updateVariables(GetAlignInfoVariables(ainf) );
            pano.updateCtrlPointErrors( GetAlignInfoCtrlPoints(ainf) );
		}
		DisposeAlignInfo( &ainf );
    }
    if (! userScript) {
        free(script);
    }
}

#if 0
void PTools::optimize(Panorama & pano,
                      utils::MultiProgressDisplay & progDisplay,
                      int maxIter)
{
//    VariableMapVector res;
    // setup data structures
    aPrefs    aP;
    OptInfo   opt;

    SetAdjustDefaults(&aP);
    AlignInfoWrap aInfo;
    // copy pano information int libpano data structures
    if (aInfo.setInfo(pano)) {
        aInfo.setGlobal();

        opt.numVars 		= aInfo.gl.numParam;
        opt.numData 		= aInfo.gl.numPts;
        opt.SetVarsToX		= SetLMParams;
        opt.SetXToVars		= SetAlignParams;
        opt.fcn			= aInfo.gl.fcn;
        *opt.message		= 0;

        DEBUG_DEBUG("starting optimizer");
        RunLMOptimizer( &opt );

#ifdef DEBUG
        fullPath path;
        StringtoFullPath(&path, "c:/debug_optimizer.txt");

		aInfo.gl.data		= opt.message;
        WriteResults( "debug_test", &path, &aInfo.gl, distSquared, 0);
#endif


        std::ostringstream oss;
        /*
        oss << "optimizing images";
        for (UIntVector::const_iterator it = imgs.begin(); it != imgs.end(); ++it) {
            if (it + 1 != imgs.end()) {
                oss << *it << ",";
            } else {
                oss << *it;
            }
        }
        oss << "\n" << opt.message;
        progDisplay.setMessage(oss.str());
        */
        DEBUG_DEBUG("optimizer finished:" << opt.message);

        pano.updateVariables(aInfo.getVariables());
        pano.updateCtrlPointErrors( aInfo.getCtrlPoints());

    }
}
#endif

/** autooptimise the panorama (does local optimisation first) */
void PTools::autoOptimise(PT::Panorama & pano)
{
// DGSW FIXME - Unreferenced
//	unsigned nImg = unsigned(pano.getNrOfImages());
    // build a graph over all overlapping images
    CPGraph graph;
    createCPGraph(pano,graph);


#if DEBUG
    {
        ofstream gfile("cp_graph.dot");
        // output doxygen graph
        boost::write_graphviz(gfile, graph);
    }
#endif

    std::set<std::string> optvars;
    optvars.insert("r");
    optvars.insert("p");
    optvars.insert("y");

    unsigned int startImg = pano.getOptions().optimizeReferenceImage;

    // start a breadth first traversal of the graph, and optimize
    // the links found (every vertex just once.)

    PTools::OptimiseVisitor optVisitor(pano, optvars);

    boost::queue<boost::graph_traits<CPGraph>::vertex_descriptor> qu;
    boost::breadth_first_search(graph, startImg,
                                color_map(get(vertex_color, graph)).
                                visitor(optVisitor));
/*
#ifdef DEBUG
    // print optimized script to cout
    DEBUG_DEBUG("after local optim:");
    VariableMapVector vars = optVisitor.getVariables();
    for (unsigned v=0; v < pano.getNrOfImages(); v++) {
        printVariableMap(std::cerr, vars[v]);
        std::cerr << std::endl;
    }
#endif

    // apply variables to input panorama
    pano.updateVariables(optVisitor.getVariables());

#ifdef DEBUG
    UIntSet allImg;
    fill_set(allImg,0, pano.getNrOfImages()-1);
    // print optimized script to cout
    DEBUG_DEBUG("after updateVariables():");
    pano.printPanoramaScript(std::cerr, pano.getOptimizeVector(), pano.getOptions(), allImg, false);
#endif
    */
}

OptimizeVector PTools::createOptVars(const Panorama & optPano, int mode)
{
    OptimizeVector optvars;
    for (unsigned i=0; i < optPano.getNrOfImages(); i++) {
        set<string> imgopt;
        if (i!=0) {
            if (mode & OPT_POS)
            // except for first image, optimize position
            imgopt.insert("r");
            imgopt.insert("p");
            imgopt.insert("y");
        }
        if (mode & OPT_HFOV) {
            imgopt.insert("v");
        }
        if (mode & OPT_B)
            imgopt.insert("b");
        if (mode & OPT_AC) {
            imgopt.insert("a");
            imgopt.insert("c");
        }
        if (mode & OPT_DE) {
            imgopt.insert("d");
            imgopt.insert("e");
        }
        if (mode & OPT_GT) {
            imgopt.insert("g");
            imgopt.insert("t");
        }
        optvars.push_back(imgopt);
    }

    return optvars;
}


/** use various heuristics to decide what to optimize.
 */
void PTools::smartOptimize(PT::Panorama & optPano)
{
    // remove vertical and horizontal control points
    CPVector cps = optPano.getCtrlPoints();
    CPVector newCP;
    for (CPVector::const_iterator it = cps.begin(); it != cps.end(); it++) {
        if (it->mode == ControlPoint::X_Y)
        {
            newCP.push_back(*it);
        }
    }
    optPano.setCtrlPoints(newCP);
    PTools::autoOptimise(optPano);

    // do global optimisation of position with all control points.
    optPano.setCtrlPoints(cps);
    OptimizeVector optvars = createOptVars(optPano, OPT_POS);
    optPano.setOptimizeVector(optvars);
    PTools::optimize(optPano);

    //---------------------------------------------------------------
    // Now with lens distortion

    int optmode = OPT_POS;
    double origHFOV = const_map_get(optPano.getImageVariables(0),"v").getValue();

    // determine which parameters to optimize
    double rmin, rmax, rmean, rvar, rq10, rq90;
    optPano.calcCtrlPntsRadiStats(rmin, rmax, rmean, rvar, rq10, rq90);

    DEBUG_DEBUG("Ctrl Point radi statistics: min:" << rmin << " max:" << rmax << " mean:" << rmean << " var:" << rvar << " q10:" << rq10 << " q90:" << rq90);

    if (origHFOV > 30) {
        // only optimize principal point if there are prespective effects
        optmode |= OPT_DE;
    }

    // heuristics for distortion and fov optimisation
    if ( (rq90 - rq10) >= 1) {
        // very well distributed control points 
        optmode |= OPT_AC | OPT_B | OPT_HFOV;
    } else if ( (rq90 - rq10) > 0.7) {
        optmode |= OPT_AC | OPT_B;
    } else {
        optmode |= OPT_B;
    }

    // check if this is a 360 deg pano.
    optPano.centerHorizontically();
    FDiff2D fov = optPano.calcFOV();

    if (fov.x >= 359) {
        // optimize HFOV for 360 deg panos
        optmode |= OPT_HFOV;
    }

    DEBUG_DEBUG("second optimization: " << optmode);

    // save old pano, might be needed if optimization went wrong
    PT::UIntSet allImgs;
    fill_set(allImgs, 0, optPano.getNrOfImages()-1);
    Panorama oldPano = optPano.getSubset(allImgs);
    optvars = createOptVars(optPano, optmode);
    optPano.setOptimizeVector(optvars);
    // global optimisation.
    PTools::optimize(optPano);

    // --------------------------------------------------------------
    // do some plausibility checks and reoptimize with less variables
    // if something smells fishy
    bool smallHFOV = false;
    bool highDist = false;
    bool highShift = false;
    const VariableMapVector & vars = optPano.getVariables();
    for (VariableMapVector::const_iterator it = vars.begin() ; it != vars.end(); it++)
    {
        if (const_map_get(*it,"v").getValue() < 1.0) smallHFOV = true;
        if (fabs(const_map_get(*it,"a").getValue()) > 0.8) highDist = true;
        if (fabs(const_map_get(*it,"b").getValue()) > 0.8) highDist = true;
        if (fabs(const_map_get(*it,"c").getValue()) > 0.8) highDist = true;
        if (fabs(const_map_get(*it,"d").getValue()) > 2000) highShift = true;
        if (fabs(const_map_get(*it,"e").getValue()) > 2000) highShift = true;
    }

    if (smallHFOV || highDist || highShift) {
        DEBUG_DEBUG("Optimization with strange result. status: HFOV: " << smallHFOV << " dist:" << highDist << " shift:" << highShift);
        // something seems to be wrong
        if (smallHFOV) {
            // do not optimize HFOV
            optmode &= ~OPT_HFOV;
        }
        if (highDist) {
            optmode &= ~OPT_AC;
        }
        if (highShift) {
            optmode &= ~OPT_DE;
        }

        // revert and redo optimisation
        optPano = oldPano;
        optvars = createOptVars(optPano, optmode);
        optPano.setOptimizeVector(optvars);
        DEBUG_DEBUG("recover optimisation: " << optmode);
        // global optimisation.
        PTools::optimize(optPano);
    }
}


#ifdef PT_CUSTOM_OPT
void PTools::stopOptimiser()
{
    optdata.terminate = true;
}
#endif


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

#include <fstream>

#include "PT/PTOptimise.h"
#include "PT/ImageGraph.h"

#include <boost/graph/graphviz.hpp>

using namespace std;
using namespace PT;
using namespace boost;

VariableMap PTools::optimisePair(Panorama & pano,
                                 const OptimizeVector & optvec,
                                 unsigned int firstImg,
                                 unsigned int secondImg)
{
    VariableMapVector res;
    // setup data structures
    aPrefs    aP;
    OptInfo   opt;

    SetAdjustDefaults(&aP);

    AlignInfoWrap aInfo;

    UIntVector imgs(2);
    imgs[0] = firstImg;
    imgs[1] = secondImg;

    // set the information
    if (aInfo.setInfo(pano,imgs, optvec)) {
        aInfo.setGlobal();

        opt.numVars 		= aInfo.gl.numParam;
        opt.numData 		= aInfo.gl.numPts;
        opt.SetVarsToX		= SetLMParams;
        opt.SetXToVars		= SetAlignParams;
        opt.fcn			= aInfo.gl.fcn;
        *opt.message		= 0;

        DEBUG_DEBUG("starting optimizer for images " << firstImg << " and " << secondImg);
        RunLMOptimizer( &opt );
        DEBUG_DEBUG("optimizer finished");

        res = aInfo.getVariables();
    }

    // return optimized variables of secondImg
    return res[1];
}

/** autooptimise the panorama (does local optimisation first) */
PT::VariableMapVector PTools::autoOptimise(PT::Panorama & pano)
{

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

    unsigned int startImg = pano.getOptions().optimizeReferenceImage;

    OptimizeVector optvec(2);
    optvec[1].insert("y");
    optvec[1].insert("p");
    optvec[1].insert("r");

    PTools::OptimiseVisitor optVisitor(pano, optvec);

    // start a breadth first traversal of the graph, and optimize
    // the links found (every vertex just once.)
    boost::breadth_first_search(graph, startImg, visitor(optVisitor));
    // iterate over all image combinations.
    unsigned nImg = pano.getNrOfImages();

    return optVisitor.getVars();
}

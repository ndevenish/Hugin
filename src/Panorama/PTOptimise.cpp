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

#include "PT/PTOptimise.h"

using namespace PT;

VariableMapVector PTools::optimisePair(Panorama & pano,
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

    UIntSet imgs;
    imgs.insert(firstImg);
    imgs.insert(secondImg);

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

    // get optimized variables
    return res;
}

#if 0
/** autooptimise the panorama (does local optimisation first) */
PT::VariableMapVector PT::autoOptimise(PT::Panorama & pano)
{
    
    // build a graph over all overlapping images
    CPGraph graph;
    createCPGraph(pano,graph);
    
    // iterate over all image combinations.
    unsigned nImg = pano.getNrOfImages();
}
#endif

// -*- c-basic-offset: 4 -*-
/** @file PTOptimise.h
 *
 *  functions to call the optimizer of panotools.
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

#ifndef _PTOPTIMISE_H
#define _PTOPTIMISE_H

#include "PT/Panorama.h"
#include "PT/PanoToolsInterface.h"

#include <boost/graph/breadth_first_search.hpp>

namespace PTools
{

/** optimize the images specified in this set.
 *
 *  control points between the images should exist.
 */
PT::VariableMap optimisePair(PT::Panorama & pano,
                             const PT::OptimizeVector & optvec,
                             unsigned int firstImg,
                             unsigned int secondImg);

/** a traverse functor to optimise the image links */
class OptimiseVisitor: public boost::default_bfs_visitor
{
public:
    OptimiseVisitor(PT::Panorama & pano, const PT::OptimizeVector & optvec)
        : m_pano(pano), m_opt(optvec), m_optVars(pano.getNrOfImages())
        { 
        };
    template < typename Edge, typename Graph >
    void tree_edge(Edge e, const Graph & g)
    {
        DEBUG_DEBUG("optimising image pair " << boost::source(e,g) << ", " << boost::target(e,g));
        m_optVars[target(e,g)] = optimisePair(m_pano, m_opt, source(e,g), target(e,g));
        // apply vars to panorama... (breaks undo!!!)
        m_pano.updateVariables(target(e,g), m_optVars[target(e,g)]);
    }
    
    PT::VariableMapVector getVars()
    { return m_optVars; }
    
private:
    PT::Panorama & m_pano;
    PT::OptimizeVector m_opt;
    PT::VariableMapVector m_optVars;
};
    

/** autooptimise the panorama (does local optimisation first) */
PT::VariableMapVector autoOptimise(PT::Panorama & pano);
    
    
} // namespace




#endif // _PTOPTIMISE_H

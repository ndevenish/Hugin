// -*- c-basic-offset: 4 -*-
/** @file PTOptimise.h
 *
 *  functions to call the optimizer of panotools.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PTOptimise.h 1951 2007-04-15 20:54:49Z dangelo $
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

#include <sstream>

#include "PT/Panorama.h"
#include "PT/PanoToolsInterface.h"
#include "PT/ImageGraph.h"

#include <boost/graph/breadth_first_search.hpp>

namespace PT
{
    
    /** autooptimise the panorama (does local optimisation first) */
    void autoOptimise(PT::Panorama & pano);
    
    
    /** use various heuristics to decide what to optimize.
    */
    void smartOptimize(PT::Panorama & pano);
    
    
    enum OptMode {OPT_POS=1, OPT_B=2, OPT_AC=4, OPT_DE=8, OPT_HFOV=16, OPT_GT=32, OPT_VIG=64, OPT_VIGCENTRE=128, OPT_EXP=256, OPT_WB=512, OPT_RESP=1024};
    // helper function for optvar creation
    PT::OptimizeVector createOptVars(const PT::Panorama & optPano, int mode, unsigned anchorImg=0);
    
        
    /** optimize the images specified in this set.
     *
     *  control points between the images should exist.
     */
    //PT::VariableMap optimisePair(PT::Panorama & pano,
    //                             const PT::OptimizeVector & optvec,
    //                             unsigned int firstImg,
    //                             unsigned int secondImg,
    //                             utils::MultiProgressDisplay & progDisplay);

        
        
    /** a traverse functor to optimise the image links */
    class OptimiseVisitor: public boost::default_bfs_visitor
    {
    public:
        OptimiseVisitor(PT::Panorama & pano, const std::set<std::string> & optvec)
            : m_opt(optvec), m_pano(pano)
        {
        }

        template < typename Vertex, typename Graph >
            void discover_vertex(Vertex v, const Graph & g);
        
        const PT::VariableMapVector & getVariables() const
        {
            return m_pano.getVariables();
        }
    /*
        const PT::CPVector & getCtrlPoints() const
        { 
            return m_cps;
        }
    */

    private:
        const std::set<std::string> & m_opt;
        PT::Panorama & m_pano;
        
    };


} // namespace




#endif // _PTOPTIMISE_H

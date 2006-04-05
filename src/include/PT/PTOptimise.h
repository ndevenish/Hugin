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

#include <sstream>

#include "PT/Panorama.h"
#include "PT/PanoToolsInterface.h"
#include "PT/ImageGraph.h"

#include <boost/graph/breadth_first_search.hpp>

namespace PTools
{


/** optimize the images \p imgs, for variables \p optvec, using \p vars
 *  as start. saves the control point distances in \p cps.
 *
 * \param panorama description
 * \param imgs vector with all image numbers that should be used.
 * \param optvect vector of vector of variable names
 * \param cps control points
 * \param progDisplay progress display
 *
 
 */
void optimize(PT::Panorama & pano,
              const char * script = 0);

/*
void optimize_PT(const PT::Panorama & pano,
                 const PT::UIntVector &imgs,
                 const PT::OptimizeVector & optvec,
                 PT::VariableMapVector & vars,
                 PT::CPVector & cps,
                 int maxIter=1000);
*/

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
        };

    template < typename Vertex, typename Graph >
    void discover_vertex(Vertex v, const Graph & g)
    {
        PT::UIntSet imgs;
        imgs.insert(v);
//        PT::VariableMapVector vars(1);
#ifdef DEBUG
        std::cerr << "before optim "<< v << " : ";
        printVariableMap(std::cerr, m_pano.getImageVariables(v));
        std::cerr << std::endl;
#endif

        // collect all optimized neighbours
        typename boost::graph_traits<PT::CPGraph>::adjacency_iterator ai;
        typename boost::graph_traits<PT::CPGraph>::adjacency_iterator ai_end;
        for (tie(ai, ai_end) = adjacent_vertices(v, g);
             ai != ai_end; ++ai)
        {
            if (*ai != v) {
                if ( (get(boost::vertex_color, g))[*ai] != boost::color_traits<boost::default_color_type>::white()) {
                    // image has been already optimized, use as anchor
                    imgs.insert(unsigned(*ai));
                    DEBUG_DEBUG("non white neighbour " << (*ai));
                } else {
                    DEBUG_DEBUG("white neighbour " << (*ai));
                }
            }
        }

        // get pano with neighbouring images.
        PT::Panorama localPano = m_pano.getSubset(imgs);

        // find number of current image in subset
        unsigned currImg = 0;
        unsigned cnt=0;
        for (PT::UIntSet::const_iterator it= imgs.begin(); it != imgs.end(); ++it) {
            if (v == *it) {
                currImg = cnt;
            }
            cnt++;
        }

        PT::OptimizeVector optvec(imgs.size());
        optvec[currImg] = m_opt;
        localPano.setOptimizeVector(optvec);

        if ( imgs.size() > 1) {
            DEBUG_DEBUG("optimising image " << v << ", with " << imgs.size() -1 << " already optimised neighbour imgs.");

            optimize(localPano);
            m_pano.updateVariables(v, localPano.getImageVariables(currImg));
#ifdef DEBUG
            std::cerr << "after optim " << v << " : ";
            printVariableMap(std::cerr, m_pano.getImageVariables(v));
            std::cerr << std::endl;
#endif
        }
    }

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


/** autooptimise the panorama (does local optimisation first) */
void autoOptimise(PT::Panorama & pano);


} // namespace




#endif // _PTOPTIMISE_H

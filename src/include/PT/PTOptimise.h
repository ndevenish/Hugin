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

// my optimize function, without pano tools gui hooks.
int fcnPano2(int m,int n, double * x, double * fvec, int * iflag);


/** optimize the images \p imgs, for variables \p optvec, using \p vars
 *  as start. saves the control point distances in \p cps.
 */
void PTools::optimize(const PT::Panorama & pano,
                      const PT::UIntVector &imgs,
                      const PT::OptimizeVector & optvec,
                      PT::VariableMapVector & vars,
                      PT::CPVector & cps,
                      utils::MultiProgressDisplay & progDisplay,
                      int maxIter=1000);

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
    OptimiseVisitor(const PT::Panorama & pano, const std::set<std::string> & optvec,
                    PT::VariableMapVector & vars, PT::CPVector & cps,
                    utils::MultiProgressDisplay & pdisp)
        : m_pano(pano), m_opt(optvec), m_optVars(vars),
          m_cps(cps), m_progDisp(pdisp)
        {
            m_optVars = pano.getVariables();
        };

    template < typename Vertex, typename Graph >
    void discover_vertex(Vertex v, const Graph & g)
    {
        PT::UIntVector imgs;
        imgs.push_back(v);
        PT::VariableMapVector vars(1);
        vars[0] = m_optVars[v];
#ifdef DEBUG
        std::cerr << "before optim "<< v << " : ";
        printVariableMap(std::cerr, vars[0]);
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
//            if (m_colors[*ai] != boost::color_traits<boost::default_color_type>::white()) {
                    // image has been already optimized, use as anchor
                    imgs.push_back(*ai);
                    vars.push_back(m_optVars[*ai]);
                    DEBUG_DEBUG("non white neighbour " << (*ai));
#ifdef DEBUG
                    std::cerr << "vars " << (*ai) << " : ";
                    printVariableMap(std::cerr, m_optVars[*ai]);
                    std::cerr << std::endl;
#endif
                } else {
                    DEBUG_DEBUG("white neighbour " << (*ai));
                }
            }
        }

        PT::OptimizeVector optvec(imgs.size());
        optvec[0] = m_opt;

        if ( imgs.size() > 1) {
            DEBUG_DEBUG("optimising image " << v << ", with " << imgs.size() -1 << " already optimised neighbour imgs.");
            std::ostringstream oss;
            oss << "optimizing image " << v;
            m_progDisp.pushTask(utils::ProgressTask(oss.str(), "",0));
            optimize(m_pano, imgs, optvec, vars, m_cps, m_progDisp, 1000);
            m_progDisp.popTask();
            m_optVars[v] = vars[0];
#ifdef DEBUG
            std::cerr << "after optim " << v << " : ";
            printVariableMap(std::cerr, m_optVars[v]);
            std::cerr << std::endl;
#endif
        } else {
            DEBUG_ERROR("image " << v << ": no optimized neighbour images");
        }
    }

    const PT::VariableMapVector & getVariables() const
    {
#ifdef DEBUG
        for ( PT::VariableMapVector::const_iterator it=m_optVars.begin();
              it != m_optVars.end(); ++it )
        {
            std::cerr << "optVars " << (it - m_optVars.begin()) << " ";
            printVariableMap(std::cerr, *it);
            std::cerr << std::endl;
        }
#endif
        return m_optVars;
    }

    const PT::CPVector & getCtrlPoints() const
    { return m_cps; }

private:
    const PT::Panorama & m_pano;
    const std::set<std::string> & m_opt;
    PT::VariableMapVector & m_optVars;
    PT::CPVector & m_cps;
    utils::MultiProgressDisplay & m_progDisp;
};


/** autooptimise the panorama (does local optimisation first) */
PT::VariableMapVector autoOptimise(const PT::Panorama & pano,
                                   const std::set<std::string> & optvars,
                                   PT::CPVector & cps,
                                   utils::MultiProgressDisplay & progDisp);

/** can be used to stop the optimiser */
void stopOptimiser();

// utility function, needed by PanoToolsInterface
double distSphere( int num );
double distSquared( int num );


} // namespace




#endif // _PTOPTIMISE_H

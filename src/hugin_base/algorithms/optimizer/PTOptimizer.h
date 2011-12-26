// -*- c-basic-offset: 4 -*-
/** @file 
 *
* !! from PTOptimise.h 1951
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

#ifndef _PTOPTIMIZER_H
#define _PTOPTIMIZER_H

#include <algorithms/PanoramaAlgorithm.h>

#include <hugin_shared.h>
#include <set>
#include <boost/graph/breadth_first_search.hpp>
#include <panodata/PanoramaData.h>

namespace HuginBase {
    
    
    ///
    class IMPEX PTOptimizer : public PanoramaAlgorithm
    {
    
        public:
            ///
            PTOptimizer(PanoramaData& panorama)
             : PanoramaAlgorithm(panorama)
            {};
        
            ///
            virtual ~PTOptimizer()
            {}
            
            
        public:
            ///
            virtual bool modifiesPanoramaData() const
                { return true; }
            
            /// calls PTools::optimize()
            virtual bool runAlgorithm();
    };
    
    /// Pairwise ransac optimisation 
    class IMPEX RANSACOptimizer : public PanoramaAlgorithm
    {
        public:
	    enum Mode {AUTO, HOMOGRAPHY, RPY, RPYV, RPYVB};

            ///
            RANSACOptimizer(PanoramaData& panorama, int i1, int i2, double maxError, Mode mode=RPY)
		: PanoramaAlgorithm(panorama), o_i1(i1), o_i2(i2),
	          o_maxError(maxError), o_mode(mode)
            {};
        
            ///
            virtual ~RANSACOptimizer()
            {}
            
            
        public:
            ///
            virtual bool modifiesPanoramaData() const
                { return true; }

	    static std::vector<int> findInliers(PanoramaData & pano, int i1, int i2, double maxError,
						Mode mode=RPY);
            
            /// calls PTools::optimize()
            virtual bool runAlgorithm();

        private:
	    int o_i1, o_i2;
	    double o_maxError;
	    std::vector<int> o_inliers;
	    Mode o_mode;
    };
    
    
    ///
    class IMPEX AutoOptimise : public PTOptimizer
    {
        
        public:
            ///
            AutoOptimise(PanoramaData& panorama, bool optRoll=true)
             : PTOptimizer(panorama)
            {};
        
            ///
            virtual ~AutoOptimise()
            {}
            
        
        public:
            ///
            static void autoOptimise(PanoramaData& pano, bool optRoll=true);
            
        protected:
            /// a traverse functor to optimise the image links
            class OptimiseVisitor: public boost::default_bfs_visitor
            {
            public:
                OptimiseVisitor(PanoramaData& pano, const std::set<std::string> & optvec)
                    : m_opt(optvec), m_pano(pano)
                {};
                
                ///
                template <typename Vertex, typename Graph>
                void discover_vertex(Vertex v, const Graph & g);
                
                ///
                VariableMapVector getVariables() const
                    { return m_pano.getVariables(); }
            
//                ///
//                const CPVector & getCtrlPoints() const
//                    { return m_cps; }
            
            private:
                const std::set<std::string> & m_opt;
                PanoramaData & m_pano;
            };
            
            
        public:
            ///
            virtual bool runAlgorithm()
            {
                autoOptimise(o_panorama);
                return true; // let's hope so.
            }

    };
    
    ///
    class IMPEX SmartOptimizerStub
    {
        public:
            ///
            enum OptMode {
                OPT_POS=1,
                OPT_B=2, 
                OPT_AC=4, 
                OPT_DE=8, 
                OPT_HFOV=16, 
                OPT_GT=32, 
                OPT_VIG=64, 
                OPT_VIGCENTRE=128, 
                OPT_EXP=256, 
                OPT_WB=512, 
                OPT_RESP=1024
            };
            
            /// helper function for optvar creation
            static OptimizeVector createOptVars(const PanoramaData& optPano, int mode, unsigned anchorImg=0);   
    };
    
    class IMPEX SmartOptimise : public PTOptimizer, protected SmartOptimizerStub
    {
        
        public:
            ///
            SmartOptimise(PanoramaData& panorama)
             : PTOptimizer(panorama)
            {};
        
            ///
            virtual ~SmartOptimise()
            {}
        
        public:
            ///
            static void smartOptimize(PanoramaData& pano);
        
            
        public:
            ///
            virtual bool runAlgorithm()
            {
                smartOptimize(o_panorama);
                return true; // let's hope so.
            }

    };
    
}//namesapce



//==============================================================================
// template implementation


#include <algorithms/optimizer/ImageGraph.h>
#include <panotools/PanoToolsOptimizerWrapper.h>

namespace HuginBase {

template <typename Vertex, typename Graph>
void AutoOptimise::OptimiseVisitor::discover_vertex(Vertex v, const Graph & g)
{
    UIntSet imgs;
    imgs.insert(v);
    //        VariableMapVector vars(1);
#ifdef DEBUG
    std::cerr << "before optim "<< v << " : ";
    printVariableMap(std::cerr, m_pano.getImageVariables(v));
    std::cerr << std::endl;
#endif
    
    // collect all optimized neighbours
    typename boost::graph_traits<CPGraph>::adjacency_iterator ai;
    typename boost::graph_traits<CPGraph>::adjacency_iterator ai_end;
    for (boost::tuples::tie(ai, ai_end) = adjacent_vertices(v, g);
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
    PanoramaData& localPano = *(m_pano.getNewSubset(imgs)); // don't forget to delete
    
    // find number of current image in subset
    unsigned currImg = 0;
    unsigned cnt=0;
    for (UIntSet::const_iterator it= imgs.begin(); it != imgs.end(); ++it) {
        if (v == *it) {
            currImg = cnt;
        }
        cnt++;
    }
    
    OptimizeVector optvec(imgs.size());
    optvec[currImg] = m_opt;
    localPano.setOptimizeVector(optvec);
    
    if ( imgs.size() > 1) {
        DEBUG_DEBUG("optimising image " << v << ", with " << imgs.size() -1 << " already optimised neighbour imgs.");
        
        PTools::optimize(localPano);
        m_pano.updateVariables(unsigned(v), localPano.getImageVariables(currImg));
#ifdef DEBUG
        std::cerr << "after optim " << v << " : ";
        printVariableMap(std::cerr, m_pano.getImageVariables(v));
        std::cerr << std::endl;
#endif
    }

    delete &localPano;
}

} //namespace
#endif //_h

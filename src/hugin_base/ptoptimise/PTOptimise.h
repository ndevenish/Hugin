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



        public:
            ///
            static void autoOptimise(PanoramaData& pano);
            
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
                const VariableMapVector & getVariables() const
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
            static void smartOptimise(PanoramaData& pano);
            
        protected:
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
            static PT::OptimizeVector createOptVars(const PanoramaData& optPano, int mode, unsigned anchorImg=0);
            
    }
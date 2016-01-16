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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PTOPTIMIZER_H
#define _PTOPTIMIZER_H

#include <algorithms/PanoramaAlgorithm.h>

#include <hugin_shared.h>
#include <set>
#include <panodata/PanoramaData.h>
#include <panotools/PanoToolsOptimizerWrapper.h>

namespace HuginBase {
    
    
    ///
    class IMPEX PTOptimizer : public PanoramaAlgorithm
    {
    
        public:
            ///
            explicit PTOptimizer(PanoramaData& panorama)
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
            explicit SmartOptimise(PanoramaData& panorama)
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
    
}//namespace

#endif //_h

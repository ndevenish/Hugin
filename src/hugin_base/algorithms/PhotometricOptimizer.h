// -*- c-basic-offset: 4 -*-
/** @file PhotometricOptimizer.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: PhotometricOptimizer.h 1931 2007-04-15 20:10:38Z dangelo $
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

#ifndef _PHOTOMETRIC_OPTIMIZER_H_
#define _PHOTOMETRIC_OPTIMIZER_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include <string>
#include <vector>
#include <set>

#include <common/math.h>
#include <PT/Panorama.h>

//#include <vigra_ext/VignettingCorrection.h>
#include <vigra_ext/utils.h>
#include <levmar/lm.h>


namespace PT
{
    
    
    class PhorometricOptimizer : TimeConsumingPanoramaAlgorithm
    {
        
        public:
            ///
            typedef std::vector<vigra_ext::PointPairRGB> PointPairs;
        
            ///
            PhorometricOptimizer(PanoramaData& panorama, ProgressDisplay* progressDisplay,
                                 const OptimizeVector& vars,
                                 const PointPairs& correspondences)
                : TimeConsumingPanoramaAlgorithm(panorama, progressDisplay)
            {};
        
            ///
            virtual ~PhorometricOptimizer();
    
            
        public:
            ///
            void optimizePhotometric(PanoramaData& pano, const OptimizeVector& vars,
                                     const PointPairs& correspondences,
                                     AppBase::ProgressReporter& progress,
                                     double& error);
        
        protected:
            ///
            struct VarMapping
            {
                std::string type;
                std::set<unsigned> imgs;
            };

            ///
            struct OptimData
            {
                const PanoramaData& m_pano;
                std::vector<SrcPanoImage> m_imgs;
                std::vector<VarMapping> m_vars;
                std::vector<vigra_ext::PointPairRGB> m_data;
                double huberSigma;
                bool symmetricError;

                int m_maxIter;
                AppBase::ProgressReporter& m_progress;

                OptimData(const PanoramaData& pano, const OptimizeVector& optvars,
                          const std::vector<vigra_ext::PointPairRGB>& data,
                          double mEstimatorSigma, bool symmetric,
                          int maxIter, AppBase::ProgressReporter& progress);

                // copy optimisation variables into x
                void ToX(double * x);

                // copy new values from x to into this->m_imgs
                void FromX(double * x);
            };

            ///
            void photometricError(double* p, double* x, int m, int n, void* data);


        public:
            ///
            virtual bool modifiesPanoramaData()
                { return true; }
            
            ///
            virtual bool runAlgorithm()
            {
                AppBase::ProgressReport* progRep = 
                    AppBase::ProgressReportAdaptor::newProgressReporter(getProgressDisplay(), 1.0); // is this correct? how much progress requierd?
                
                optimizePhotometric(o_panorama, 
                                    o_vars, o_correspondences,
                                    *progRep, o_resultError);
                
                delete progRep;
                
                // optimizePhotometric does not tell us if it's cancelled
                if(hasProgressDisplay())
                {
                    if(getProgressDisplay()->wasCancelled())
                        cancelAlgorithm();
                }
                
                return wasCancelled(); // let's hope so.
            }
    
            
        public:
            double getResultError() const
            {
                // [TODO] if(!hasRunSuccessfully()) DEBUG;
                return o_resultError;
            }
                
            
        protected:
            const OptimizeVector& o_vars;
            const PointPairs& o_correspondences;
            double o_resultError;
    }




    class SmartPhotometrictOptimizer : PhorometricOptimizer
    {
        public:
            /// local optimize definition.
            enum PhotometricOptimizeMode {
                OPT_PHOTOMETRIC_LDR=0, 
                OPT_PHOTOMETRIC_LDR_WB, 
                OPT_PHOTOMETRIC_HDR, 
                OPT_PHOTOMETRIC_HDR_WB
            };
        
            ///
            SmartPhotometrictOptimizer(PanoramaData& panorama, ProgressDisplay* progressDisplay,
                                       const OptimizeVector& vars,
                                       const PointPairs& correspondences)
                : PhorometricOptimizer(panorama, progressDisplay, )
                
            {};
            
            ///
            virtual ~PhorometricOptimizer();
        
            
        public:
            /** use various heuristics to decide what to optimize.
             */
            void smartOptimizePhotometric(Panorama & pano, PhotometricOptimizeMode mode,
                                          const std::vector<vigra_ext::PointPairRGB> & correspondences,
                                          utils::ProgressReporter & progress,
                                          double & error);
            
            ///
            virtual bool runAlgorithm()
            {
                AppBase::ProgressReport* progRep;
                
                if(hasProgressDisplay()) {
                    progRep = new AppBase::ProgressReportAdaptor(*getProgressDisplay(), 1.0); // is this correct? how much progress requierd?
                } else {
                    progRep = new AppBase::DummyProgressReport();
                }
                
                smartOptimizePhotometric(o_panorama,
                                         o_optMode,
                                         o_vars, o_correspondences,
                                         *progRep, o_resultError);
                
                delete progRep;
                
                // smartOptimizePhotometric does not tell us if it's cancelled
                if(hasProgressDisplay())
                {
                    if(getProgressDisplay()->wasCancelled())
                        cancelAlgorithm();
                }
                
                return !wasCancelled(); // let's hope so.
            }

            
        protected:
            PhotometricOptimizeMode o_optMode;
    };

    
    
} // namespace


#endif

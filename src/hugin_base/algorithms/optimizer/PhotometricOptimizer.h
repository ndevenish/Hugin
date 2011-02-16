// -*- c-basic-offset: 4 -*-
/** @file hugin_base/algorithms/optimizer/PhotometricOptimizer.h
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

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>
#include <algorithms/optimizer/PTOptimizer.h>

#include <vector>
#include <panodata/PanoramaData.h>
#include <appbase/ProgressReporterOld.h>
#include <vigra_ext/VignettingCorrection.h>

namespace HuginBase
{
    
    class IMPEX PhotometricOptimizer : public TimeConsumingPanoramaAlgorithm
    {
        
        public:
            ///
            typedef std::vector<vigra_ext::PointPairRGB> PointPairs;
        
            ///
            PhotometricOptimizer(PanoramaData& panorama, AppBase::ProgressDisplay* progressDisplay,
                                 const OptimizeVector& vars,
                                 const PointPairs& correspondences)
                : TimeConsumingPanoramaAlgorithm(panorama, progressDisplay),
                  o_vars(vars), o_correspondences(correspondences), o_resultError(0.0)
            {};
        
            ///
            virtual ~PhotometricOptimizer() {};
    
            
        public:
            ///
            static void optimizePhotometric(PanoramaData& pano, const OptimizeVector& vars,
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


                ///
                OptimData(const PanoramaData& pano, const OptimizeVector& optvars,
                          const std::vector<vigra_ext::PointPairRGB>& data,
                          double mEstimatorSigma, bool symmetric,
                          int maxIter, AppBase::ProgressReporter& progress);

                /// copy optimisation variables into x
                void ToX(double * x);

                /// copy new values from x to into this->m_imgs
                void FromX(double * x);
                
            };
            
            static int photometricVis(double *p, double *x, int m, int n, int iter, double sqerror, void * data);

            ///
            static void photometricError(double* p, double* x, int m, int n, void* data);


        public:
            ///
            virtual bool modifiesPanoramaData() const
                { return true; }
            
            ///
            virtual bool runAlgorithm();
    
            
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
    };




    class IMPEX SmartPhotometricOptimizer : public PhotometricOptimizer, protected SmartOptimizerStub
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
            SmartPhotometricOptimizer(PanoramaData& panorama, AppBase::ProgressDisplay* progressDisplay,
                                       const OptimizeVector& vars,
                                       const PointPairs& correspondences,
                                       PhotometricOptimizeMode optMode)
                : PhotometricOptimizer(panorama, progressDisplay, vars, correspondences), o_optMode(optMode)
            {};
            
            ///
            virtual ~SmartPhotometricOptimizer() {};
        
            
        public:
            /** use various heuristics to decide what to optimize.
             */
            static void smartOptimizePhotometric(PanoramaData & pano, PhotometricOptimizeMode mode,
                                                 const std::vector<vigra_ext::PointPairRGB> & correspondences,
                                                 AppBase::ProgressReporter & progress,
                                                 double & error);
            
            ///
            virtual bool runAlgorithm();

            
        protected:
            PhotometricOptimizeMode o_optMode;
    };

    
    
} // namespace


#endif

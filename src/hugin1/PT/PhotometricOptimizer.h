// -*- c-basic-offset: 4 -*-
/** @file hugin1/PT/PhotometricOptimizer.h
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

#ifndef _Hgn1_PHOTOMETRIC_OPTIMIZER_H_
#define _Hgn1_PHOTOMETRIC_OPTIMIZER_H_

#include <algorithms/optimizer/PhotometricOptimizer.h>

#include "PT/Panorama.h"
#include "PT/PanoramaMemento.h"

namespace PT
{


    static void optimizePhotometric(Panorama & pano, const OptimizeVector & vars,
                             const std::vector<vigra_ext::PointPairRGB> & correspondences,
                             AppBase::ProgressReporter & progress,
                             double & error)
    {
        HuginBase::PhotometricOptimizer::optimizePhotometric(pano, vars, correspondences, progress, error);
    }


    typedef HuginBase::SmartPhotometricOptimizer::PhotometricOptimizeMode PhotometricOptimizeMode;
    static PhotometricOptimizeMode OPT_PHOTOMETRIC_LDR    = HuginBase::SmartPhotometricOptimizer::OPT_PHOTOMETRIC_LDR;
    static PhotometricOptimizeMode OPT_PHOTOMETRIC_LDR_WB = HuginBase::SmartPhotometricOptimizer::OPT_PHOTOMETRIC_LDR_WB;
    static PhotometricOptimizeMode OPT_PHOTOMETRIC_HDR    = HuginBase::SmartPhotometricOptimizer::OPT_PHOTOMETRIC_HDR;
    static PhotometricOptimizeMode OPT_PHOTOMETRIC_HDR_WB = HuginBase::SmartPhotometricOptimizer::OPT_PHOTOMETRIC_HDR_WB;


    static void smartOptimizePhotometric(Panorama & pano, PhotometricOptimizeMode mode,
                                  const std::vector<vigra_ext::PointPairRGB> & correspondences,
                                  AppBase::ProgressReporter & progress,
                                  double & error)
    {
        HuginBase::SmartPhotometricOptimizer::smartOptimizePhotometric(pano, mode, correspondences, progress, error);
    }

}


#endif

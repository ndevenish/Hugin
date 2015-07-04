// -*- c-basic-offset: 4 -*-
/** @file CalculateMeanExposure.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 * !! from Panorama.h 1947 
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

#ifndef _BASICALGORITHMS_CALCULATEMEANEXPOSURE_H
#define _BASICALGORITHMS_CALCULATEMEANEXPOSURE_H

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>



namespace HuginBase {


class IMPEX CalculateMeanExposure : public PanoramaAlgorithm
{

    public:
        ///
        explicit CalculateMeanExposure(PanoramaData& panorama)
            : PanoramaAlgorithm(panorama), o_resultExposure(0.0)
        {};
        
        ///
        virtual ~CalculateMeanExposure() {};
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
            
        ///
        virtual bool runAlgorithm()
        {
            o_resultExposure = calcMeanExposure(o_panorama);
            return true; // let's hope so.
        }
          
        
    public:
        
        ///
        static double calcMeanExposure(const PanoramaData& pano);
        
        ///
        virtual double getResultExposure() const
        { 
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultExposure;
        }
    
        
    protected:
        double o_resultExposure;
};


}
#endif // _H

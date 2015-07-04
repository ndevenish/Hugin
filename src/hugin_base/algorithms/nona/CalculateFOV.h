// -*- c-basic-offset: 4 -*-
/** @file CalculateFOV.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: Panorama.h 1947 2007-04-15 20:46:00Z dangelo $
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

#ifndef _BASICALGORITHMS_CALCULATECFOV_H
#define _BASICALGORITHMS_CALCULATECFOV_H

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>

#include <panodata/PanoramaData.h>


namespace HuginBase {


class IMPEX CalculateFOV : public PanoramaAlgorithm
{

    public:
        ///
        explicit CalculateFOV(PanoramaData& panorama)
         : PanoramaAlgorithm(panorama)
        {};
        
        ///
        virtual ~CalculateFOV() {};
        
        
    public:
        ///
        bool modifiesPanoramaData() const
            { return false; }
            
        ///
        bool runAlgorithm()
        {
            o_resultFOV = calcFOV(o_panorama);
            return true; // let's hope so.
        }
          
        
    public:
        
        ///
        static hugin_utils::FDiff2D calcFOV(const PanoramaData& panorama);
            
        ///
        double getResultHorizontalFOV()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultFOV.x;
        }
            
        ///
        double getResultVerticalFOV()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultFOV.y;
        }
        
        ///
        hugin_utils::FDiff2D getResultFOV()
        { 
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultFOV;
        }
    
        
    protected:
        hugin_utils::FDiff2D o_resultFOV;
};


}
#endif // _H

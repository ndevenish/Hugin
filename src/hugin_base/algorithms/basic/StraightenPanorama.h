// -*- c-basic-offset: 4 -*-
/** @file PanoramaDataLegacySupport.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _BASICALGORITHMS_STRAIGHTENPANORAMA_H
#define _BASICALGORITHMS_STRAIGHTENPANORAMA_H

#include <algorithms/basic/RotatePanorama.h>


namespace HuginBase {


class StraightenPanorama : public RotatePanorama
{

    public:
        ///
        StraightenPanorama(PanoramaData& panorama)
         : RotatePanorama(panorama, Matrix3())
        {};
        
        ///
        virtual ~StraightenPanorama() {};
          
        
    public:
        ///
        static Matrix3 calcStraighteningRotation(const PanoramaData& panorama);       
        
    public:
        ///
        virtual bool runAlgorithm()
        {
            o_transformMat = calcStraighteningRotation(o_panorama);
            RotatePanorama::runAlgorithm();
            
            return true; // let's hope so.
        }
};


}

#endif //_H        
// -*- c-basic-offset: 4 -*-
/** @file RotatePanorama.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _BASICALGORITHMS_ROTATEPANORAMA_H
#define _BASICALGORITHMS_ROTATEPANORAMA_H

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>

#include <panodata/PanoramaData.h>


namespace HuginBase {
    
    
class IMPEX RotatePanorama : public PanoramaAlgorithm
{

    public:
        ///
        RotatePanorama(PanoramaData& panorama, const Matrix3& transformMat)
         : PanoramaAlgorithm(panorama), o_transformMat(transformMat)
        {};
        
        ///
        RotatePanorama(PanoramaData& panorama, double yaw, double pitch, double roll);
        
        ///
        virtual ~RotatePanorama() {};
          
        
    public:
        ///
        static void rotatePano(PanoramaData& panorama, const Matrix3& transformMat);
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return true; }
            
        ///
        virtual bool runAlgorithm()
        {
            rotatePano(o_panorama, o_transformMat);
            return true; // let's hope so.
        }

        
    protected:
        Matrix3 o_transformMat;
};


} // namespace
#endif

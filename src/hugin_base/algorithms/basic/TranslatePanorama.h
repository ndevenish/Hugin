// -*- c-basic-offset: 4 -*-
/** @file TranslatePanorama.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>  Yuval Levy <http://www.photopla.net/>
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

#ifndef _BASICALGORITHMS_TRANSLATEPANORAMA_H
#define _BASICALGORITHMS_TRANSLATEPANORAMA_H

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>
#include <panodata/PanoramaData.h>


namespace HuginBase {
    
    
class IMPEX TranslatePanorama : public PanoramaAlgorithm
{

    public:
        ///
        TranslatePanorama(PanoramaData& panorama, const double x, const double y, const double z)
         : PanoramaAlgorithm(panorama), o_x(x), o_y(y), o_z(z)
        {};
        
        ///
        TranslatePanorama(PanoramaData& panorama);
        
        ///
        virtual ~TranslatePanorama() {};
          
        
    public:
        ///
        static void translatePano(PanoramaData& panorama, const double x, const double y, const double z);
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return true; }
            
        ///
        virtual bool runAlgorithm()
        {
            translatePano(o_panorama, o_x, o_y, o_z);
            return true; // let's hope so.
        }

        
    protected:
        double o_x;
        double o_y;
        double o_z;
};


} // namespace
#endif

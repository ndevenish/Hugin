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


namespace HuginBase {


class FitPanorama : PanoramaAlgorithm
{

    public:
        ///
        FitPanorama(PanoramaData& panorama)
         : PanoramaAlgorithm(panorama)
        {};
        
        ///
        virtual ~FitPanorama();
        
        
    public:
        ///
        bool modifiesPanoramaData()
            { return false; }
            
        ///
        bool runAlgorithm()
        {
            fitPano(o_panorama, o_resultHFOV, o_resultHeight);
            return true; // let's hope so.
        }
          
        
    public:
        
        ///
        static void fitPano(const PanoramaData& panorama, double& HFOV, double& height)
            
        ///
        double getResultHorizontalFOV()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultHFOV;
        }
            
        ///
        double getResultVerticalFOV()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultHeight;
        }
        
        ///
        FDiff2D getResultFOV()
        { 
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return FDiff2D(getResultHorizontalFOV(), getResultVerticalFOV());
        }
    
        
    protected:
        double o_resultHFOV;
        double o_resultHeight;
};


}
        
        
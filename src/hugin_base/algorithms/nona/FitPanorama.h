// -*- c-basic-offset: 4 -*-
/** @file FitPanorama.h
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

#ifndef _FITPANORAMA_H
#define _FITPANORAMA_H

#include <hugin_shared.h>
#include <algorithms/PanoramaAlgorithm.h>



namespace HuginBase {


class IMPEX CalculateFitPanorama : public PanoramaAlgorithm
{

    public:
        ///
        explicit CalculateFitPanorama(PanoramaData& panorama)
            : PanoramaAlgorithm(panorama), o_resultHFOV(0), o_resultHeight(0)
        {};
        
        ///
        virtual ~CalculateFitPanorama() {};
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
            
        ///
        virtual bool runAlgorithm()
        {
            fitPano(o_panorama, o_resultHFOV, o_resultHeight);
            return true; // let's hope so.
        }
          
        
    public:
        ///
        static void fitPano(PanoramaData& panorama, double& HFOV, double& height);
            
        ///
        virtual double getResultHorizontalFOV()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultHFOV;
        }
            
        ///
        virtual double getResultHeight()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultHeight;
        }
        
    protected:
        double o_resultHFOV;
        double o_resultHeight;
};



///
class IMPEX FitPanorama : public CalculateFitPanorama
{

    public:
        ///
        explicit FitPanorama(PanoramaData& panorama)
         : CalculateFitPanorama(panorama)
        {};
        
        ///
        virtual ~FitPanorama() {};
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return true; }
            
        ///
        virtual bool runAlgorithm();
};


} // namespace
#endif // _H

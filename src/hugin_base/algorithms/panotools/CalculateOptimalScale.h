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

#ifndef _BASICALGORITHMS_CALCULATEMEANEXPOSURE_H
#define _BASICALGORITHMS_CALCULATEMEANEXPOSURE_H

#include <algorithm/CalculateOptimalScale.h>


namespace HuginBase {


class CalculateOptimalScale : PanoramaAlgorithm
{

    public:
        ///
        CalculateOptimalScale(PanoramaData& panorama)
         : PanoramaAlgorithm(panorama)
        {};
        
        ///
        virtual ~FitPanorama();
        
        
    public:
        ///
        virtual bool modifiesPanoramaData()
            { return false; }
            
        ///
        virtual bool runAlgorithm()
        {
            o_optimalScale =  calcOptimalScale(o_panorama);
            return true; // let's hope so.
        }
        
    public:
        ///
        static double calcOptimalScale(PanoramaData& panorama);
        
        /** function to calculate the scaling factor so that the distances
         * in the input image and panorama image are similar at the panorama center
         */
        static double calcOptimalPanoScale(const SrcPanoImage & src,
                                           const PanoramaOptions & dest);
        
        ///
        virtual double getResultOptimalScale()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_optimalScale;
        }
        
        ///
        virtual unsigned getResultOptimalWidth()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return roundi(getResultOptimalScale() * o_panorama.getOptions().getWidth());
        }
        
    protected:
        double o_optimalScale;
        
};

}
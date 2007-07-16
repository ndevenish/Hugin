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

#ifndef _BASICALGORITHMS_CALCULATECPSTSTISTICS_H
#define _BASICALGORITHMS_CALCULATECPSTSTISTICS_H

#include <algorithm/PanoramaAlgorithm.h>



namespace HuginBase {


/// just some common implementation; probably not so useful
class CalculateCPStatistics : public PanoramaAlgorithm
{

    protected:
        ///
        CalculateCPStatistics(PanoramaData& panorama, const int& imgNr=-1)
         : PanoramaAlgorithm(panorama), o_imageNmber(imgNr)
        {};
        
    public:
        ///
        virtual ~CalculateCPStatistics() {};
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
            
        ///
        virtual bool runAlgorithm() =0;
          
        
    public:
        ///
        virtual double getResultMin()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultMin;
        }
        
        ///
        virtual double getResultMax()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultMax;
        }
        
        ///
        virtual double getResultMean()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultMean;
        }
    
        ///
        virtual double getResultVariance()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultVar;
        }
        
    protected:
        int o_imageNmber;
        double o_resultMin, o_resultMax, o_resultMean, o_resultVar;
};
    
    
    
    
class CalculateCPStatisticsError : public CalculateCPStatistics
{

    public:
        ///
        CalculateCPStatisticsError(PanoramaData& panorama, const int& imgNr=-1)
         : CalculateCPStatistics(panorama, imgNr)
        {};
        
        ///
        virtual ~CalculateCPStatisticsError() {};
          
        
    public:
        ///
        static void calcCtrlPntsErrorStats(const PanoramaData& pano,
                                           double & min, double & max, double & mean,
                                           double & var,
                                           const int& imgNr=-1);
        
        
    public:
        ///
        virtual bool runAlgorithm()
        {
            calcCtrlPntsErrorStats(o_panorama, 
                                   o_resultMin, o_resultMax, o_resultMean,
                                   o_resultVar,
                                   o_imageNmber);
            return true; // let's hope so.
        }
        
};


class CalculateCPStatisticsRadial : public CalculateCPStatistics
{
    
    public:
        ///
        CalculateCPStatisticsRadial(PanoramaData& panorama, const int& imgNr=-1)
         : CalculateCPStatistics(panorama, imgNr)
        {};
        
        ///
        virtual ~CalculateCPStatisticsRadial() {};
        
        
    public:
        ///
        static void calcCtrlPntsRadiStats(const PanoramaData& pano,
                                          double & min, double & max, double & mean, double & var,
                                          double & q10, double & q90, 
                                          const int& imgNr=-1);
        
        ///
        virtual double getResultPercentile10()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultQ10;
        }
        
        ///
        virtual double getResultPercentile90()
        {
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_resultQ90;
        }
        

        // virtual double getResultPercentile(const int& percent) ?
        
        
    public:
        ///
        virtual bool runAlgorithm()
        {
                calcCtrlPntsRadiStats(o_panorama, 
                                      o_resultMin, o_resultMax, o_resultMean, o_resultVar,
                                      o_resultQ10, o_resultQ90,
                                      o_imageNmber);
                return true; // let's hope so.
        }
        
        
    protected:
        double o_resultQ10, o_resultQ90;
        
};

}
        
#endif //_H
// -*- c-basic-offset: 4 -*-
/** @file hugin1/PT/Panorama.h
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

#ifndef _Hgn1_PANORAMA_H
#define _Hgn1_PANORAMA_H

#include <panodata/Panorama.h>
#include <panodata/PTScriptParsing.h>
#include <algorithms/nona/CalculateFOV.h>
#include <algorithms/nona/FitPanorama.h>
#include <algorithms/basic/CalculateOptimalScale.h>
#include <algorithms/basic/CalculateCPStatistics.h>
#include <algorithms/nona/CenterHorizontally.h>
#include <algorithms/basic/RotatePanorama.h>
#include <algorithms/basic/TranslatePanorama.h>
#include <algorithms/basic/StraightenPanorama.h>
#include <algorithms/basic/CalculateMeanExposure.h>
#include <algorithms/basic/CalculateOptimalROI.h>
#include <algorithms/basic/LayerStacks.h>

#include <typeinfo>
#include "PT/PanoImage.h"
#include "PT/PanoramaMemento.h"


namespace PT {

using HuginBase::UIntSet;
using HuginBase::UIntVector;


class Panorama : public HuginBase::Panorama
{
public:
    Panorama()
    : HuginBase::Panorama()
    {};
    
    Panorama(const HuginBase::Panorama& pano)
      : HuginBase::Panorama(pano)
    {};
    
    virtual ~Panorama() {};
    
    
public:
    /** calculates the horizontal and vertial FOV of the complete panorama
        *
        *  @return HFOV,VFOV
        */
    hugin_utils::FDiff2D calcFOV() const
    {
        Panorama pano(*this);
		return HuginBase::CalculateFOV::calcFOV(pano);
	}
    
    /** calculate the HFOV and height so that the whole input
        *  fits in into the output panorama */
    void fitPano(double & HFOV, double & height)
    {
        HuginBase::CalculateFitPanorama fitPano(*this);
        fitPano.run();
        HFOV = fitPano.getResultHorizontalFOV();
        height = fitPano.getResultHeight();
    }
    
    /** calculate the optimal width for this panorama 
        *
        *  Optimal means that the pixel density at the panorama and
        *  image center of the image with the highest resolution
        *  are the same.
        */
    unsigned calcOptimalWidth() const
    {
        Panorama pano(*this);
		return hugin_utils::roundi(HuginBase::CalculateOptimalScale::calcOptimalScale(pano) * pano.getOptions().getWidth());
    }
    
    /** Calculates the ROI to make the best ROI without excess for crop
         */
    void calcOptimalROI(vigra::Rect2D & roi,vigra::Size2D & size)
    {
        printf("calcOptimalROI Called\n");
        HuginBase::CalculateOptimalROI cropPano(*this);
        
        cropPano.run();
        
        roi=cropPano.getResultOptimalROI();
        size=cropPano.getResultOptimalSize();
    }

    /** Calculates the Stack-based ROI to make the best ROI without excess for crop
         */
    void calcOptimalStackROI(vigra::Rect2D & roi,vigra::Size2D & size)
    {
        printf("calcOptimalROI Called\n");
        UIntSet activeImages=getActiveImages();
        std::vector<UIntSet> stackImgs=getHDRStacks(*this,activeImages, getOptions());
        HuginBase::CalculateOptimalROI cropPano(*this);
        //only use hdr autocrop for projects with stacks
        //otherwise fall back to "normal" autocrop
        if(stackImgs.size()<activeImages.size())
        {
            cropPano.setStacks(stackImgs);
        }
        cropPano.run();
        roi=cropPano.getResultOptimalROI();
        size=cropPano.getResultOptimalSize();
    }


    /** calculate control point error distance statistics */
    void calcCtrlPntsErrorStats(double & min, double & max, double & mean, double & var, int imgNr=-1) const
    {
        Panorama pano(*this);
        HuginBase::CalculateCPStatisticsError calcStat(pano, imgNr);
        calcStat.run();
        min = calcStat.getResultMin();
        max = calcStat.getResultMax();
        mean = calcStat.getResultMean();
        var = calcStat.getResultVariance();
    }
    
    /** calculate control point radial distance statistics. q10 and q90 are the 10% and 90% quantile */
    void calcCtrlPntsRadiStats(double & min, double & max, double & mean, double & var,
                               double & q10, double & q90, int imgNr=-1) const
    {
        Panorama pano(*this);
        HuginBase::CalculateCPStatisticsRadial calcStat(pano, imgNr);
        calcStat.run();
        min = calcStat.getResultMin();
        max = calcStat.getResultMax();
        mean = calcStat.getResultMean();
        var = calcStat.getResultVariance();
        q10 = calcStat.getResultPercentile10();
        q90 = calcStat.getResultPercentile90();
    }
    
    /** center panorama horizontically */
    void centerHorizontically()
    {
        HuginBase::CenterHorizontally(*this).run();
    }
    
    /** rotate the complete panorama
        *
        *  Will modify the position of all images.
        */
    void rotate(double yaw, double pitch, double roll)
    {
        HuginBase::RotatePanorama(*this, yaw,pitch,roll).run();
    }
    
    /** rotate the complete panorama.
        *
        *  Will modify the position of all images.
        */
    void rotate(const Matrix3 & rot)
    {
        HuginBase::RotatePanorama(*this, rot).run();
    }

    /** translate the complete panorama
        *
        *  Will modify the position of all images.
        */
    void translate(double x, double y, double z)
    {
        HuginBase::TranslatePanorama(*this, x, y, z).run();
    }

    /** translate the complete panorama.
        *
        *  Will modify the position of all images.
        */
//    void translate(const Matrix3 & rot)
//    {
//        HuginBase::TranslatePanorama(*this, rot).run();
//    }

    
    /** try to automatically straighten the panorama */
    void straighten()
    {
        HuginBase::StraightenPanorama(*this).run();
    }
    
};


class PanoramaObserver : public HuginBase::PanoramaObserver
{
    
public:
    virtual ~PanoramaObserver() {};
    
    
public:
    virtual void panoramaChanged(Panorama &pano)
    { DEBUG_DEBUG("Default panoramaChanged called"); };
    
    
    virtual void panoramaImagesChanged(Panorama &pano, const UIntSet & changed)
    { DEBUG_DEBUG("DEFAULT handler method"); };
    
    
public:
    virtual void panoramaChanged(HuginBase::PanoramaData& pano)
    {
            DEBUG_INFO("New interface is called.")
            
            try {
                panoramaChanged(dynamic_cast<Panorama&>(pano));
            } catch(std::bad_cast e) {
                DEBUG_WARN("Can't handle Non- PT::Panorama instance.")
            }
    };
    
    virtual void panoramaImagesChanged(HuginBase::PanoramaData& pano,
                                       const UIntSet& changed)
    {
        DEBUG_INFO("New interface is called.")
        
        try {
            panoramaImagesChanged(dynamic_cast<Panorama&>(pano), changed);
        } catch(std::bad_cast e) {
            DEBUG_WARN("Can't handle Non-PT::Panorama instance.")
        }
    };
};


inline double calcOptimalPanoScale(const SrcPanoImage& src,
                                   const PanoramaOptions& dest)
{
    return HuginBase::CalculateOptimalScale::calcOptimalPanoScale(src, dest);
}

inline double calcMeanExposure(Panorama& pano)
{
	return HuginBase::CalculateMeanExposure::calcMeanExposure(pano);
}


using HuginBase::PTScriptParsing::getPTParam;
//using HuginBase::PTScriptParsing::getParam;
using HuginBase::PTScriptParsing::getIntParam;
using HuginBase::PTScriptParsing::readVar;
using HuginBase::PTScriptParsing::getDoubleParam;
using HuginBase::PTScriptParsing::getPTDoubleParam;



} // namespace
#endif // _PANORAMA_H

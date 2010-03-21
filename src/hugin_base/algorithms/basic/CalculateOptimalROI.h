// -*- c-basic-offset: 4 -*-
/** @file PanoramaDataLegacySupport.h
 *
 *  @author <cnidarian>
 *
 *  $Id: CalculateOptimalROI.h 2510 2009-9-9 cnidarian $
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


#ifndef _BASICALGORITHMS_CALCULATEOPTIMALROI_H
#define _BASICALGORITHMS_CALCULATEOPTIMALROI_H


#include <hugin_shared.h>
#include <panotools/PanoToolsInterface.h>

#include <algorithm/PanoramaAlgorithm.h>

#include <panodata/PanoramaData.h>


namespace HuginBase {


class IMPEX CalculateOptimalROI : public PanoramaAlgorithm
{

    public:
        ///
        CalculateOptimalROI(PanoramaData& panorama)
         : PanoramaAlgorithm(panorama)
        {
            //set to zero for error condition
            o_optimalROI = vigra::Rect2D(0,0,0,0);
            o_optimalSize = vigra::Size2D(0,0);
        }
        
        ///
        virtual ~CalculateOptimalROI()
        {}
        
        
    public:
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
            
        ///
        virtual bool runAlgorithm()
        {
            printf("Run called\n");
            calcOptimalROI(o_panorama);
            return true; // let's hope so.
        }
        
    public:
        ///
        bool calcOptimalROI(PanoramaData& panorama);
        
        /** Find the largest area of ROI such that there is no excess
         */
        vigra::Rect2D calcOutsideBox(int imgnum, const SrcPanoImage & src,
                                           const PanoramaOptions & dest);
        void drawOutputRegion(int imgnum, unsigned char *tmp, const SrcPanoImage & src,
                                           const PanoramaOptions & dest);
        
        /// return the ROI structure?, for now area
        virtual vigra::Rect2D getResultOptimalROI()
        {
            //printf("Get Result ROI\n");
            //printf("Get Result ROI\n");
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_optimalROI;
        }

        /// return the ROI structure?, for now area
        virtual vigra::Size2D getResultOptimalSize()
        {
            //printf("Get Result Size\n");
            // [TODO] if(!hasRunSuccessfully()) DEBUG;
            return o_optimalSize;
        }


    protected:
        vigra::Rect2D o_optimalROI;
        vigra::Size2D o_optimalSize;
        
        int totImages;
        PTools::Transform *transfList;
        //in case input images are different sizes
        vigra::Size2D *imgSizeList;
        
        int imgPixel(unsigned char *img,int i, int j);
        
        //local stuff, convert over later
        struct nonrec
        {
            int left,right,top,bottom;
            struct nonrec *next;
        };

        void makecheck(int left,int top,int right,int bottom);
        int autocrop(unsigned char *img);
        void nonreccheck(unsigned char *img,int left,int top,int right,int bottom,int acc,int dodouble);
        

        int count;
        int total;
        struct nonrec *begin;
        struct nonrec *head;
        struct nonrec *tail;
        struct nonrec best;
        struct nonrec min;
        struct nonrec max;

        int maxvalue;
        
};


} //namespace
#endif

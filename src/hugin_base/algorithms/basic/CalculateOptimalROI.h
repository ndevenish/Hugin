// -*- c-basic-offset: 4 -*-
/** @file CalculateOptimalROI.h
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
#include <algorithms/PanoramaAlgorithm.h>
#include <panodata/PanoramaData.h>

#include <boost/dynamic_bitset.hpp>

namespace HuginBase {

class IMPEX CalculateOptimalROI : public PanoramaAlgorithm
{
    public:
        ///
        CalculateOptimalROI(PanoramaData& panorama, bool intersect = false)
         : PanoramaAlgorithm(panorama), intersection(intersect)
        {
            //set to zero for error condition
            o_optimalROI = vigra::Rect2D(0,0,0,0);
            o_optimalSize = vigra::Size2D(0,0);
        }

        CalculateOptimalROI(PanoramaData& panorama, std::vector<UIntSet> hdr_stacks)
         : PanoramaAlgorithm(panorama), intersection(true), stacks(hdr_stacks)
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

        /** sets the stack vector */
        void setStacks(std::vector<UIntSet> hdr_stacks);

    private:
        vigra::Rect2D o_optimalROI;
        vigra::Size2D o_optimalSize;
        
        bool intersection;
        
        std::vector<UIntSet> stacks;
        
        UIntSet activeImages;
        std::map<unsigned int,PTools::Transform*> transfMap;
        //map for storing already tested pixels
        boost::dynamic_bitset<> testedPixels;
        boost::dynamic_bitset<> pixels;
        
        bool imgPixel(int i, int j);
        bool stackPixel(int i, int j, UIntSet &stack);
        
        //local stuff, convert over later
        struct nonrec
        {
            int left,right,top,bottom;
            struct nonrec *next;
        };

        void makecheck(int left,int top,int right,int bottom);
        int autocrop();
        void nonreccheck(int left,int top,int right,int bottom,int acc,int searchStrategy);
        
        int count;
        struct nonrec *head;
        struct nonrec *tail;
        struct nonrec best;

        long maxvalue;
};

} //namespace
#endif

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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _BASICALGORITHMS_CALCULATEOPTIMALROI_H
#define _BASICALGORITHMS_CALCULATEOPTIMALROI_H

#include <hugin_shared.h>
#include <panotools/PanoToolsInterface.h>
#include <algorithms/PanoramaAlgorithm.h>
#include <panodata/PanoramaData.h>

#include <vector>
#include <list>

namespace HuginBase {

class IMPEX CalculateOptimalROI : public TimeConsumingPanoramaAlgorithm
{
    public:
        /** constructor */
        CalculateOptimalROI(PanoramaData& panorama, AppBase::ProgressDisplay* progress, bool intersect = false)
            : TimeConsumingPanoramaAlgorithm(panorama, progress), intersection(intersect)
        {
            //set to zero for error condition
            m_bestRect = vigra::Rect2D(0,0,0,0);
            o_optimalSize = vigra::Size2D(0,0);
        }
        CalculateOptimalROI(PanoramaData& panorama, AppBase::ProgressDisplay* progress, std::vector<UIntSet> hdr_stacks)
            : TimeConsumingPanoramaAlgorithm(panorama, progress), intersection(true), stacks(hdr_stacks)
        {
            //set to zero for error condition
            m_bestRect = vigra::Rect2D(0, 0, 0, 0);
            o_optimalSize = vigra::Size2D(0,0);
        }
        
        /** destructor */
        virtual ~CalculateOptimalROI() {};
        
        ///
        virtual bool modifiesPanoramaData() const
            { return false; }
            
        ///
        virtual bool runAlgorithm()
        {
            printf("Run called\n");
            return calcOptimalROI(o_panorama);
        }
        
        /// return the ROI structure?, for now area
        virtual vigra::Rect2D getResultOptimalROI()
        {
            if (hasRunSuccessfully())
            {
                return m_bestRect;
            }
            else
            {
                return vigra::Rect2D();
            }
        }

        /** sets the stack vector */
        void setStacks(std::vector<UIntSet> hdr_stacks);

    private:
        ///
        bool calcOptimalROI(PanoramaData& panorama);

        vigra::Size2D o_optimalSize;
        bool intersection;
        std::vector<UIntSet> stacks;
        UIntSet activeImages;
        std::map<unsigned int,PTools::Transform*> transfMap;
        //map for storing already tested pixels
        std::vector<bool> testedPixels;
        std::vector<bool> pixels;
        vigra::Rect2D m_bestRect;

        bool imgPixel(int i, int j);
        bool stackPixel(int i, int j, UIntSet &stack);
        
        //local stuff, convert over later
        bool autocrop();
        void nonreccheck(const vigra::Rect2D& rect, int acc, int searchStrategy, long& maxvalue);
        bool CheckRectCoversPano(const vigra::Rect2D& rect);
        void AddCheckingRects(std::list<vigra::Rect2D>& testingRects, const vigra::Rect2D& rect, const long maxvalue);

        void CleanUp();
};

} //namespace
#endif

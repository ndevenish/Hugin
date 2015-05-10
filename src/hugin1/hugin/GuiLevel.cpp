// -*- c-basic-offset: 4 -*-
/**  @file GuiLevel.cpp
 *
 *  @brief definition of helper for GuiLevel
 *
 *  @author T. Modes
 *
 */
 
 /*  This is free software; you can redistribute it and/or
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

#include "GuiLevel.h"
#include "panodata/StandardImageVariableGroups.h"

GuiLevel GetMinimumGuiLevel(HuginBase::PanoramaData& pano)
{
    if(pano.getNrOfImages()>0)
    {
        for(size_t i=0;i<pano.getNrOfImages();i++)
        {
            const HuginBase::SrcPanoImage& img=pano.getImage(i);
            if(img.getX()!=0 || img.getY()!=0 || img.getZ()!=0 || img.getShear().squareLength()>0)
            {
                return GUI_EXPERT;
            };
        }
        HuginBase::StandardImageVariableGroups variable_group(pano);
        if(variable_group.getStacks().getNumberOfParts()<pano.getNrOfImages())
        {
            return GUI_ADVANCED;
        };
        for(size_t i=0;i<pano.getNrOfImages();i++)
        {
            const HuginBase::SrcPanoImage& img=pano.getImage(i);
            if(img.getRadialVigCorrCenterShift().squareLength()>0)
            {
                return GUI_ADVANCED;
            };
            HuginBase::MaskPolygonVector masks=img.getMasks();
            for(size_t j=0; j<masks.size(); j++)
            {
                if(masks[j].getMaskType()==HuginBase::MaskPolygon::Mask_Stack_negative ||
                    masks[j].getMaskType()==HuginBase::MaskPolygon::Mask_Stack_positive)
                {
                    return GUI_ADVANCED;
                };
            };
        };
    };
    return GUI_SIMPLE;
};

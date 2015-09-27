// -*- c-basic-offset: 4 -*-

/** @file PreviewColorPickerTool.h
 *
 *  @author Thomas Modes
 *
 */
 
/*  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PREVIEWCOLORPICKERTOOL_H
#define _PREVIEWCOLORPICKERTOOL_H

#include "Tool.h"
#include "base_wx/wxutils.h"

/** The PreviewColorPickerTool allows to select a region in the panorama which should be grey.
 *  In the selected region it calculates the new red multiplier and blue multiplier to get
 *  a grey region.
 *
 */
class PreviewColorPickerTool : public Tool
{
public:
    /** constructor */
    explicit PreviewColorPickerTool(ToolHelper *helper):Tool(helper), m_red(0.0), m_blue(0.0), m_count(0) {};
    void Activate();
    /** process mouse button events */
    void MouseButtonEvent(wxMouseEvent &e);
private:
    double m_red;
    double m_blue;
    unsigned int m_count;
    void CalcCorrection(hugin_utils::FDiff2D pos);
    void CalcCorrectionForImage(unsigned int i,vigra::Point2D pos);
};

#endif


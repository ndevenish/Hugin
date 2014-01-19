// -*- c-basic-offset: 4 -*-

/** @file PreviewDeleteCPTool.h
 *
 *  @author T. Modes
 *
 *  @brief interface of ToolHelper for deleting control points in the pano space
 *
 */
/*
 *  This program is free software; you can redistribute it and/or
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _PREVIEWDELETECPTOOL_H
#define _PREVIEWDELETECPTOOL_H

#include "Tool.h"
#include "hugin_utils/utils.h"

#include <panotools/PanoToolsInterface.h>

/** Tool to delete all cp in a selected rectangle
 */
class PreviewDeleteCPTool : public Tool
{
public:
    PreviewDeleteCPTool(ToolHelper *helper) : Tool(helper), m_mouse_down(false) {};
    /** activate the tool */
    void Activate();
    /** draw selection rectangle */
    void ReallyAfterDrawImagesEvent();
    /** mouse move handling */
    void MouseMoveEvent(double x, double y, wxMouseEvent & e);
    /** mouse button handling */
    void MouseButtonEvent(wxMouseEvent &e);
private:
    void DeleteCP(const hugin_utils::FDiff2D& pos1, const hugin_utils::FDiff2D& pos2);
    bool m_mouse_down;
    hugin_utils::FDiff2D m_start_pos;
    hugin_utils::FDiff2D m_current_pos;
};

#endif


// -*- c-basic-offset: 4 -*-

/** @file PreviewEditCPTool.h
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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _PREVIEWEDITCPTOOL_H
#define _PREVIEWEDITCPTOOL_H

#include "Tool.h"
#include "base_wx/wxutils.h"

#include <panotools/PanoToolsInterface.h>

/** id for context menu */
enum{
    ID_CREATE_CP=wxID_HIGHEST + 301,
    ID_REMOVE_CP
};

/** Tool to delete all cp in a selected rectangle
 */
class PreviewEditCPTool : public Tool
{
public:
    explicit PreviewEditCPTool(ToolHelper *helper) : Tool(helper), m_mouseDown(false), m_menuPopup(false) {};
    /** activate the tool */
    void Activate();
    /** draw selection rectangle */
    void ReallyAfterDrawImagesEvent();
    /** mouse move handling */
    void MouseMoveEvent(double x, double y, wxMouseEvent & e);
    /** mouse button handling */
    void MouseButtonEvent(wxMouseEvent &e);
    /** return set of found control points */
    HuginBase::UIntSet GetFoundCPs() { return m_CPinROI; };
    /** returns selected ROI */
    vigra::Rect2D GetSelectedROI();
    /** reset popup menu status */
    void SetMenuProcessed();
private:
    /** search for control points in selected rectangle */
    void FindCPInRect(const hugin_utils::FDiff2D& pos1, const hugin_utils::FDiff2D& pos2);
    /** mouse down status */
    bool m_mouseDown;
    /** true, when popup menu is shown, this is to ignore a mouse event when the popup menu is closed */
    bool m_menuPopup;
    /** position where the marking starts */
    hugin_utils::FDiff2D m_startPos;
    /** position where the marking starts in screen coordinates */
    hugin_utils::FDiff2D m_startPosScreen;
    /** current position of selection */
    hugin_utils::FDiff2D m_currentPos;
    /** current position of selection in screen coordinates*/
    hugin_utils::FDiff2D m_currentPosScreen;
    /** contains the found cp */
    HuginBase::UIntSet m_CPinROI;
};

#endif


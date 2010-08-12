// -*- c-basic-offset: 4 -*-

/** @file PreviewControlPointTool.h
 *
 *  @author James Legg
 *
 *  Copyright 2009 James Legg.
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

#ifndef _PREVIEWCONTROLPOINTTOOL_H
#define _PREVIEWCONTROLPOINTTOOL_H

#include "Tool.h"
#include "GreatCircles.h"
#include "hugin_utils/utils.h"

#include <panotools/PanoToolsInterface.h>

/** The PreviewCropTool shows lines between the ends of control points in the
 * fast preview.
 * Regular control points are drawn in orange, blue control points are drawn in
 * sky blue.
 *
 * @todo:
 *  Delete control points when clicked and show the control point window on right
 *  click / middle click / modifier-key click?
 *  Perhaps not - not responding to user input allows us to
 *  show the control points while using the interactive tools.
 */
class PreviewControlPointTool : public Tool
{
public:
    PreviewControlPointTool(ToolHelper *helper);
    void Activate();
    void AfterDrawImagesEvent();
private:
    HuginBase::PTools::Transform *transforms;
    void MakeTransforms();
    GreatCircles m_greatCircles;
};

#endif


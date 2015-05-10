// -*- c-basic-offset: 4 -*-
/** @file PanosphereSphereTool.h
 *
 *  @author Darko Makreshanski
 *
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

#ifndef _PANOSPHERE_SPHERE_TOOL_H
#define _PANOSPHERE_SPHERE_TOOL_H

#include "Tool.h"

/**
 * tool to draw a whiteish transparent sphere for the panosphere
 */
class PanosphereSphereTool : public PanosphereOverviewTool
{
public:
    void Activate();
    PanosphereSphereTool(PanosphereOverviewToolHelper *helper, const wxColour backgroundColour);
    
    void BeforeDrawImagesBackEvent();
    void BeforeDrawImagesFrontEvent();
    /** sets the sphere background color */
    void SetPreviewBackgroundColor (wxColour c);

protected:
    wxColour m_background_color;
};

#endif


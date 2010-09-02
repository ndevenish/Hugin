// -*- c-basic-offset: 4 -*-

/** @file Tool.cpp
 *
 *  @author James Legg
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "Tool.h"
#include "ToolHelper.h"

Tool::Tool(ToolHelper * helper_in)
{
    helper = helper_in;
}

Tool::~Tool() {}

PreviewTool::PreviewTool(PreviewToolHelper *helper_in) : Tool(helper_in) {}

PreviewTool::~PreviewTool() {}

OverviewTool::OverviewTool(OverviewToolHelper *helper_in) : Tool(helper_in) {}

OverviewTool::~OverviewTool() {}

PanosphereOverviewTool::PanosphereOverviewTool(PanosphereOverviewToolHelper *helper_in) : OverviewTool(helper_in) {}

PanosphereOverviewTool::~PanosphereOverviewTool() {}

PlaneOverviewTool::PlaneOverviewTool(PlaneOverviewToolHelper *helper_in) : OverviewTool(helper_in) {}

PlaneOverviewTool::~PlaneOverviewTool() {}


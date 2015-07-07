// -*- c-basic-offset: 4 -*-
/** @file ProjectionGridTool.h
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

#ifndef _PROJECTION_GRID_TOOL_H
#define _PROJECTION_GRID_TOOL_H

#include "Tool.h"

/**
 * tool to draw a grid to create correspondence between the overview and the preview
 */
class ProjectionGridTool
{
public:
    explicit ProjectionGridTool(ToolHelper* helper);
    virtual ~ProjectionGridTool();
    
protected:

    bool createTexture();

    bool texture_created;
    unsigned int texture_num;

    MeshManager::MeshInfo * mesh_info;

    ToolHelper* helper_g;

};

class PreviewProjectionGridTool : public PreviewTool, public ProjectionGridTool
{
public:
    void Activate();
    explicit PreviewProjectionGridTool(PreviewToolHelper *helper) : PreviewTool(helper), ProjectionGridTool(helper) {}

    void createMesh();

    void AfterDrawImagesEvent();
    void BeforeDrawImagesEvent();
protected:

};

class PanosphereOverviewProjectionGridTool : public PanosphereOverviewTool, public ProjectionGridTool
{
public:
    void Activate();
    explicit PanosphereOverviewProjectionGridTool(PanosphereOverviewToolHelper *helper) : PanosphereOverviewTool(helper), ProjectionGridTool(helper) {}

    void createMesh();

    void AfterDrawImagesBackEvent();
    void BeforeDrawImagesBackEvent();

    void AfterDrawImagesFrontEvent();
    void BeforeDrawImagesFrontEvent();
protected:

};

#endif


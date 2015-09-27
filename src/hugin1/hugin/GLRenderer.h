// -*- c-basic-offset: 4 -*-
/** @file GLRenderer.h
 *
 *  @author James Legg
 *  @author Darko Makreshanski
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

#ifndef _GLRENDERER_H
#define _GLRENDERER_H


// TODO needed?
//#include <vector>
/* something messed up... temporary fix :-( */
#include "base_wx/wxutils.h"
#include <base_wx/wxImageCache.h>
#include <vigra_ext/ROIImage.h>
#include <vigra/diff2d.hxx>
#include <utility>

class ToolHelper;
class OverviewToolHelper;
class PreviewToolHelper;
class PanosphereOverviewToolHelper;
class PlaneOverviewToolHelper;

/** The renderer handles drawing the opengl scene. It is used by a GLViewer, which is
 * a wxWidget. The work of generating textures to represent the image is done by
 * a TextureManager, and the remappings are made in display lists by a
 * MeshManager. The GLViewer gives us instances of those objects to use.
 */
class GLRenderer
{
public:
    /** ctor.
     */
    explicit GLRenderer(const wxColour backgroundColour);

    /** dtor.
     */
    virtual ~GLRenderer();
    /** Resize the viewport because the window's dimensions have changed.
     * @return the number of screen pixels from the corner of the widget to the
     * start of the panorama,  both horizontally and vertically.
     * @param width the width of the widget in screen pixels.
     * @param height the height of the widget in screen pixels.
     */
    virtual vigra::Diff2D Resize(int width, int height) = 0;
    virtual void Redraw() = 0;

    void SetBackground(unsigned char red, unsigned char green, unsigned char blue);
    float width_o, height_o;
    /** sets the preview background color */
    void SetPreviewBackgroundColor(const wxColour c);

protected:
    HuginBase::Panorama  * m_pano;
    TextureManager * m_tex_man;
    MeshManager * m_mesh_man;
    ToolHelper *m_tool_helper;
    int width, height;
    wxColour m_background_color;

};

/**
 * subclass for the preview canvas
 */
class GLPreviewRenderer : public GLRenderer
{
public:
    GLPreviewRenderer(HuginBase::Panorama * pano, TextureManager *tex_man,
               MeshManager *mesh_man, VisualizationState *visualization_state,
               PreviewToolHelper *tool_helper, const wxColour backgroundColour);

    vigra::Diff2D Resize(int width, int height);
    void Redraw();

protected:
    VisualizationState * m_visualization_state;
};

/**
 * subclass for the panosphere overview mode
 */
class GLPanosphereOverviewRenderer : public GLRenderer
{
public:
    GLPanosphereOverviewRenderer(HuginBase::Panorama * pano, TextureManager *tex_man,
               MeshManager *mesh_man, PanosphereOverviewVisualizationState *visualization_state,
               PanosphereOverviewToolHelper *tool_helper, const wxColour backgroundColour);

    vigra::Diff2D Resize(int width, int height);
    void Redraw();
protected:
    PanosphereOverviewVisualizationState * m_visualization_state;

};

/**
 * subclass for the plane overview mode
 */
class GLPlaneOverviewRenderer : public GLRenderer
{
public:
    GLPlaneOverviewRenderer(HuginBase::Panorama * pano, TextureManager *tex_man,
               MeshManager *mesh_man, PlaneOverviewVisualizationState *visualization_state,
               PlaneOverviewToolHelper *tool_helper, const wxColour backgroundColour);

    vigra::Diff2D Resize(int width, int height);
    void Redraw();
protected:
    PlaneOverviewVisualizationState * m_visualization_state;

};

#endif

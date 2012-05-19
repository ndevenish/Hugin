// -*- c-basic-offset: 4 -*-
/** @file GLViewer.h
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef _GL_VIEWER_H
#define _GL_VIEWER_H

#include "ViewState.h"
#include "base_wx/platform.h"
#include <wx/glcanvas.h>
#include <utility>
#include <vigra/diff2d.hxx>

class GLRenderer;
class GLPanosphereOverviewRenderer;
class GLPlaneOverviewRenderer;
class TextureManager;
class MeshManager;
class ToolHelper;
class PreviewToolHelper;
class OverviewToolHelper;
class PanosphereOverviewToolHelper;
class PlaneOverviewToolHelper;
class GLPreviewFrame;




/** A wxWidget to display the fast preview.
 * It is the OpenGL equivalent of PreviewPanel.
 * The actual work in rendering the preview is done by a GLRenderer.
 */
class GLViewer: public wxGLCanvas
{
public:
    GLViewer(
            wxWindow* parent, 
            PT::Panorama &pano, 
            int args[], 
            GLPreviewFrame *frame,
            wxGLContext * shared_context = NULL
            );
    virtual ~GLViewer();
    void RedrawE(wxPaintEvent& e);
    void Resized(wxSizeEvent& e);
    void Redraw();
    static void RefreshWrapper(void *obj);
    void SetUpContext();
    void SetPhotometricCorrect(bool state);
    virtual void SetLayoutMode(bool state);
    virtual void SetLayoutScale(double scale);
    
    VisualizationState * m_visualization_state;
    static ViewState * m_view_state;

    void SetActive(bool active) {this->active = active;}
    bool IsActive() {return active;}

#if defined __WXGTK__ || wxCHECK_VERSION(2,9,0)
    wxGLContext * GetContext() {return m_glContext;}
#endif

    void SetViewerBackground(wxColour col);

    /** sets the text which should overlay the preview */
    void SetOverlayText(const wxString text);
    /** switches the overlay on/off */
    void SetOverlayVisibility(const bool isVisible);

protected:
    void OnEraseBackground(wxEraseEvent& e);
    void MouseMotion(wxMouseEvent& e);
    void MouseLeave(wxMouseEvent & e);
    void MouseButtons(wxMouseEvent& e);
    void MouseWheel(wxMouseEvent& e);
    void KeyDown(wxKeyEvent & e);
    void KeyUp(wxKeyEvent & e);

    DECLARE_EVENT_TABLE()

    ToolHelper *m_tool_helper;
    GLRenderer *m_renderer;
#if defined __WXGTK__ || wxCHECK_VERSION(2,9,0)
    wxGLContext *m_glContext;
#endif
    PT::Panorama  * m_pano;

    wxString m_overlayText;
    bool m_overlay;
    virtual void setUp() = 0;

    bool started_creation, redrawing;
    static bool initialised_glew;
    vigra::Diff2D offset;
    GLPreviewFrame *frame;

    bool active;

    wxColour m_background_color;
};

class GLPreview : public GLViewer
{
public:
    GLPreview(
            wxWindow* parent, 
            PT::Panorama &pano, 
            int args[], 
            GLPreviewFrame *frame,
            wxGLContext * shared_context = NULL
            ) : GLViewer(parent, pano, args, frame, shared_context) {}
    void setUp();

};

class GLOverview : public GLViewer
{
public:
    GLOverview(
            wxWindow* parent, 
            PT::Panorama &pano, 
            int args[], 
            GLPreviewFrame *frame,
            wxGLContext * shared_context = NULL
            ) : GLViewer(parent, pano, args, frame, shared_context) {
        panosphere_m_renderer = 0;
        plane_m_renderer = 0;
    }

    void SetPanosphereMode();
    void SetPlaneMode();
    
    void setUp();

    virtual void SetLayoutMode(bool state);
    virtual void SetLayoutScale(double scale);

    enum OverviewMode {
        PANOSPHERE,
        PLANE
    };

    void SetMode(OverviewMode mode);
    OverviewMode GetMode() {return mode;}

protected:

    OverviewMode mode;

    PanosphereOverviewVisualizationState * panosphere_m_visualization_state;
    PanosphereOverviewToolHelper * panosphere_m_tool_helper;
    GLPanosphereOverviewRenderer * panosphere_m_renderer;
    
    PlaneOverviewVisualizationState * plane_m_visualization_state;
    PlaneOverviewToolHelper * plane_m_tool_helper;
    GLPlaneOverviewRenderer * plane_m_renderer;

};

#endif
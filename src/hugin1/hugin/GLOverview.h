// -*- c-basic-offset: 4 -*-
/** @file GLOverview.h
 *
 *  @author James Legg
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

#ifndef _GL_OVERVIEW_H
#define _GL_OVERVIEW_H

#include "ViewState.h"
#include "base_wx/platform.h"
#include <wx/glcanvas.h>
#include <utility>
#include <vigra/diff2d.hxx>

class GLRenderer;
class TextureManager;
class MeshManager;
class PreviewToolHelper;
class GLPreviewFrame;

/** A wxWidget to display the fast preview.
 * It is the OpenGL equivalent of PreviewPanel.
 * The actual work in rendering the preview is done by a GLRenderer.
 */
class GLOverview: public wxPanel
{
public:
    GLOverview(wxWindow* parent, PT::Panorama &pano, int args[], GLPreviewFrame *frame);
    virtual ~GLOverview();
    void RedrawE(wxPaintEvent& e);
    void Resized(wxSizeEvent& e);
    void Redraw();
    static void RefreshWrapper(void *obj);
    void SetUpContext();
    void SetPhotometricCorrect(bool state);
    void SetLayoutMode(bool state);
    void SetLayoutScale(double scale);
    
    ViewState * m_view_state;
protected:
    void OnEraseBackground(wxEraseEvent& e);
    void MouseMotion(wxMouseEvent& e);
    void MouseLeave(wxMouseEvent & e);
    void LeftDown(wxMouseEvent& e);
    void LeftUp(wxMouseEvent& e);
    void RightDown(wxMouseEvent& e);
    void RightUp(wxMouseEvent& e);
    void KeyDown(wxKeyEvent & e);
    void KeyUp(wxKeyEvent & e);
    
    DECLARE_EVENT_TABLE()
    
    PreviewToolHelper *m_tool_helper;
    GLRenderer *m_renderer;
    wxGLContext *m_glContext;
    PT::Panorama  * m_pano;
    
    bool started_creation, initialised_glew, redrawing;
    vigra::Diff2D offset;
    GLPreviewFrame *frame;
    
};

#endif



// -*- c-basic-offset: 4 -*-
/** @file GLViewer.cpp
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

#ifdef __APPLE__
#include "panoinc_WX.h"
#endif

#include "hugin_utils/utils.h"

#include "panoinc.h"
#include <config.h>
#if !defined Hugin_shared || !defined _WINDOWS
#define GLEW_STATIC
#endif
#include <GL/glew.h>
#include <base_wx/platform.h>
#include <wx/settings.h>
#include <wx/dcclient.h>
#include <wx/event.h>

#include "GLViewer.h"
#include "GLRenderer.h"
#include "TextureManager.h"
#include "MeshManager.h"
#include "ToolHelper.h"
#include "GLPreviewFrame.h"
#include "hugin/huginApp.h"


ViewState * GLViewer::m_view_state = NULL;

BEGIN_EVENT_TABLE(GLViewer, wxGLCanvas)
    EVT_PAINT (GLViewer::RedrawE)
    EVT_SIZE  (GLViewer::Resized)
    EVT_ERASE_BACKGROUND(GLViewer::OnEraseBackground)
    // mouse motion
    EVT_MOTION (GLViewer::MouseMotion)
    // mouse entered or left the preview
    EVT_LEAVE_WINDOW(GLViewer::MouseLeave)
    // mouse buttons
    EVT_LEFT_DOWN (GLViewer::LeftDown)
    EVT_LEFT_UP (GLViewer::LeftUp)
    EVT_RIGHT_DOWN (GLViewer::RightDown)
    EVT_RIGHT_UP (GLViewer::RightUp)
    // keyboard events
    EVT_KEY_DOWN(GLViewer::KeyDown)
    EVT_KEY_UP(GLViewer::KeyUp)
END_EVENT_TABLE()


GLViewer::GLViewer(
            wxWindow* parent, 
            PT::Panorama &pano, 
            int args[], 
            GLPreviewFrame *frame_in
            ) :
          wxGLCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                     0, wxT("GLPreviewCanvas"), args, wxNullPalette)
{
    /* The openGL display context doesn't seem to be created automatically on
     * wxGTK, and on wxMac the constructor doesn't fit the documentation. I
     * create a new context on anything but wxMac and hope it works... */
    #ifdef __WXMAC__
      m_glContext = GetContext();
    #else
      m_glContext = new wxGLContext(this, 0);
	#endif
    
    m_renderer = 0;
    
    m_pano = &pano;

    frame = frame_in;
    
    started_creation = false;
    initialised_glew = false;
    redrawing = false;
}

GLViewer::~GLViewer()
{
#if !defined __WXMAC__
    delete m_glContext;
#endif
    if (m_renderer)
    {
      delete m_tool_helper;
      delete m_renderer;
      delete m_view_state;
    }
}

void GLViewer::SetUpContext()
{
    // set the context
    DEBUG_INFO("Setting rendering context...");
    Show();
    #ifdef __WXMAC__
    m_glContext->SetCurrent();
    #else
    m_glContext->SetCurrent(*this);
    #endif
    DEBUG_INFO("...got a rendering context.");
    if (!started_creation)
    {
        // It appears we are setting up for the first time.
        started_creation = true;
        
        if (!initialised_glew)
        {
            // initialise the glew library, if not done it before.
            GLenum error_state = glewInit();
            initialised_glew = true;
            if (error_state != GLEW_OK)
            {
                // glewInit failed
                started_creation=false;
                DEBUG_ERROR("Error initialising GLEW: "
                        << glewGetErrorString(error_state) << ".");
                frame->Close();
                wxMessageBox(_("Error initialising GLEW\nFast preview window can not be opened."),_("Error"), wxOK | wxICON_ERROR,MainFrame::Get());
                return;
            }
        }
        // check the openGL capabilities.
        if (!(GLEW_VERSION_1_1 && GLEW_ARB_multitexture))
        {
            started_creation=false;
            DEBUG_ERROR("Sorry, OpenGL 1.1 + GL_ARB_multitexture extension required.");
            frame->Close();
            wxMessageBox(_("Sorry, the fast preview window requires a system which supports OpenGL version 1.1 with the GL_ARB_multitexture extension.\nThe fast preview cannot be opened."),_("Error"), wxOK | wxICON_ERROR,MainFrame::Get());
            return;
        }


        setUp();
        // check, if gpu supports multitextures
        // fill blend mode choice box in fast preview window
        // we can fill it just now, because we need a OpenGL context, which was created now,
        // to check if all necessary extentions are available
        frame->FillBlendChoice();
    }
}

void GLPreview::setUp()
{

    // we need something to store the state of the view and control updates
    if (!m_view_state) {
        GLint countMultiTexture;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&countMultiTexture);
        m_view_state = new ViewState(m_pano, countMultiTexture>1);
    }
    m_visualization_state = new VisualizationState(m_pano, m_view_state, RefreshWrapper, this, (MeshManager*) NULL);
    //Start the tools going:
    PreviewToolHelper *helper = new PreviewToolHelper(m_pano, m_visualization_state, frame);
    m_tool_helper = (ToolHelper*) helper;
    frame->MakePreviewTools(helper);
    // now make a renderer
    m_renderer =  new GLPreviewRenderer(m_pano, m_view_state->GetTextureManager(),
                                 m_visualization_state->GetMeshManager(),
                                 m_visualization_state, helper);
}

void GLOverview::setUp()
{
    // we need something to store the state of the view and control updates
    if (!m_view_state) {
        GLint countMultiTexture;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&countMultiTexture);
        m_view_state = new ViewState(m_pano, countMultiTexture>1);
    }
    m_visualization_state = new PanosphereOverviewVisualizationState(m_pano, m_view_state, RefreshWrapper, this);
    //Start the tools going:
    OverviewToolHelper *helper = new OverviewToolHelper(m_pano, m_visualization_state, frame);
    m_tool_helper = (ToolHelper*) helper;
    frame->MakeOverviewTools(helper);
    // now make a renderer
    m_renderer =  new GLOverviewRenderer(m_pano, m_view_state->GetTextureManager(),
                                 m_visualization_state->GetMeshManager(),
                                 (PanosphereOverviewVisualizationState*) m_visualization_state, helper);
}

void GLViewer::SetPhotometricCorrect(bool state)
{
    m_view_state->GetTextureManager()->SetPhotometricCorrect(state);
    Refresh();
}

void GLViewer::SetLayoutMode(bool state)
{
    m_visualization_state->GetMeshManager()->SetLayoutMode(state);
    Refresh();
}

void GLViewer::SetLayoutScale(double scale)
{
    m_visualization_state->GetMeshManager()->SetLayoutScale(scale);
    Refresh();
}

void GLViewer::RedrawE(wxPaintEvent& e)
{
    //TODO: CanResize specific to a viewer?
    DEBUG_DEBUG("REDRAW_E");
    if(!IsShown()) return;
    // don't redraw during a redraw.
    if (!(frame->CanResize())) {
        DEBUG_DEBUG("RESIZE IN REDRAW");
        frame->ContinueResize();
        return;
    }
        
    if (!redrawing)
    {
        DEBUG_DEBUG("REDRAW_E IN");
        redrawing = true;
        SetUpContext();
        wxPaintDC dc(this); // we need this object on the stack to draw.
        Redraw();
        redrawing = false;
    }
}

void GLViewer::RefreshWrapper(void * obj)
{
    DEBUG_DEBUG("REFRESH WRAPPER");
    GLViewer* self = (GLViewer*) obj;
    self->Refresh();
}

void GLViewer::Resized(wxSizeEvent& e)
{

    DEBUG_DEBUG("RESIZED_OUT");
   
    if (frame->CanResize()) {
        frame->UpdateDocksSize();
        DEBUG_DEBUG("RESIZED_IN");
        wxGLCanvas::OnSize(e);
        if(!IsShown()) return;
        // if we have a render at this point, tell it the new size.
        if (m_renderer)
        {
          int w, h;
          GetClientSize(&w, &h);    
          SetUpContext();
          offset = m_renderer->Resize(w, h);
          Redraw();
        };
    }
}

void GLViewer::Redraw()
{
    // get the renderer to redraw the OpenGL stuff
    if(!m_renderer) return;
    DEBUG_INFO("Rendering.");
    
    // we should use the window background colour outside the panorama
    // FIXME shouldn't this work on textured backrounds?
    wxColour col = wxSystemSettings::GetColour(wxSYS_COLOUR_APPWORKSPACE);
    m_renderer->SetBackground(col.Red(), col.Green(), col.Blue());
    if (m_visualization_state->RequireRecalculateViewport())
    {
        // resize the viewport in case the panorama dimensions have changed.
        int w, h;
        GetClientSize(&w, &h);
        offset = m_renderer->Resize(w, h);
    }
    m_visualization_state->DoUpdates();
    m_renderer->Redraw();
    glFlush();
    SwapBuffers();
    // tell the view state we did all the updates and redrew.
    m_visualization_state->FinishedDraw();
    DEBUG_INFO("Finished Rendering.");
}

void GLViewer::OnEraseBackground(wxEraseEvent& e)
{
    // Do nothing, to avoid flashing on MSW
}

void GLViewer::MouseMotion(wxMouseEvent& e)
{
    if(m_renderer)
        m_tool_helper->MouseMoved((int) e.m_x - offset.x,
                              (int) e.m_y - offset.y, e);
}

void GLViewer::MouseLeave(wxMouseEvent & e)
{
    if(m_renderer)
        m_tool_helper->MouseLeave();
}

void GLViewer::LeftDown(wxMouseEvent& e)
{
    if(m_renderer)
        m_tool_helper->MouseButtonEvent(e);
}

void GLViewer::LeftUp(wxMouseEvent& e)
{
    if(m_renderer)
        m_tool_helper->MouseButtonEvent(e);
}

void GLViewer::RightDown(wxMouseEvent& e)
{
    if(m_renderer)
        m_tool_helper->MouseButtonEvent(e);
}

void GLViewer::RightUp(wxMouseEvent& e)
{
    if(m_renderer)
        m_tool_helper->MouseButtonEvent(e);
}

void GLViewer::KeyDown(wxKeyEvent& e)
{
    if(m_renderer)
        m_tool_helper->KeypressEvent(e.GetKeyCode(), e.GetModifiers(), true);
}

void GLViewer::KeyUp(wxKeyEvent& e)
{
    if(m_renderer)
        m_tool_helper->KeypressEvent(e.GetKeyCode(), e.GetModifiers(), false);
}


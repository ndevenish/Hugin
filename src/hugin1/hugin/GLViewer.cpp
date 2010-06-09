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
#include "PreviewToolHelper.h"
#include "GLPreviewFrame.h"
#include "hugin/huginApp.h"


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

GLViewer::GLViewer(wxWindow* parent, PT::Panorama &pano, int args[], GLPreviewFrame *frame_in) :
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
        
        // check, if gpu supports multitextures
        GLint countMultiTexture;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&countMultiTexture);

//        GLint countMultiTexture = 0;
        // we need something to store the state of the view and control updates
        m_view_state = new ViewState(m_pano, RefreshWrapper, countMultiTexture>1, this);
        //Start the tools going:
        m_tool_helper = new PreviewToolHelper(m_pano, m_view_state, frame);
        frame->MakeTools(m_tool_helper);
        // now make a renderer
        m_renderer =  new GLRenderer(m_pano, m_view_state->GetTextureManager(),
                                     m_view_state->GetMeshManager(),
                                     m_view_state, m_tool_helper);
//        m_renderer =  new GLRenderer(m_pano, NULL, NULL, NULL, NULL);
        // fill blend mode choice box in fast preview window
        // we can fill it just now, because we need a OpenGL context, which was created now,
        // to check if all necessary extentions are available
        frame->FillBlendChoice();
    }
}

void GLViewer::SetPhotometricCorrect(bool state)
{
    m_view_state->GetTextureManager()->SetPhotometricCorrect(state);
    Refresh();
}

void GLViewer::SetLayoutMode(bool state)
{
    m_view_state->GetMeshManager()->SetLayoutMode(state);
    Refresh();
}

void GLViewer::SetLayoutScale(double scale)
{
    m_view_state->GetMeshManager()->SetLayoutScale(scale);
    Refresh();
}

void GLViewer::RedrawE(wxPaintEvent& e)
{
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
    GLViewer* self = (GLViewer*) obj;
    self->Refresh();
}

void GLViewer::Resized(wxSizeEvent& e)
{

    std::cout << "RB" << std::endl;

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
    std::cout << "RE" << std::endl;
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
    if (m_view_state->RequireRecalculateViewport())
    {
        // resize the viewport in case the panorama dimensions have changed.
        int w, h;
        GetClientSize(&w, &h);
        offset = m_renderer->Resize(w, h);
    }
    m_view_state->DoUpdates();
    m_renderer->Redraw();
    glFlush();
    SwapBuffers();
    // tell the view state we did all the updates and redrew.
    m_view_state->FinishedDraw();
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


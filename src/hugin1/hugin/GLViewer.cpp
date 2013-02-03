// -*- c-basic-offset: 4 -*-
/** @file GLViewer.cpp
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

#if wxCHECK_VERSION(2,9,0)
#include "wx/textwrapper.h"
#else
// in wxWidgets 2.8 the text wrapper functions are in private scope of wxWidgets
// so we can't use them, used lines from src/common/dlgcmn.cpp
class wxTextWrapper
{
public:
    wxTextWrapper() { m_eol = false; }

    // win is used for getting the font, text is the text to wrap, width is the
    // max line width or -1 to disable wrapping
    void Wrap(wxWindow *win, const wxString& text, int widthMax)
    {
        const wxChar *lastSpace = NULL;
        wxString line;

        const wxChar *lineStart = text.c_str();
        for ( const wxChar *p = lineStart; ; p++ )
        {
            if ( IsStartOfNewLine() )
            {
                OnNewLine();

                lastSpace = NULL;
                line.clear();
                lineStart = p;
            }

            if ( *p == _T('\n') || *p == _T('\0') )
            {
                DoOutputLine(line);

                if ( *p == _T('\0') )
                    break;
            }
            else // not EOL
            {
                if ( *p == _T(' ') )
                    lastSpace = p;

                line += *p;

                if ( widthMax >= 0 && lastSpace )
                {
                    int width;
                    win->GetTextExtent(line, &width, NULL);

                    if ( width > widthMax )
                    {
                        // remove the last word from this line
                        line.erase(lastSpace - lineStart, p + 1 - lineStart);
                        DoOutputLine(line);

                        // go back to the last word of this line which we didn't
                        // output yet
                        p = lastSpace;
                    }
                }
                //else: no wrapping at all or impossible to wrap
            }
        }
    }

    // we don't need it, but just to avoid compiler warnings
    virtual ~wxTextWrapper() { }

protected:
    // line may be empty
    virtual void OnOutputLine(const wxString& line) = 0;

    // called at the start of every new line (except the very first one)
    virtual void OnNewLine() { }

private:
    // call OnOutputLine() and set m_eol to true
    void DoOutputLine(const wxString& line)
    {
        OnOutputLine(line);

        m_eol = true;
    }

    // this function is a destructive inspector: when it returns true it also
    // resets the flag to false so calling it again woulnd't return true any
    // more
    bool IsStartOfNewLine()
    {
        if ( !m_eol )
            return false;

        m_eol = false;

        return true;
    }


    bool m_eol;
};
#endif

// utility function to wrap text to given width, from wxWidgets help
wxString WrapText(wxWindow *win, const wxString& text, int widthMax)
{
    class HardBreakWrapper : public wxTextWrapper
    {
    public:
        HardBreakWrapper(wxWindow *win, const wxString& text, int widthMax)
        {
            Wrap(win, text, widthMax);
        }

        wxString const& GetWrapped() const { return m_wrapped; }

    protected:
        virtual void OnOutputLine(const wxString& line)
        {
            m_wrapped += line;
        }

        virtual void OnNewLine()
        {
            m_wrapped += '\n';
        }

    private:
        wxString m_wrapped;
    };

    HardBreakWrapper wrapper(win, text, widthMax);
    return wrapper.GetWrapped();
}

bool GLViewer::initialised_glew=false;
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
    EVT_MOUSEWHEEL(GLViewer::MouseWheel)
    EVT_MOUSE_EVENTS(GLViewer::MouseButtons)
    // keyboard events
    EVT_KEY_DOWN(GLViewer::KeyDown)
    EVT_KEY_UP(GLViewer::KeyUp)
END_EVENT_TABLE()


GLViewer::GLViewer(
            wxWindow* parent, 
            PT::Panorama &pano, 
            int args[], 
            GLPreviewFrame *frame_in,
            wxGLContext * shared_context
            ) :
#if defined __WXGTK__ || wxCHECK_VERSION(2,9,0)
          wxGLCanvas(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                     0, wxT("GLPreviewCanvas"), args, wxNullPalette)
#else
          wxGLCanvas(parent,shared_context,wxID_ANY,wxDefaultPosition,
                     wxDefaultSize,0,wxT("GLPreviewCanvas"),args,wxNullPalette)
#endif
{
    /* The openGL display context doesn't seem to be created automatically on
     * wxGTK, (wxMSW and wxMac 2.8 does implicit create wxGLContext,
     * wxWidgets 2.9 requires to explicit create wxGLContext, 
     * so I create a new context... */
#if defined __WXGTK__ || wxCHECK_VERSION(2,9,0)
    m_glContext = new wxGLContext(this, shared_context);
#endif
  
    m_renderer = 0;
    m_visualization_state = 0;
    
    m_pano = &pano;
    m_overlay=false;

    frame = frame_in;

    m_background_color = frame->GetPreviewBackgroundColor();
    
    started_creation = false;
    redrawing = false;

    active = true;
}

GLViewer::~GLViewer()
{
#if defined __WXGTK__ || wxCHECK_VERSION(2,9,0)
    delete m_glContext;
#endif
    if (m_renderer)
    {
      delete m_tool_helper;
      delete m_renderer;
      // because m_view_state is a static member variable we need to check
      // if other class has already deleted it
      if(m_view_state)
      {
        delete m_view_state;
        m_view_state=NULL;
      }
    }
}

void GLViewer::SetUpContext()
{
    // set the context
    DEBUG_INFO("Setting rendering context...");
    Show();
#if defined __WXGTK__ || wxCHECK_VERSION(2,9,0)
    m_glContext->SetCurrent(*this);
#else
    SetCurrent();
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
                wxMessageBox(_("Error initializing GLEW\nFast preview window can not be opened."),_("Error"), wxOK | wxICON_ERROR,wxTheApp->GetTopWindow());
                return;
            }
        }
        // check the openGL capabilities.
        if (!(GLEW_VERSION_1_1 && GLEW_ARB_multitexture))
        {
            started_creation=false;
            wxConfigBase::Get()->Write(wxT("DisableOpenGL"), 1l);
            wxConfigBase::Get()->Flush();
            DEBUG_ERROR("Sorry, OpenGL 1.1 + GL_ARB_multitexture extension required.");
            frame->Close();
            wxMessageBox(_("Sorry, the fast preview window requires a system which supports OpenGL version 1.1 with the GL_ARB_multitexture extension.\nThe fast preview cannot be opened.\n\nHugin has been configured to start without fast preview.\nPlease restart Hugin."),_("Error"), wxOK | wxICON_ERROR,wxTheApp->GetTopWindow());
            return;
        }

        setUp();
    }
}

void GLPreview::setUp()
{
    DEBUG_DEBUG("Preview Setup");
    // we need something to store the state of the view and control updates
    if (!m_view_state) {
        GLint countMultiTexture;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&countMultiTexture);
        m_view_state = new ViewState(m_pano, countMultiTexture>1);
    }
    m_visualization_state = new VisualizationState(m_pano, m_view_state, this, RefreshWrapper, this, (PreviewMeshManager*) NULL);
    //Start the tools going:
    PreviewToolHelper *helper = new PreviewToolHelper(m_pano, m_visualization_state, frame);
    m_tool_helper = (ToolHelper*) helper;
    frame->MakePreviewTools(helper);
    // now make a renderer
    m_renderer =  new GLPreviewRenderer(m_pano, m_view_state->GetTextureManager(),
                                 m_visualization_state->GetMeshManager(),
                                 m_visualization_state, helper, m_background_color);
}

void GLOverview::setUp()
{
DEBUG_DEBUG("Overview Setup");
    // we need something to store the state of the view and control updates
    if (!m_view_state) {
        GLint countMultiTexture;
        glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB,&countMultiTexture);
        m_view_state = new ViewState(m_pano, countMultiTexture>1);
    }

    panosphere_m_visualization_state = new PanosphereOverviewVisualizationState(m_pano, m_view_state, this, RefreshWrapper, this);
    plane_m_visualization_state = new PlaneOverviewVisualizationState(m_pano, m_view_state, this, RefreshWrapper, this);

    m_visualization_state = panosphere_m_visualization_state;

    //Start the tools going:
    panosphere_m_tool_helper = new PanosphereOverviewToolHelper(m_pano, panosphere_m_visualization_state, frame);
    frame->MakePanosphereOverviewTools(panosphere_m_tool_helper);

    plane_m_tool_helper = new PlaneOverviewToolHelper(m_pano, plane_m_visualization_state, frame);
    frame->MakePlaneOverviewTools(plane_m_tool_helper);

    // now make a renderer
    panosphere_m_renderer =  new GLPanosphereOverviewRenderer(m_pano, m_view_state->GetTextureManager(),
                                 panosphere_m_visualization_state->GetMeshManager(),
                                 panosphere_m_visualization_state, panosphere_m_tool_helper, m_background_color);
    plane_m_renderer =  new GLPlaneOverviewRenderer(m_pano, m_view_state->GetTextureManager(),
                                 plane_m_visualization_state->GetMeshManager(),
                                 plane_m_visualization_state, plane_m_tool_helper, m_background_color);
                                 
    switch(mode) {
        case PANOSPHERE:
            m_visualization_state = panosphere_m_visualization_state;
            m_tool_helper = panosphere_m_tool_helper;
            m_renderer = panosphere_m_renderer;
            break;
        case PLANE:
            m_visualization_state = plane_m_visualization_state;
            m_tool_helper = plane_m_tool_helper;
            m_renderer = plane_m_renderer;
            break;
    }
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

void GLOverview::SetLayoutMode(bool state)
{
    panosphere_m_visualization_state->GetMeshManager()->SetLayoutMode(state);
    plane_m_visualization_state->GetMeshManager()->SetLayoutMode(state);
    Refresh();
}

void GLOverview::SetLayoutScale(double scale)
{
    panosphere_m_visualization_state->GetMeshManager()->SetLayoutScale(scale*MeshManager::PanosphereOverviewMeshInfo::scale_diff);
    plane_m_visualization_state->GetMeshManager()->SetLayoutScale(scale);
    Refresh();
}


void GLViewer::RedrawE(wxPaintEvent& e)
{
    if (!IsActive()) {
        return;
    }
    
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
    DEBUG_DEBUG("END OF REDRAW_E");
}

void GLViewer::RefreshWrapper(void * obj)
{
    DEBUG_DEBUG("REFRESH WRAPPER");
    GLViewer* self = (GLViewer*) obj;
    self->Refresh();
}

void GLViewer::Resized(wxSizeEvent& e)
{

    if (!IsActive()) {
        return;
    }

    DEBUG_DEBUG("RESIZED_OUT");
   
    if (frame->CanResize()) {
        DEBUG_DEBUG("RESIZED_IN");
        wxGLCanvas::OnSize(e);
        if(!IsShown()) return;
        // if we have a render at this point, tell it the new size.
        DEBUG_DEBUG("RESIZED_IN_SHOWN");
        if (m_renderer)
        {
          int w, h;
          DEBUG_DEBUG("RESIZED_IN_RENDERER");
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
    //finally draw the overlay text above all
    if(m_overlay && !m_overlayText.IsEmpty())
    {
        int w, h;
        GetClientSize(&w, &h);
        wxClientDC dc(this);
        PrepareDC(dc);
        dc.SetBackgroundMode(wxSOLID);
        dc.DrawLabel(WrapText(this,m_overlayText,w/4),wxRect(0,0,w,h),wxALIGN_RIGHT|wxALIGN_TOP);
    };
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

void GLViewer::MouseButtons(wxMouseEvent& e)
{
    if(m_renderer) {
        //disregard non button events
        if (e.IsButton()) {
            m_tool_helper->MouseButtonEvent(e);
        }
    }
#ifdef __WXMSW__
    //use normal mouse button processing of GLCanvas 
    //otherwise the mouse wheel is not working
    e.Skip();
#endif
}

void GLViewer::MouseWheel(wxMouseEvent& e)
{
    if(m_renderer) {
        m_tool_helper->MouseWheelEvent(e);
    }
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

void GLViewer::SetViewerBackground(wxColour col)
{
    this->m_background_color = col;
    if(m_renderer)
        m_renderer->SetPreviewBackgroundColor(col);
}

void GLOverview::SetMode(OverviewMode mode)
{
    this->mode = mode;
    if (panosphere_m_renderer != 0 && plane_m_renderer != 0) {
        switch(mode) {
            case PANOSPHERE:
                m_visualization_state = panosphere_m_visualization_state;
                m_tool_helper = panosphere_m_tool_helper;
                m_renderer = panosphere_m_renderer;
                break;
            case PLANE:
                m_visualization_state = plane_m_visualization_state;
                m_tool_helper = plane_m_tool_helper;
                m_renderer = plane_m_renderer;
                break;
        }
        this->Refresh();
    }
}

void GLViewer::SetOverlayText(const wxString text)
{
    m_overlayText=text;
};

void GLViewer::SetOverlayVisibility(const bool isVisible)
{
    m_overlay=isVisible;
};


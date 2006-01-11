// -*- c-basic-offset: 4 -*-

/** @file ImagesPanel.cpp
 *
 *  @brief implementation of ImagesCenter Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
 *
 *  $Id$
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

#include <config.h>
#include "panoinc_WX.h"

#include "panoinc.h"

#include "hugin/huginApp.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/ImageCenter.h"
#include "hugin/TextKillFocusHandler.h"

using namespace PT;
using namespace utils;
using namespace std;
using namespace vigra;

#define ID_EDIT_LEFT    (wxID_HIGHEST + 1)
#define ID_EDIT_RIGHT   (wxID_HIGHEST + 2)
#define ID_EDIT_TOP     (wxID_HIGHEST + 3)
#define ID_EDIT_BOTTOM  (wxID_HIGHEST + 4)
#define ID_BUT_RESET    (wxID_HIGHEST + 5)
#define ID_CB_AUTOCENTER (wxID_HIGHEST + 6)

BEGIN_EVENT_TABLE(ImgCenter, wxDialog)
    EVT_CLOSE   ( ImgCenter::OnClose )
    EVT_TEXT_ENTER (ID_EDIT_LEFT ,ImgCenter::OnSetLeft )
    EVT_TEXT_ENTER (ID_EDIT_RIGHT ,ImgCenter::OnSetRight )
    EVT_TEXT_ENTER (ID_EDIT_TOP ,ImgCenter::OnSetTop )
    EVT_TEXT_ENTER (ID_EDIT_BOTTOM ,ImgCenter::OnSetBottom )
    EVT_BUTTON ( ID_BUT_RESET, ImgCenter::OnResetButton )
    EVT_CHECKBOX( ID_CB_AUTOCENTER, ImgCenter::OnAutoCenter)
END_EVENT_TABLE()

// Define a constructor for my canvas
ImgCenter::ImgCenter(wxWindow *parent)
  : wxDialog(parent, -1, _("Crop Image"), wxDefaultPosition, wxDefaultSize,
    wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX)
{
    DEBUG_TRACE("");

//    wxTheXmlResource->LoadDialog(this, wxT("image_center_dialog"));

    // layout the dialog manually
    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    m_Canvas = new CenterCanvas (this,this);
    m_Canvas->SetMinSize(wxSize(400,400));

    topsizer->Add(m_Canvas,
                  1,        // vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width

    wxBoxSizer * buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    buttonSizer->Add(new wxStaticText(this, -1, _("Left:")),0, wxALIGN_CENTRE_VERTICAL);
    m_left_textctrl = new wxTextCtrl(this, ID_EDIT_LEFT);
    m_left_textctrl->PushEventHandler(new TextKillFocusHandler(this));
    buttonSizer->Add(m_left_textctrl);

    buttonSizer->Add(new wxStaticText(this, -1, _("Right:")),0, wxALIGN_CENTRE_VERTICAL);
    m_right_textctrl = new wxTextCtrl(this, ID_EDIT_RIGHT);
    m_right_textctrl->PushEventHandler(new TextKillFocusHandler(this));
    buttonSizer->Add(m_right_textctrl);

    buttonSizer->Add(new wxStaticText(this, -1, _("Top:")),0, wxALIGN_CENTRE_VERTICAL);
    m_top_textctrl = new wxTextCtrl(this, ID_EDIT_TOP);
    m_top_textctrl->PushEventHandler(new TextKillFocusHandler(this));
    buttonSizer->Add(m_top_textctrl);

    buttonSizer->Add(new wxStaticText(this, -1, _("Bottom:")), 0, wxALIGN_CENTRE_VERTICAL);
    m_bottom_textctrl = new wxTextCtrl(this, ID_EDIT_BOTTOM);
    m_bottom_textctrl->PushEventHandler(new TextKillFocusHandler(this));
    buttonSizer->Add(m_bottom_textctrl);

    buttonSizer->Add(new wxButton(this, ID_BUT_RESET, _("Reset")));

    m_autocenter_cb = new wxCheckBox(this, ID_CB_AUTOCENTER, _("Always center Crop on d,e"));
    buttonSizer->Add(m_autocenter_cb);

    wxBoxSizer * buttonSizer2 = new wxBoxSizer(wxHORIZONTAL);

    wxButton * but_OK = new wxButton(this, wxID_OK);
    buttonSizer2->Add(but_OK, 0, wxEXPAND | wxALIGN_RIGHT | wxALL, 5 );

    wxButton * but_CANCEL = new wxButton(this, wxID_CANCEL);
    buttonSizer2->Add(but_CANCEL, 0, wxEXPAND | wxALL, 5 );

    topsizer->Add(buttonSizer,
                  0,        // vertically stretchable
                  wxEXPAND | // horizontally stretchable
                  wxALL,    // draw border all around
                  5);       // border width

    topsizer->Add(buttonSizer2,
                  0,        // vertically stretchable
                  wxEXPAND | // horizontally stretchable
                          wxALL,    // draw border all around
                  5);       // border width

    // get the global config object

    SetSizer( topsizer );
    topsizer->SetSizeHints( this );

    RestoreFramePosition(this, wxT("CropDialog"));

    m_Canvas->Show();
    DEBUG_TRACE("");
}

ImgCenter::~ImgCenter(void)
{
    DEBUG_TRACE("");
    m_left_textctrl->PopEventHandler(true);
    m_right_textctrl->PopEventHandler(true);
    m_top_textctrl->PopEventHandler(true);
    m_bottom_textctrl->PopEventHandler(true);

}

vigra::Rect2D & ImgCenter::getCrop()
{
    return m_Canvas->getCrop();
}

bool ImgCenter::getCenterOnDE()
{
    return m_autocenter_cb->IsChecked();
}

void ImgCenter::SetImage (wxImage & s_img)
{
    m_Canvas -> SetImage(s_img);
}

void ImgCenter::CropChanged(vigra::Rect2D & crop)
{
    m_roi = crop;
    UpdateDisplay();
}


void ImgCenter::SetParameters (const vigra::Rect2D & crop, bool circle,
                               const vigra::Point2D & center, bool useCenter)
{
    DEBUG_TRACE("");
    m_roi = crop;
    m_circle = circle;
    m_center = center;
    m_autocenter_cb->SetValue(useCenter);
    UpdateDisplay();
    m_Canvas -> SetParameters(crop, circle, center, useCenter);
    DEBUG_TRACE("");
}

void ImgCenter::OnClose ( wxCloseEvent & e )
{
    DEBUG_TRACE("");
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    config->Write(wxT("CenterDialog/Width"),GetRect().width),
    config->Write(wxT("CenterDialog/Height"),GetRect().height);
    DEBUG_INFO( "saved last size" )

    Destroy();
    DEBUG_TRACE("");
}

void ImgCenter::UpdateDisplay()
{
    m_left_textctrl->SetValue(wxString::Format(wxT("%d"),m_roi.left()));
    m_right_textctrl->SetValue(wxString::Format(wxT("%d"),m_roi.right()));
    m_top_textctrl->SetValue(wxString::Format(wxT("%d"),m_roi.top()));
    m_bottom_textctrl->SetValue(wxString::Format(wxT("%d"),m_roi.bottom()));
}

void ImgCenter::OnSetTop(wxCommandEvent & e)
{
    long val;
    if (m_top_textctrl->GetValue().ToLong(&val)) {
        m_roi.setUpperLeft(Point2D(m_roi.left(), val));
        if (getCenterOnDE()) {
            CenterCrop();
            UpdateDisplay();
        }
        m_Canvas->SetParameters(m_roi, m_circle, m_center, getCenterOnDE());
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void ImgCenter::OnSetBottom(wxCommandEvent & e)
{
    long val;
    if (m_bottom_textctrl->GetValue().ToLong(&val)) {
        m_roi.setLowerRight(Point2D(m_roi.right(), val));
        if (getCenterOnDE()) {
            CenterCrop();
            UpdateDisplay();
        }
        m_Canvas->SetParameters(m_roi, m_circle, m_center, getCenterOnDE());
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void ImgCenter::OnSetLeft(wxCommandEvent & e)
{
    long val = 0;
    if (m_left_textctrl->GetValue().ToLong(&val)) {
        m_roi.setUpperLeft(Point2D(val, m_roi.top()));
        if (getCenterOnDE()) {
            CenterCrop();
            UpdateDisplay();
        }
        m_Canvas->SetParameters(m_roi, m_circle, m_center, getCenterOnDE());
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void ImgCenter::OnSetRight(wxCommandEvent & e)
{
    long val = 0;
    if (m_right_textctrl->GetValue().ToLong(&val)) {
        m_roi.setLowerRight(Point2D(val, m_roi.bottom()));
        if (getCenterOnDE()) {
            CenterCrop();
            UpdateDisplay();
        }
        m_Canvas->SetParameters(m_roi, m_circle, m_center, getCenterOnDE());
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void ImgCenter::OnResetButton(wxCommandEvent & e)
{
    m_roi.setUpperLeft(Point2D(0,0));
    m_roi.setLowerRight(Point2D(0,0));
    m_autocenter_cb->SetValue(false);
    UpdateDisplay();
    m_Canvas->SetParameters(m_roi, m_circle, m_center, getCenterOnDE());
}

void ImgCenter::OnAutoCenter(wxCommandEvent & e)
{
    if (e.IsChecked()) {
        CenterCrop();
        m_Canvas->SetParameters(m_roi, m_circle, m_center, getCenterOnDE());
    }
}

void ImgCenter::CenterCrop()
{
    Diff2D d(m_roi.width()/2, m_roi.height() / 2);
    m_roi.setUpperLeft( m_center - d);
    m_roi.setLowerRight( m_center + d);
}

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CenterCanvas, wxPanel)
    EVT_SIZE   ( CenterCanvas::Resize )
    EVT_MOUSE_EVENTS ( CenterCanvas::OnMouse )
    EVT_PAINT ( CenterCanvas::OnPaint )
END_EVENT_TABLE()

// Define a constructor for my canvas
CenterCanvas::CenterCanvas(wxWindow *parent, ImgCenter * listener)
  : wxPanel(parent, -1),
    m_listener(listener)
{
    DEBUG_TRACE("");
    m_state = NO_SEL;

    m_cursor_no_sel = new wxCursor(wxCURSOR_CROSS);
    m_cursor_circ_drag = new wxCursor(wxCURSOR_SIZING);
    m_cursor_move_crop = new wxCursor(wxCURSOR_HAND);
    m_cursor_drag_vert = new wxCursor(wxCURSOR_SIZENS);
    m_cursor_drag_horiz = new wxCursor(wxCURSOR_SIZEWE);
    SetCursor(*m_cursor_no_sel);
}

CenterCanvas::~CenterCanvas(void)
{
    DEBUG_TRACE("");
    delete m_cursor_no_sel;
    delete m_cursor_circ_drag;
    delete m_cursor_move_crop;
    delete m_cursor_drag_vert;
    delete m_cursor_drag_horiz;
}

void CenterCanvas::DrawView()
{
      // now show the position of current PT shift (d,e)
      wxMemoryDC memDC;
      m_display_img.Create(m_scaled_img.GetWidth(), m_scaled_img.GetHeight());
      memDC.SelectObject (m_display_img);
      memDC.BeginDrawing ();
      // copy resized image into buffer
      memDC.DrawBitmap(m_scaled_img,0,0,false);

      // draw all areas without fillings
      wxBrush brush = memDC.GetBrush();
      brush.SetStyle (wxTRANSPARENT);
      memDC.SetBrush (brush);
      memDC.SetLogicalFunction (wxINVERT);
      // draw text without background
      memDC.SetBackgroundMode (wxTRANSPARENT);
      // draw circle, if needed.
      if (!m_roi.isEmpty()) {
          // draw region of interest
          int mid_x = roundi(m_scale * (m_roi.left() + m_roi.width()/2.0));
          int mid_y = roundi(m_scale * (m_roi.top() + m_roi.height()/2.0));

          int c = 8; // size of midpoint cross
          memDC.DrawLine( mid_x + c, mid_y + c,
                          mid_x - c, mid_y - c);
          memDC.DrawLine( mid_x - c, mid_y + c,
                          mid_x + c, mid_y - c);
          // draw circle
          // draw rectangle
          memDC.DrawRectangle(roundi(m_scale*m_roi.left()),
                              roundi(m_scale*m_roi.top()),
                              roundi(m_scale*m_roi.width()),
                              roundi(m_scale*m_roi.height()));

          // draw crop circle as well, if requested.
          if (m_circle) {
              int radius = roundi(m_scale * std::min(m_roi.width(), m_roi.height())/2);
              memDC.DrawCircle ( mid_x, mid_y, radius );
          }
          // draw helping lines while dragging
          if (m_state == CIRC_DRAGGING) {
              memDC.SetBrush(wxBrush(wxT("RED"),wxTRANSPARENT));
              memDC.SetPen(wxPen(wxT("RED"), 1, wxSOLID));
              memDC.SetLogicalFunction (wxXOR);
              memDC.DrawLine(roundi(m_scale * m_circ_first_point.x),
                             roundi(m_scale * m_circ_first_point.y),
                             roundi(m_scale * m_circ_second_point.x),
                             roundi(m_scale * m_circ_second_point.y) );

              memDC.DrawCircle(roundi(m_scale * m_circ_first_point.x),
                               roundi(m_scale * m_circ_first_point.y), 7);

              memDC.DrawCircle(roundi(m_scale * m_circ_second_point.x),
                               roundi(m_scale * m_circ_second_point.y), 7);
          }
      }
//      memDC.SelectObject(wxNullBitmap);
      memDC.EndDrawing ();

}

void CenterCanvas::ChangeMode(PointState m)
{
    switch (m) {
    case NO_SEL:
        SetCursor(*m_cursor_no_sel);
        break;

    case RECT_DRAGGING:
    case CIRC_DRAGGING:
        SetCursor(*m_cursor_circ_drag);
        break;

    case CROP_SELECTED:
    case CROP_MOVE:
        SetCursor(*m_cursor_move_crop);
        SetCursor(*m_cursor_move_crop);
        break;

    case RECT_LEFT_DRAG:
    case RECT_RIGHT_DRAG:
        SetCursor(*m_cursor_drag_horiz);
        break;

    case RECT_TOP_DRAG:
    case RECT_BOTTOM_DRAG:
        SetCursor(*m_cursor_drag_vert);
        break;

    }
    m_state = m;
}


void CenterCanvas::Resize( wxSizeEvent & e )
{
    if ( img.Ok() )
    {
      int x = GetSize().x;
      int y = GetSize().y;
      DEBUG_INFO( "x "<< x <<" y "<< y );

      // scale to fit the window
      {
          int new_width;
          int new_height;

          float r_img = (float)img.GetWidth() / (float)img.GetHeight();
          float r_window = (float)x/(float)y;
          if ( r_img > r_window ) {
            m_scale = (float)x / img.GetWidth();
            new_width =  x;
            new_height = roundi(m_scale * img.GetHeight());
          } else {
            m_scale = (float)y / img.GetHeight();
            new_height = y;
            new_width = roundi(m_scale * img.GetWidth());
          }
          m_scaled_img = wxBitmap(img.Scale (new_width, new_height));
      }

      // draw new view into offscreen buffer
      DrawView();
      // eventually update the view
      Refresh(false);
    }
    DEBUG_TRACE ("end")
}

void CenterCanvas::UpdateCropCircle()
{
    if (m_listener) {
        m_listener->CropChanged(m_roi);
    }
    DrawView();
    Refresh(false);
}


// Define the repainting behaviour
void CenterCanvas::OnPaint(wxPaintEvent & dc)
{
    wxPaintDC paintDC( this );
    if ( m_display_img.Ok() )
    {
        paintDC.DrawBitmap(m_display_img, 0,0, FALSE);
    }
}

void CenterCanvas::SetImage(wxImage & s_img)
{
    img = s_img;
    ChangeMode(NO_SEL);
    m_roi.setUpperLeft(Point2D(0,0));
    m_roi.setLowerRight(Point2D(0,0));

    wxSizeEvent e;
    Resize (e);
    // draw new view into offscreen buffer
    DrawView();
    // eventually update the view
    Refresh(false);
}

void CenterCanvas::SetParameters (const vigra::Rect2D & crop, bool circle,
                                  const vigra::Point2D & center, bool useCenter)
{
    DEBUG_TRACE("");

    m_roi = crop;
    m_circle = circle;
    m_center = center;
    m_centered = useCenter;
    if (crop.isEmpty()) {
        ChangeMode(NO_SEL);
    } else {
        ChangeMode(CROP_SELECTED);
    }

    // draw new view into offscreen buffer
    DrawView();
    // eventually update the view
    Refresh(false);

    DEBUG_TRACE("");
}

//#define MAX(a,b) ( ( a > b ) ? a : b)
//#define MIN(a,b) ( ( a < b ) ? a : b)

void CenterCanvas::OnMouse ( wxMouseEvent & e )
{
    if (!img.Ok()) {
        // nothing to do if we don't have an image
        return;
    }

    double xpos = e.m_x / m_scale;
    int xpos_i = roundi(xpos);
    double ypos = e.m_y / m_scale;
    int ypos_i = roundi(ypos);
    vigra::Point2D mpos(xpos_i, ypos_i);

    switch (m_state) {
        case NO_SEL:
            if (e.LeftDown()) {
                if (m_circle) {
                    // set starting point
                    m_circ_first_point.x = xpos_i;
                    m_circ_first_point.y = ypos_i;
                    ChangeMode(CIRC_DRAGGING);
                } else {
                    // set starting point
                    vigra::Point2D p(xpos_i, ypos_i);
                    m_roi.setUpperLeft(p);
                    m_roi.setLowerRight(p);
                    ChangeMode(RECT_DRAGGING);
                }
            }
            break;
        case RECT_DRAGGING:
            {
                vigra::Point2D fp(xpos_i, ypos_i);
                if (m_centered) {
                    // special case for centered crops
                    vigra::Diff2D d = fp - m_center;
                    vigra::Point2D sp(m_center - d);
                    m_roi.setUpperLeft(sp);
                    m_roi.setLowerRight(fp);
                } else {
                    m_roi.setLowerRight(fp);
                }
                UpdateCropCircle();
                if (e.LeftUp()) {
                    // stop dragging
                    ChangeMode(CROP_SELECTED);
                }
            }
            break;
        case CIRC_DRAGGING:
            m_circ_second_point.x = xpos_i;
            m_circ_second_point.y = ypos_i;
            if (m_centered) {
                double dx = xpos - m_center.x;
                double dy = ypos - m_center.y;
                double r = sqrt(dx*dx+dy*dy);
                Diff2D d(roundi(r), roundi(r));
                m_roi.setUpperLeft(m_center - d);
                m_roi.setLowerRight(m_center + d);
            } else {
                // calculate circle from two points
                m_roi = calcCircleROIFromPoints(m_circ_first_point, m_circ_second_point);
            }
            UpdateCropCircle();

            if (e.LeftUp()) {
                // stop dragging
                ChangeMode(CROP_SELECTED);
                UpdateCropCircle();
            }
            break;
        case CROP_SELECTED:
            if (m_circle) {
                vigra::Point2D mid (m_roi.left() + m_roi.width() / 2, m_roi.top() + m_roi.height() / 2);
                vigra::Diff2D mouse_d = mpos - mid;
                double r_mouse =  sqrt((double)mouse_d.x*mouse_d.x + mouse_d.y*mouse_d.y);
                double r_circ = std::min(m_roi.width(), m_roi.height())/2.0;
                if (abs(r_mouse - r_circ) < 2 / m_scale) {
                    // we are close to a point on the circle
                    SetCursor(*m_cursor_circ_drag);
                    if (e.LeftDown()) {
                        // ok, determine first point, on the other side of the currently selected point.
                        // angle
                        double angle = atan2((double)mouse_d.y, (double)mouse_d.x);
                        double opp_angle = angle + M_PI;
                        // calculate point there.
                        double nx = cos(opp_angle) * r_circ;
                        double ny = sin(opp_angle) * r_circ;
                        m_circ_first_point.x = roundi(mid.x + nx);
                        m_circ_first_point.y = roundi(mid.y + ny);
                        ChangeMode(CIRC_DRAGGING);
                    }
                } else {
                    if (&GetCursor() != m_cursor_move_crop) {
                        SetCursor(*m_cursor_move_crop);
                    }
                }
            }
            // no treatment for rectangular crop area yet.
            // check if mouse is on a rectangular crop border

            if (abs(xpos_i - m_roi.left()) < 2 / m_scale && ypos_i > m_roi.top() && ypos_i < m_roi.bottom()) {
                SetCursor(*m_cursor_drag_horiz);
                if (e.LeftDown()) {
                    m_moveAnchor = e.GetPosition();
                    ChangeMode(RECT_LEFT_DRAG);
                }
            }

            if (abs(xpos_i - m_roi.right()) < 2 / m_scale && ypos_i > m_roi.top() && ypos_i < m_roi.bottom()) {
                SetCursor(*m_cursor_drag_horiz);
                if (e.LeftDown()) {
                    m_moveAnchor = e.GetPosition();
                    ChangeMode(RECT_RIGHT_DRAG);
                }
            }
            if (abs(ypos_i - m_roi.top()) < 2 / m_scale && xpos_i > m_roi.left() && xpos_i < m_roi.right()) {
                SetCursor(*m_cursor_drag_vert);
                if (e.LeftDown()) {
                    m_moveAnchor = e.GetPosition();
                    ChangeMode(RECT_TOP_DRAG);
                }
            }

            if (abs(ypos_i - m_roi.bottom()) < 2 / m_scale && xpos_i > m_roi.left() && xpos_i < m_roi.right()) {
                SetCursor(*m_cursor_drag_vert);
                if (e.LeftDown()) {
                    m_moveAnchor = e.GetPosition();
                    ChangeMode(RECT_BOTTOM_DRAG);
                }
            }

            // check for moving of roi if nothing else was done, and it is allowed
            if (m_state == CROP_SELECTED && ! m_centered) {
                if ( e.LeftDown() ) {
                    m_moveAnchor = e.GetPosition();
                    ChangeMode(CROP_MOVE);
                }
            }
            break;
        case CROP_MOVE:
            {
            if (e.LeftUp()) {
                ChangeMode(CROP_SELECTED);
            }
            int dx = e.m_x - m_moveAnchor.x;
            int dy = e.m_y - m_moveAnchor.y;
            m_moveAnchor = e.GetPosition();
            m_roi.moveBy(roundi(dx/m_scale), roundi(dy/m_scale));
            UpdateCropCircle();
            break;
            }
        case RECT_LEFT_DRAG:
            {
            if (e.LeftUp()) {
                ChangeMode(CROP_SELECTED);
            }
            if (m_centered) {
                int dx = abs(xpos_i - m_center.x);
                m_roi.setUpperLeft(Point2D(roundi(m_center.x - dx), m_roi.top()));
                m_roi.setLowerRight(Point2D(roundi(m_center.x + dx), m_roi.bottom()));
            } else {
                int dx = e.m_x - m_moveAnchor.x;
                m_moveAnchor = e.GetPosition();
                m_roi.setUpperLeft(Point2D(roundi(m_roi.left() + dx/m_scale), m_roi.top()));
            }
            UpdateCropCircle();
            }
            break;

        case RECT_RIGHT_DRAG:
            {
            if (e.LeftUp()) {
                ChangeMode(CROP_SELECTED);
            }
            if (m_centered) {
                int dx = abs(xpos_i - m_center.x);
                m_roi.setUpperLeft(Point2D(roundi(m_center.x - dx), m_roi.top()));
                m_roi.setLowerRight(Point2D(roundi(m_center.x + dx), m_roi.bottom()));
            } else {
                int dx = e.m_x - m_moveAnchor.x;
                m_moveAnchor = e.GetPosition();
                m_roi.setLowerRight(Point2D(roundi(m_roi.right() + dx/m_scale), m_roi.bottom()));
            }
            UpdateCropCircle();
            }
            break;

        case RECT_TOP_DRAG:
            {
            if (e.LeftUp()) {
                ChangeMode(CROP_SELECTED);
            }
            if (m_centered) {
                int dy = abs(ypos_i - m_center.y);
                m_roi.setUpperLeft(Point2D(m_roi.left(), roundi(m_center.y - dy)));
                m_roi.setLowerRight(Point2D(m_roi.right(), roundi(m_center.y + dy)));
            } else {
                int dy = e.m_y - m_moveAnchor.y;
                m_moveAnchor = e.GetPosition();
                m_roi.setUpperLeft(Point2D(m_roi.left(), roundi(m_roi.top() + dy/m_scale)));
            }
            UpdateCropCircle();
            }
            break;

        case RECT_BOTTOM_DRAG:
            {
            if (e.LeftUp()) {
                ChangeMode(CROP_SELECTED);
            }
            if (m_centered) {
                int dy = abs(ypos_i - m_center.y);
                m_roi.setUpperLeft(Point2D(m_roi.left(), roundi(m_center.y - dy)));
                m_roi.setLowerRight(Point2D(m_roi.right(), roundi(m_center.y + dy)));
            } else {
                int dy = e.m_y - m_moveAnchor.y;
                m_moveAnchor = e.GetPosition();
                m_roi.setLowerRight(Point2D(m_roi.right(), roundi(m_roi.bottom() + dy/m_scale)));
            }
            UpdateCropCircle();
            }
            break;

    }
}

    // rewrite
#if 0
    int dist_x, dist_y, s_x, s_y, mid_x, mid_y, radius;
    bool isDrag = false;
    if ( e.m_altDown )
      isDrag = true;
    bool isSetting = false;
    if ( e.m_leftDown )
      isSetting = true;


    // set drawing variables
    if ( isSetting && !isDrag ) {
      if ( !first_is_set ) {
        first_x = e.m_x;
        first_y = e.m_y;
        first_is_set = true ;
      } else if ( first_is_set && !second_is_set ) {
        second_x = e.m_x;
        second_y = e.m_y;
        second_is_set = true;
      } else if ( first_is_set && second_is_set ) {
        first_x = e.m_x;
        first_y = e.m_y;
        second_is_set = false;
      }

      // small message
      if ( (e.m_x < c_img.GetWidth()) && (e.m_y < c_img.GetHeight()) ) {
//        int pos_x = (int)((float)e.m_x * zoom) + 1;
//        int pos_y = (int)((float)e.m_y * zoom) + 1;
//        int x = (int)((float)c_img.GetWidth() * zoom);
//        int y = (int)((float)c_img.GetHeight() * zoom);
//        DEBUG_INFO( "m"<< e.m_x <<"x"<< e.m_y <<" pos"<<pos_x <<"x"<< pos_y <<" image"<< x <<"x"<< y <<" inside" )
      } else {
        DEBUG_INFO( "outside" )
      }
    } else if ( first_is_set && second_is_set && isDrag ) {
      // moving the circle around
      first_x += e.m_x - old_m_x;
      first_y += e.m_y - old_m_y;
      second_x += e.m_x - old_m_x;
      second_y += e.m_y - old_m_y;
      s_x = second_x;
      s_y = second_y;
      DEBUG_INFO( "m"<< e.m_x <<"x"<< e.m_y <<" old_m"<<old_m_x <<"x"<< old_m_y <<" second"<< second_x <<"x"<< second_y )
    } else if ( first_is_set && !second_is_set ) {
      // update coordinates
      s_x = e.m_x;
      s_y = e.m_y;
    } else if ( first_is_set && second_is_set ) {
      s_x = second_x;
      s_y = second_y;
    }
    old_m_x = e.m_x;
    old_m_y = e.m_y;

    if ( first_is_set ) {
          // vertical and horicontal distance between two mouse clicks
          dist_x = (max(s_x,first_x)-min(s_x,first_x))/2;
          dist_y = (max(s_y,first_y)-min(s_y,first_y))/2;
          // vertical and horicontal middle point between two mouse clicks
          mid_x = min(s_x,first_x) + dist_x;
          mid_y = min(s_y,first_y) + dist_y;
          // half distance between two mouse clicks
          radius = (int)std::sqrt((float)dist_x*dist_x + dist_y*dist_y);
    }




    // show some drawing
    if ( first_is_set && (!second_is_set || isDrag) ) {

      // select an fresh image to draw on an real! copy
      wxPaintDC paintDC( this );
      wxMemoryDC memDC;
      memDC.SelectObject (dirty_img);
      memDC.BeginDrawing ();
#if 1
      wxMemoryDC sourceDC;
      sourceDC.SelectObject (c_img);
      // clear the drawing image
      memDC.Blit(0, 0, c_img.GetWidth(), c_img.GetHeight(), & sourceDC,
                 0, 0, wxCOPY, FALSE);

      // need to cover the whole circle
/*      memDC.Blit( min(first_x,second_x),  min(first_y,second_y),
                    MAX(first_x, second_x) - min(first_x, second_x),
                    MAX(first_y,second_y) - min(first_y,second_y),
                    & sourceDC,
                    min(first_x,second_x),  min(first_y,second_y), wxCOPY, FALSE);
*///      DEBUG_INFO( "m"<< e.m_x <<"x"<< e.m_y <<" w/h"<<first_x <<"x"<< first_y <<" center"<< second_x <<"x"<< second_y <<" min_pos"<< min(first_x,second_x) <<"x"<< min(first_y,second_y) <<" size"<< MAX(first_x, second_x) - min(first_x, second_x) <<"x"<< MAX(first_y,second_y) - min(first_y,second_y) )
      sourceDC.SelectObject(wxNullBitmap);
#else
      wxRect rect ( min(first_x,second_x), min(first_y,second_y),
                    max(first_x,second_x)-min(first_x,second_x),
                    max(first_y,second_y)-min(first_y,second_y));

      Refresh (TRUE,&rect);
#endif
      wxPaintEvent dummyE;
      OnPaint (dummyE);

      // draw all areas without fillings
      wxBrush brush = memDC.GetBrush ();
      brush.SetStyle (wxTRANSPARENT);
      memDC.SetBrush (brush);
      memDC.SetLogicalFunction (wxINVERT);
      // draw text without background
      memDC.SetBackgroundMode (wxTRANSPARENT);
      // draw an diagonal line
      memDC.DrawLine( first_x, first_y, s_x, s_y );
      memDC.DrawCircle ( mid_x, mid_y, radius );
      // draw the midpoint and the coordinates
      int c = 5; // size of midpoint cross
      // calculate PT variables - lens shift
      pt_d = (zoom * (float)mid_x) - (float)img.GetWidth()/2.0;
      pt_e = (zoom * (float)mid_y) - (float)img.GetHeight()/2.0;
      if ( (first_x < s_x && first_y > s_y)  // 1th and 3th quadrants
           || (first_x > s_x && first_y < s_y) ) {
        memDC.DrawLine( mid_x + c, mid_y + c,
                        mid_x - c, mid_y - c );
        memDC.DrawText( wxString::Format(wxT("%d,%d"), (int)pt_d, (int)pt_e),
                        mid_x + 10, mid_y + 10 );
      } else {
        memDC.DrawLine( mid_x - c, mid_y + c,
                        mid_x + c, mid_y - c );
        memDC.DrawText( wxString::Format(wxT("%d,%d"), (int)pt_d, (int)pt_e),
                        mid_x + 10, mid_y - 15 );
      }
      memDC.EndDrawing ();
      // Transparent bliting if there's a mask in the bitmap
#if 0 // timeconsuming but save
      paintDC.Blit(0, 0, c_img.GetWidth(), c_img.GetHeight(), & memDC,
                 0, 0, wxCOPY, FALSE);
#else // qick
      int rad = (radius > 85) ? radius : 85;
      paintDC.Blit(mid_x - rad, mid_y - rad,
                   2 * rad + 1, 2 * rad + 1, & memDC,
                   mid_x - rad, mid_y - rad, wxCOPY, FALSE);
#endif
      memDC.SelectObject(wxNullBitmap);
    }
//    DEBUG_INFO ( "Mouse " << e.m_x <<"x"<< e.m_y);

#endif


// -*- c-basic-offset: 4 -*-

/** @file ImagesPanel.cpp
 *
 *  @brief implementation of ImagesCenter Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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
#include "base_wx/ImageCache.h"
#include "hugin/CropPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/TextKillFocusHandler.h"

using namespace PT;
using namespace utils;
using namespace std;
using namespace vigra;

/*
#define ID_EDIT_LEFT    (wxID_HIGHEST + 1)
#define ID_EDIT_RIGHT   (wxID_HIGHEST + 2)
#define ID_EDIT_TOP     (wxID_HIGHEST + 3)
#define ID_EDIT_BOTTOM  (wxID_HIGHEST + 4)
#define ID_BUT_RESET    (wxID_HIGHEST + 5)
#define ID_CB_AUTOCENTER (wxID_HIGHEST + 6)
*/

BEGIN_EVENT_TABLE(CropPanel, wxPanel)
    EVT_LIST_ITEM_SELECTED( XRCID("crop_list_unknown"), CropPanel::ListSelectionChanged )
    EVT_LIST_ITEM_DESELECTED( XRCID("crop_list_unknown"), CropPanel::ListSelectionChanged )
    EVT_TEXT_ENTER (XRCID("crop_left_text") ,CropPanel::OnSetLeft )
    EVT_TEXT_ENTER (XRCID("crop_right_text") ,CropPanel::OnSetRight )
    EVT_TEXT_ENTER (XRCID("crop_top_text") ,CropPanel::OnSetTop )
    EVT_TEXT_ENTER (XRCID("crop_bottom_text") ,CropPanel::OnSetBottom )
    EVT_BUTTON ( XRCID("crop_reset_button") , CropPanel::OnResetButton )
    EVT_CHECKBOX( XRCID("crop_autocenter_cb") , CropPanel::OnAutoCenter)
END_EVENT_TABLE()

// Define a constructor for my canvas
CropPanel::CropPanel()
  : m_pano(0), m_circular(false)
{

}

bool CropPanel::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                         long style, const wxString& name)
{
    DEBUG_TRACE("");
    if (! wxPanel::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    wxXmlResource::Get()->LoadPanel(this, wxT("crop_panel"));
    wxPanel * panel = XRCCTRL(*this, "crop_panel", wxPanel);

    wxBoxSizer *topsizer = new wxBoxSizer( wxVERTICAL );
    topsizer->Add(panel, 1, wxEXPAND, 0);
    SetSizer(topsizer);

#ifdef DEBUG
    SetBackgroundColour(wxTheColourDatabase->Find(wxT("RED")));
    panel->SetBackgroundColour(wxTheColourDatabase->Find(wxT("BLUE")));
#endif

    // get custom sub widgets
    m_imagesList = XRCCTRL(*this, "crop_list_unknown", ImagesListCrop);
    DEBUG_ASSERT(m_imagesList);

    m_Canvas = XRCCTRL(*this, "crop_edit_unknown", CenterCanvas);
    DEBUG_ASSERT(m_imagesList);

    // get all other widgets

    m_left_textctrl = XRCCTRL(*this,"crop_left_text", wxTextCtrl);
    DEBUG_ASSERT(m_left_textctrl);
    m_left_textctrl->PushEventHandler(new TextKillFocusHandler(this));

    m_top_textctrl = XRCCTRL(*this,"crop_top_text", wxTextCtrl);
    DEBUG_ASSERT(m_top_textctrl);
    m_top_textctrl->PushEventHandler(new TextKillFocusHandler(this));

    m_right_textctrl = XRCCTRL(*this,"crop_right_text", wxTextCtrl);
    DEBUG_ASSERT(m_right_textctrl);
    m_right_textctrl->PushEventHandler(new TextKillFocusHandler(this));

    m_bottom_textctrl = XRCCTRL(*this,"crop_bottom_text", wxTextCtrl);
    DEBUG_ASSERT(m_bottom_textctrl);
    m_bottom_textctrl->PushEventHandler(new TextKillFocusHandler(this));

    m_autocenter_cb = XRCCTRL(*this,"crop_autocenter_cb", wxCheckBox);
    DEBUG_ASSERT(m_autocenter_cb);

    DEBUG_TRACE("");
    return true;
}

void CropPanel::Init(Panorama * panorama)
{
    m_pano = panorama;
    m_imagesList->Init(m_pano);
    m_Canvas->Init(this);
    m_Canvas->Show();
    // observe the panorama
    m_pano->addObserver(this);
}


CropPanel::~CropPanel(void)
{
    DEBUG_TRACE("");
    m_left_textctrl->PopEventHandler(true);
    m_right_textctrl->PopEventHandler(true);
    m_top_textctrl->PopEventHandler(true);
    m_bottom_textctrl->PopEventHandler(true);

    m_pano->removeObserver(this);
}

void CropPanel::ListSelectionChanged(wxListEvent& e)
{
    DEBUG_TRACE(e.GetIndex());
    m_selectedImages = m_imagesList->GetSelected();
    DEBUG_DEBUG("selected Images: " << m_selectedImages.size());
    bool hasImage = (m_selectedImages.size() > 0);
    m_left_textctrl->Enable(hasImage);
    m_top_textctrl->Enable(hasImage);
    m_bottom_textctrl->Enable(hasImage);
    m_right_textctrl->Enable(hasImage);
    if (hasImage) {
        // show first image.
        int imgNr = *(m_selectedImages.begin());
        Pano2Display(imgNr);
    } else {
        // TODO: set an empty display
    }
    // else do some stuff...
}

void CropPanel::panoramaImagesChanged (PT::Panorama &pano, const PT::UIntSet & imgNrs)
{
    if (m_selectedImages.size() > 0) {
        int imgNr = *(m_selectedImages.begin());
        if (set_contains(imgNrs, imgNr )) {
            Pano2Display(imgNr);
        }
    }
}

// transfer panorama state to our state
void CropPanel::Pano2Display(int imgNr)
{
    const PanoImage & img = m_pano->getImage(imgNr);

    std::string newImgFile = img.getFilename();
    // check if we need to display a new image
    if (m_currentImageFile != newImgFile) {
//        wxImage wximg;
        ImageCache::EntryPtr imgV = ImageCache::getInstance().getImage(newImgFile);
        m_Canvas->SetImage(imgV);
        m_currentImageFile == newImgFile;
    }
    m_imgOpts = img.getOptions();

    VariableMap vars = m_pano->getImageVariables(imgNr);
    int dx = roundi(map_get(vars,"d").getValue());
    int dy = roundi(map_get(vars,"e").getValue());
    m_center = vigra::Point2D(img.getWidth()/2 + dx, img.getHeight()/2 + dy);
    m_circular= m_pano->getLens(img.getLensNr()).getProjection() == PT::Lens::CIRCULAR_FISHEYE;

    UpdateDisplay();
}

// transfer our state to panorama
void CropPanel::Display2Pano()
{
    // set crop image options.
    vector<ImageOptions> opts;
    for (UIntSet::iterator it = m_selectedImages.begin();
         it != m_selectedImages.end(); ++it)
    {
        const PanoImage & img = m_pano->getImage(*it);
        ImageOptions opt = img.getOptions();
        opt.cropRect = m_imgOpts.cropRect;
        opt.autoCenterCrop = m_imgOpts.autoCenterCrop;
        if (opt.cropRect.isEmpty()) {
            opt.docrop = false;
        } else if ((!m_circular) && opt.docrop &&  (opt.cropRect.left() == 0 && opt.cropRect.top() == 0 
                && opt.cropRect.right() == (int) img.getWidth() 
                && opt.cropRect.bottom() == (int) img.getHeight()) )
        {
            opt.docrop = false;
        } else {
            opt.docrop = true;
        }
        opts.push_back(opt);
    }

    GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImageOptionsCmd(*m_pano, opts, m_selectedImages)
                                           );
}

// redraw display with new information
void CropPanel::UpdateDisplay()
{
    DEBUG_TRACE("")
    m_autocenter_cb->SetValue(m_imgOpts.autoCenterCrop);
    m_left_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.left()));
    m_right_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.right()));
    m_top_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.top()));
    m_bottom_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.bottom()));

    m_Canvas->UpdateDisplay(m_imgOpts.cropRect, m_circular, m_center, m_imgOpts.autoCenterCrop);
}


void CropPanel::OnSetTop(wxCommandEvent & e)
{
    long val;
    if (m_top_textctrl->GetValue().ToLong(&val)) {
        DEBUG_DEBUG(val);
        m_imgOpts.cropRect.setUpperLeft(Point2D(m_imgOpts.cropRect.left(), val));
        if (m_imgOpts.autoCenterCrop) {
            CenterCrop();
            UpdateDisplay();
        }
        Display2Pano();
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void CropPanel::OnSetBottom(wxCommandEvent & e)
{
    long val;
    if (m_bottom_textctrl->GetValue().ToLong(&val)) {
        DEBUG_DEBUG(val);
        m_imgOpts.cropRect.setLowerRight(Point2D(m_imgOpts.cropRect.right(), val));
        if (m_imgOpts.autoCenterCrop) {
            CenterCrop();
            UpdateDisplay();
        }
        Display2Pano();
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void CropPanel::OnSetLeft(wxCommandEvent & e)
{
    long val = 0;
    if (m_left_textctrl->GetValue().ToLong(&val)) {
        DEBUG_DEBUG(val);
        m_imgOpts.cropRect.setUpperLeft(Point2D(val, m_imgOpts.cropRect.top()));
        if (m_imgOpts.autoCenterCrop) {
            CenterCrop();
            UpdateDisplay();
        }
        Display2Pano();
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void CropPanel::OnSetRight(wxCommandEvent & e)
{
    long val = 0;
    if (m_right_textctrl->GetValue().ToLong(&val)) {
        DEBUG_DEBUG(val);
        m_imgOpts.cropRect.setLowerRight(Point2D(val, m_imgOpts.cropRect.bottom()));
        if (m_imgOpts.autoCenterCrop) {
            CenterCrop();
            UpdateDisplay();
        }
        Display2Pano();
    } else {
        wxLogError(_("Please enter a valid number"));
    }
}

void CropPanel::OnResetButton(wxCommandEvent & e)
{
    // suitable defaults.
    m_imgOpts.cropRect.setUpperLeft(Point2D(0,0));
    m_imgOpts.cropRect.setLowerRight(Point2D(0,0));
    m_imgOpts.autoCenterCrop = true;
    m_imgOpts.docrop = false;
    UpdateDisplay();
    Display2Pano();
}

void CropPanel::OnAutoCenter(wxCommandEvent & e)
{
    m_imgOpts.autoCenterCrop = e.IsChecked();
    if (m_imgOpts.autoCenterCrop) {
        CenterCrop();
        UpdateDisplay();
    }
    Display2Pano();
}

// event handlers for CenterCanvas
void CropPanel::CropChanged(vigra::Rect2D & crop, bool dragging)
{
    m_imgOpts.cropRect = crop;
    if (dragging) {
        m_left_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.left()));
        m_right_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.right()));
        m_top_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.top()));
        m_bottom_textctrl->SetValue(wxString::Format(wxT("%d"),m_imgOpts.cropRect.bottom()));
    } else {
        if (m_imgOpts.autoCenterCrop) {
            CenterCrop();
        }
        UpdateDisplay();
        Display2Pano();
    }
}


void CropPanel::CenterCrop()
{
    Diff2D d(m_imgOpts.cropRect.width()/2, m_imgOpts.cropRect.height() / 2);
    m_imgOpts.cropRect.setUpperLeft( m_center - d);
    m_imgOpts.cropRect.setLowerRight( m_center + d);
}



IMPLEMENT_DYNAMIC_CLASS(CropPanel, wxFrame)

CropPanelXmlHandler::CropPanelXmlHandler()
    : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *CropPanelXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, CropPanel)

            cp->Create(m_parentAsWindow,
                       GetID(),
                       GetPosition(), GetSize(),
                       GetStyle(wxT("style")),
                       GetName());

    SetupWindow( cp);

    return cp;
}

bool CropPanelXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("CropPanel"));
}

IMPLEMENT_DYNAMIC_CLASS(CropPanelXmlHandler, wxXmlResourceHandler)


//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CenterCanvas, wxPanel)
    EVT_SIZE   ( CenterCanvas::Resize )
    EVT_MOUSE_EVENTS ( CenterCanvas::OnMouse )
    EVT_PAINT ( CenterCanvas::OnPaint )
END_EVENT_TABLE()

CenterCanvas::CenterCanvas()
    : m_listener(0)
{

}

// Define a constructor for my canvasbool
bool CenterCanvas::Create(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                     long style, const wxString& name)
{
    DEBUG_TRACE("");
    if (! wxPanel::Create(parent, id, pos, size, style, name)) {
        return false;
    }

    m_state = NO_SEL;

    m_cursor_no_sel = new wxCursor(wxCURSOR_CROSS);
    m_cursor_circ_drag = new wxCursor(wxCURSOR_SIZING);
    m_cursor_move_crop = new wxCursor(wxCURSOR_HAND);
    m_cursor_drag_vert = new wxCursor(wxCURSOR_SIZENS);
    m_cursor_drag_horiz = new wxCursor(wxCURSOR_SIZEWE);
    SetCursor(*m_cursor_no_sel);
    return true;
}

void CenterCanvas::Init(CropPanel * listener)
{
    m_listener = listener;
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
        m_listener->CropChanged(m_roi, m_state != CROP_SELECTED);
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

void CenterCanvas::SetImage(ImageCache::EntryPtr s_img)
{
    m_imgCacheImg = s_img;
    img = imageCacheEntry2wxImage(s_img);

    wxSizeEvent e;
    Resize (e);
    // draw new view into offscreen buffer
    DrawView();
    // eventually update the view
    Refresh(false);
}

void CenterCanvas::UpdateDisplay(const vigra::Rect2D & crop, bool circle,
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
                    m_circ_first_point = p;
                    m_roi.setUpperLeft(p);
                    m_roi.setLowerRight(p);
                    ChangeMode(RECT_DRAGGING);
                }
            }
            break;
        case RECT_DRAGGING:
            {
                if (e.LeftUp()) {
                    ChangeMode(CROP_SELECTED);
                }
                vigra::Point2D fp(xpos_i, ypos_i);
                if (m_centered) {
                    // special case for centered crops
                    vigra::Diff2D d = fp - m_center;
                    vigra::Point2D sp(m_center - d);
                    m_roi.setUpperLeft(sp);
                    m_roi.setLowerRight(fp);
                } else {
                    // check on which side we are.
                    int left,right,top,bottom;
                    if ( m_circ_first_point.x < fp.x) {
                        right = fp.x;
                        left = m_circ_first_point.x;
                    } else {
                        left = fp.x;
                        right = m_circ_first_point.x;
                    }
                    if ( m_circ_first_point.y < fp.y) {
                        bottom = fp.y;
                        top = m_circ_first_point.y;
                    } else {
                        top = fp.y;
                        bottom = m_circ_first_point.y;
                    }
                    m_roi.setLowerRight(Point2D(right, bottom));
                    m_roi.setUpperLeft(Point2D(left, top));
                    DEBUG_DEBUG("RECT_DRAGGING: ul:" << left << " " << top << " lr:" << right << " " << bottom);
                    //m_roi.setLowerRight(fp);
                }
                UpdateCropCircle();
            }
            break;
        case CIRC_DRAGGING:
            if (e.LeftUp()) {
                ChangeMode(CROP_SELECTED);
            }
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
            } else {
                if (&GetCursor() != m_cursor_move_crop) {
                    SetCursor(*m_cursor_move_crop);
                }
                // no treatment for rectangular crop area yet.
                // check if mouse is on a rectangular crop border
            }
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


IMPLEMENT_DYNAMIC_CLASS(CenterCanvas, wxFrame)

CenterCanvasXmlHandler::CenterCanvasXmlHandler()
    : wxXmlResourceHandler()
{
    AddWindowStyles();
}

wxObject *CenterCanvasXmlHandler::DoCreateResource()
{
    XRC_MAKE_INSTANCE(cp, CenterCanvas)

            cp->Create(m_parentAsWindow,
                       GetID(),
                       GetPosition(), GetSize(),
                       GetStyle(wxT("style")),
                       GetName());

    SetupWindow( cp);

    return cp;
}

bool CenterCanvasXmlHandler::CanHandle(wxXmlNode *node)
{
    return IsOfClass(node, wxT("CenterCanvas"));
}

IMPLEMENT_DYNAMIC_CLASS(CenterCanvasXmlHandler, wxXmlResourceHandler)

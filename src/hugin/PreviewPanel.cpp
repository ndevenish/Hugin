// -*- c-basic-offset: 4 -*-

/** @file PreviewPanel.cpp
 *
 *  @brief implementation of PreviewPanel Class
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



#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/listctrl.h>	// needed on mingw
#include <wx/imaglist.h>
#include <wx/spinctrl.h>

#include "hugin/ImageCache.h"
#include "common/stl_utils.h"
#include "PT/PanoCommand.h"
#include "hugin/PanoToolsInterface.h"

#include "hugin/PreviewPanel.h"
#ifdef min
#undef min
#endif

#ifdef max
#undef max
#endif

using namespace PT;
using namespace std;

BEGIN_EVENT_TABLE(PreviewPanel, wxPanel)
//    EVT_PAINT(CPImageCtrl::OnPaint)
//    EVT_LEFT_DOWN(CPImageCtrl::mousePressEvent)
//    EVT_MOTION(CPImageCtrl::mouseMoveEvent)
//    EVT_LEFT_UP(CPImageCtrl::mouseReleaseEvent)
    EVT_SIZE(PreviewPanel::OnResize)
//    EVT_MOUSE_EVENTS ( PreviewPanel::OnMouse )
    EVT_PAINT ( PreviewPanel::OnDraw )
END_EVENT_TABLE()

PreviewPanel::PreviewPanel(wxWindow *parent, Panorama * pano)
    : wxPanel (parent, -1, wxDefaultPosition,
               wxSize(256,128), wxEXPAND),
    pano(*pano), m_autoPreview(false),m_panoImgSize(1,1),
    parentWindow(parent)
{
    DEBUG_TRACE("");
    pano->addObserver(this);
}

PreviewPanel::~PreviewPanel()
{
    for (vector<wxBitmap *>::iterator it = m_remappedBitmaps.begin();
         it != m_remappedBitmaps.end();
         ++it)
    {
        delete *it;
    }
}

void PreviewPanel::panoramaChanged(Panorama &pano)
{
    bool dirty = false;

    const PanoramaOptions & newOpts = pano.getOptions();

    // check if an important options has been changed
    if (newOpts.HFOV != opts.HFOV) dirty = true;
    if (newOpts.VFOV != opts.VFOV) dirty = true;
    if (newOpts.projectionFormat != opts.projectionFormat) dirty = true;

    opts = newOpts;
    if (m_autoPreview && dirty) {
        for (unsigned int i = 0; i < m_remappedBitmaps.size(); i++) {
            m_dirtyImgs.insert(i);
        }
        updatePreview();
        // resize
    }
}

void PreviewPanel::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    bool dirty = false;

    unsigned int nrImages = pano.getNrOfImages();
    unsigned int nrRemapped = m_remappedBitmaps.size();

    // remove items for nonexisting images
    for (int i=nrRemapped-1; i>=(int)nrImages; i--)
    {
        DEBUG_DEBUG("Deleting remapped wxImage" << i);
        delete m_remappedBitmaps[i];
        m_remappedBitmaps.pop_back();
        dirty = true;
    }
    // update existing items
    if ( nrImages >= nrRemapped ) {
        for(PT::UIntSet::iterator it = changed.begin(); it != changed.end(); ++it){
            if (*it >= nrRemapped) {
                // create new item.
//                wxImage * bmp = new wxImage(sz.GetWidth(), sz.GetHeight());
                m_remappedBitmaps.push_back(new wxBitmap());
                m_dirtyImgs.insert(*it);
            } else {
                // update existing item
                m_dirtyImgs.insert(*it);
            }
        }
    }
    if (m_autoPreview) {
        updatePreview();
    }
}

void PreviewPanel::SetDisplayedImages(const UIntSet & imgs)
{
    m_displayedImages = imgs;
    updatePreview();
    Refresh(false);
}

void PreviewPanel::ForceUpdate()
{
    unsigned int nImages = pano.getNrOfImages();
    for (unsigned int i=0; i < nImages; i++)
    {
        m_dirtyImgs.insert(i);
    }
    updatePreview();
}

void PreviewPanel::updatePreview()
{
    DEBUG_TRACE("");
    bool dirty = false;
    // temporary bitmap for our remapped image
    // calculate the image size from panel widht, height from vfov

    double finalWidth = pano.getOptions().width;
    double finalHeight = pano.getOptions().getHeight();

    m_panoImgSize = GetClientSize();

    double ratioPano = finalWidth / finalHeight;
    double ratioPanel = m_panoImgSize.GetWidth() / m_panoImgSize.GetHeight();
    
    if (ratioPano < ratioPanel) {
        // panel is wider than pano
        m_panoImgSize.SetWidth((int) (m_panoImgSize.GetHeight() * ratioPano));
    } else {
        // panel is taller than pano
        m_panoImgSize.SetHeight((int)(m_panoImgSize.GetWidth() / ratioPano));
    }
    
    wxImage timg(m_panoImgSize.GetWidth(), m_panoImgSize.GetHeight());

    UIntSet::iterator it = m_dirtyImgs.begin();
    while(it != m_dirtyImgs.end()) {
        if (set_contains(m_displayedImages, *it)) {
            if (!PTools::mapImage(timg, pano, *it, pano.getOptions())) {
                DEBUG_ERROR("mapImage for image " << *it << " failed" );
            }
            // FIXME.. we just mask out the black areas and hope that the
            // image doesn't contain some..
            timg.SetMaskColour(0,0,0);
            // convert to a drawable bitmap, lets hope that the mask
            // is transfered
            DEBUG_ASSERT(m_remappedBitmaps.size() > *it);
            *(m_remappedBitmaps[*it]) = timg.ConvertToBitmap();
            dirty = true;
            UIntSet::iterator tit = it;
            ++it;
            m_dirtyImgs.erase(tit);
        } else {
            // do not update image if it is not shown.
            ++it;
        }
    }
    if (dirty) {
        Refresh();
    }
}

void PreviewPanel::OnDraw(wxPaintEvent & event)
{
    wxClientDC dc(this);

    if (!IsShown()){
        return;
    }

    int offsetX = 0;
    int offsetY = 0;
    
    wxSize sz = GetClientSize();
    if (sz.GetWidth() > m_panoImgSize.GetWidth()) {
        offsetX = (sz.GetWidth() - m_panoImgSize.GetWidth()) / 2;
    }
    if (sz.GetHeight() > m_panoImgSize.GetHeight()) {
        offsetY = (sz.GetHeight() - m_panoImgSize.GetHeight()) / 2;
    }
    
    DEBUG_TRACE("redrawing preview Panel");

    dc.SetPen(wxPen("BLACK",1,wxSOLID));
    dc.SetBrush(wxBrush("BLACK",wxSOLID));
    dc.DrawRectangle(offsetX, offsetY, m_panoImgSize.GetWidth(), m_panoImgSize.GetHeight());

    for (vector<wxBitmap *>::iterator it = m_remappedBitmaps.begin();
         it != m_remappedBitmaps.end();
         ++it)
    {
        // draw only images that are scheduled to be drawn
        if (set_contains(m_displayedImages, it-m_remappedBitmaps.begin())) {
            if ((*it)->Ok()) {
                DEBUG_DEBUG("drawing image " << it - m_remappedBitmaps.begin());
                dc.DrawBitmap(*(*it), offsetX, offsetY, true);
            }
        }
    }

    wxCoord w = m_panoImgSize.GetWidth();
    wxCoord h = m_panoImgSize.GetHeight();

    // draw center lines over display
    dc.SetPen(wxPen("WHITE", 1, wxSOLID));
    dc.SetLogicalFunction(wxINVERT);
    dc.DrawLine(offsetX + w/2, offsetY,
                offsetX + w/2, offsetY + h);
    dc.DrawLine(offsetX, offsetY + h/2,
                offsetX + w, offsetY+ h/2);
}

void PreviewPanel::OnResize(wxSizeEvent & e)
{
    DEBUG_TRACE("");
    wxSize sz = GetClientSize();
    if (sz != m_panoImgSize) {
        if (m_autoPreview) {
            ForceUpdate();
        }
    }
}

void PreviewPanel::OnMouse(wxMouseEvent & e)
{
    DEBUG_DEBUG("OnMouse: " << e.m_x << "x" << e.m_y);
}
#if 0

// Yaw by slider -> int -> double  -- roughly
void ImagesPanel::SetYaw ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
        int var = XRCCTRL(*this, "images_slider_yaw", wxSlider) ->GetValue();
        DEBUG_INFO ("yaw = " << var );

        char text[16];
        sprintf( text, "%d ", var );
        XRCCTRL(*this, "images_stext_orientation", wxStaticText) ->SetLabel(text);
        XRCCTRL(*this,"images_text_yaw",wxTextCtrl)->SetValue( doubleToString (
                                              (double) var ).c_str() );

        ChangePano ( "y" , (double) var );

    }
    //DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}
void ImagesPanel::SetPitch ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
      int var = XRCCTRL(*this,"images_slider_pitch",wxSlider) ->GetValue() * -1;
      DEBUG_INFO ("pitch = " << var )

      char text[16];
      sprintf( text, "%d", var );
      XRCCTRL(*this, "images_stext_orientation", wxStaticText) ->SetLabel(text);

      ChangePano ( "p" , (double) var );

      XRCCTRL(*this,"images_text_pitch",wxTextCtrl)->SetValue( doubleToString (
                                              (double) var ).c_str() );
    }
//    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}
void ImagesPanel::SetRoll ( wxCommandEvent & e )
{
    if ( images_list->GetSelected().size() > 0 ) {
      int var = XRCCTRL(*this, "images_slider_roll", wxSlider) ->GetValue();
      DEBUG_INFO ("roll = " << var )

      char text[16];
      sprintf( text, "%d", var );
      XRCCTRL(*this, "images_stext_roll", wxStaticText) ->SetLabel(text);

      ChangePano ( "r" , (double) var );

      XRCCTRL(*this, "images_text_roll", wxTextCtrl)->SetValue( doubleToString (
                                              (double) var ).c_str() );
    }
//    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}


void ImagesPanel::SetYawPitch ( double coord_x, double coord_y ) {
    wxCommandEvent e;
    SetYaw (e);
    SetPitch (e);
}

#endif


#if 0

BEGIN_EVENT_TABLE(ImgPreview, wxScrolledWindow)
    EVT_MOUSE_EVENTS ( ImgPreview::OnMouse )
    EVT_PAINT ( CenterCanvas::OnDraw )
END_EVENT_TABLE()

// Define a constructor for my canvas
ImgPreview::ImgPreview(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama *pano)
  : wxScrolledWindow(parent, -1, pos, size),
    pano(*pano)
{
    DEBUG_TRACE("");
}

ImgPreview::~ImgPreview(void)
{
    DEBUG_TRACE("");
}

// Define the repainting behaviour
void ImgPreview::OnDraw(wxDC & dc)
{
  if ( p_img && p_img->Ok() )
  {
    wxMemoryDC memDC;
    memDC.SelectObject(* p_img);

    // Transparent blitting if there's a mask in the bitmap
    dc.Blit(0, 0, p_img->GetWidth(), p_img->GetHeight(), & memDC,
      0, 0, wxCOPY, TRUE);

    memDC.SelectObject(wxNullBitmap);
  }
}

void ImgPreview::ChangePreview ( wxImage & s_img )
{
          // right image preview
          wxImage r_img;

          int new_width;
          int new_height;
          if ( ((float)s_img.GetWidth() / (float)s_img.GetHeight())
                > 2.0 ) {
            new_width =  256;
            new_height = (int)((float)s_img.GetHeight()/
                                        (float)s_img.GetWidth()*256.0);
          } else {
            new_width = (int)((float)s_img.GetWidth()/
                                        (float)s_img.GetHeight()*128.0);
            new_height = 128;
          }

          r_img = s_img.Scale( new_width, new_height );
          delete p_img;
          p_img = new wxBitmap( r_img.ConvertToBitmap() );
          Refresh();
}


void ImgPreview::OnMouse ( wxMouseEvent & e )
{
    double coord_x = (double)e.m_x / 256.0 *  360.0 - 180.0;
    double coord_y = (double)e.m_y / 128.0 * -180.0 +  90.0;

    frame->SetStatusText(wxString::Format("%d°,%d°",
              (int)coord_x,
              (int)coord_y ), 1);

    // mouse yaw and pitch -> sliders
    if ( e.m_shiftDown || (e.m_shiftDown && e.m_controlDown)
           || (!e.m_shiftDown && !e.m_controlDown))
      XRCCTRL(*images_panel,"images_slider_pitch",wxSlider)
                                       ->SetValue((int) -coord_y);
    if ( e.m_controlDown || (e.m_shiftDown && e.m_controlDown)
           || (!e.m_shiftDown && !e.m_controlDown))
      XRCCTRL(*images_panel,"images_slider_yaw", wxSlider)
                                       ->SetValue((int) coord_x);

    wxCommandEvent event;
    // mouse yaw and pitch -> pano
    if ( e.m_leftDown ) {
      if ( (!e.m_shiftDown && !e.m_controlDown) || (e.m_shiftDown && e.m_controlDown) ) {
        images_panel->SetYawPitch( coord_x, coord_y );
      } else {
        if ( e.m_shiftDown ) {
          images_panel->SetPitch (event);
        }
        if ( e.m_controlDown ) {
          images_panel->SetYaw (event);
        }
      }
    }
    // pano yaw and pitch -> sliders
    if ((images_panel->imgNr[0] >= 1)) {
      if (e.Leaving() || (e.m_controlDown
           && !(e.m_shiftDown && e.m_controlDown)))
        XRCCTRL(*images_panel,"images_slider_pitch",wxSlider) ->SetValue( -1 *
             (int) pano.getVariable(images_panel->imgNr[1]) .pitch .getValue());
      if (e.Leaving() || (e.m_shiftDown
            && !(e.m_shiftDown && e.m_controlDown)))
        XRCCTRL(*images_panel,"images_slider_yaw",wxSlider) ->SetValue(
             (int) pano.getVariable(images_panel->imgNr[1]) .yaw .getValue());
    }

//    DEBUG_INFO ( "Mouse " << e.Entering() << e.Leaving());
}

#endif

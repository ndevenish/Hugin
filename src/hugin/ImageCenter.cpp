// -*- c-basic-offset: 4 -*-

/** @file ImagesPanel.cpp
 *
 *  @brief implementation of ImagesPanel Class
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

#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/ImageCenter.h"

using namespace PT;
using namespace utils;
using namespace std;

BEGIN_EVENT_TABLE(ImgCenter, wxDialog)
    EVT_SIZE   ( ImgCenter::FitParent )
    EVT_BUTTON ( XRCID("image_center_b_apply"), ImgCenter::OnApply )
    EVT_BUTTON ( XRCID("image_center_b_close"), ImgCenter::OnClose )
END_EVENT_TABLE()

// Define a constructor for my canvas
ImgCenter::ImgCenter(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama& pano, const UIntSet& i)
  : wxDialog(parent, -1, _("Center dialog"), pos, size, wxRESIZE_BORDER),
    pano(pano)
{
    DEBUG_TRACE("");
    wxTheXmlResource->LoadPanel(this, wxT("image_center_dialog"));

    c_canvas = new CenterCanvas ((wxWindow*)this,
               XRCCTRL(*this, "image_center_view", wxPanel)->GetPosition(),
               XRCCTRL(*this, "image_center_view", wxPanel)->GetSize(), pano,i);

    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    SetSize(config->Read(wxT("CenterDialogSize_x"),200l),
            config->Read(wxT("CenterDialogSize_y"),200l));

    c_canvas->Show();
    DEBUG_TRACE("");
}

ImgCenter::~ImgCenter(void)
{
    DEBUG_TRACE("");
}

void ImgCenter::FitParent( wxSizeEvent & e )
{
    wxSize new_size = GetClientSize();
    XRCCTRL(*this, "image_center_dialog", wxPanel)->SetSize ( new_size );
    c_canvas->SetSize (XRCCTRL(*this, "image_center_view", wxPanel)->GetSize());
    DEBUG_INFO( "" << new_size.GetWidth() <<"x"<< new_size.GetHeight()  );
}

void ImgCenter::ChangeView ( wxImage & s_img )
{
    DEBUG_TRACE("");

    c_canvas -> ChangeView (s_img);
    Refresh();

    DEBUG_TRACE("");
}


void ImgCenter::OnApply ( wxCommandEvent & e )
{
    DEBUG_TRACE("");
    c_canvas->ChangePano();
    DEBUG_TRACE("");
}

void ImgCenter::OnClose ( wxCommandEvent & e )
{
    DEBUG_TRACE("");
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    config->Write(wxT("CenterDialogSize_x"),wxString::Format(wxT("%d"),GetRect().width)),
    config->Write(wxT("CenterDialogSize_y"),wxString::Format(wxT("%d"),GetRect().height));
    DEBUG_INFO( "saved last size" )

    Destroy();
    DEBUG_TRACE("");
}


//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(CenterCanvas, wxPanel)
    EVT_SIZE   ( CenterCanvas::Resize )
    EVT_MOUSE_EVENTS ( CenterCanvas::OnMouse )
    EVT_PAINT ( CenterCanvas::OnPaint )
END_EVENT_TABLE()

// Define a constructor for my canvas
CenterCanvas::CenterCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama& pano, const UIntSet & imgs)
  : wxPanel(parent, -1, pos, size, wxALL|wxRESIZE_BORDER | wxMINIMIZE_BOX | wxMAXIMIZE_BOX),
    pano(pano), imgNr(imgs)
{
    DEBUG_TRACE("");

    c_img = wxBitmap(0,0);

    first_x = -1;
    first_y = -1;
    first_is_set = false;
    second_is_set = false;

    Show();
    DEBUG_TRACE("");
}

CenterCanvas::~CenterCanvas(void)
{
    DEBUG_TRACE("");
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

          if ( ((float)img.GetWidth() / (float)img.GetHeight())
                > (float)x/(float)y ) {
            new_width =  x;
            new_height = (int)((float)img.GetHeight()/
                                        (float)img.GetWidth()*(float)x);
            zoom = (float)img.GetWidth()/(float)x;
          } else {
            new_width = (int)((float)img.GetWidth()/
                                        (float)img.GetHeight()*(float)y);
            new_height = y;
            zoom = (float)img.GetHeight()/(float)y;
          }

          c_img = wxBitmap(img.Scale (new_width, new_height));
      }

      // why???
//      dirty_img = c_img.ConvertToImage().ConvertToBitmap();
      DEBUG_TRACE ("")

      // now show the position of current PT shift (d,e)
      wxMemoryDC memDC;
      memDC.SelectObject (c_img);
      memDC.BeginDrawing ();
      // draw all areas without fillings
      wxBrush brush = memDC.GetBrush ();
      brush.SetStyle (wxTRANSPARENT);
      memDC.SetBrush (brush);
      memDC.SetLogicalFunction (wxINVERT);
      // draw text without background
      memDC.SetBackgroundMode (wxTRANSPARENT);

      DEBUG_TRACE ("")
      // We have to work on several images.
      for(UIntSet::iterator it = imgNr.begin(); it != imgNr.end(); ++it) {
        unsigned int imageNr = *it;
#if 0
      for(unsigned int i = 1; i <= imgNr[0]; i++) {
        unsigned int imageNr = imgNr[i];
#endif
        DEBUG_INFO ( "imageNr = "<< imageNr);
        const VariableMap & new_var = pano.getImageVariables(imageNr);
        pt_d = const_map_get(new_var,"d").getValue();
        pt_e = const_map_get(new_var,"e").getValue();
        DEBUG_INFO( "image("<< imageNr <<") with d/e: "<< pt_d <<"/"<< pt_e );
        // paint a cross / text with values
        int c = 8; // size of midpoint cross
        int mid_x = (int)((pt_d + (float)img.GetWidth()/2.0) / zoom);
        int mid_y = (int)((pt_e + (float)img.GetHeight()/2.0) / zoom);
        {
          memDC.DrawLine( mid_x + c, mid_y + c,
                          mid_x - c, mid_y - c);
          memDC.DrawLine( mid_x - c, mid_y + c,
                          mid_x + c, mid_y - c);
          memDC.DrawText( wxString::Format(wxT("%d,%d"),(int)pt_d,(int)pt_e),
                          mid_x+10,mid_y-10);
        }
      }
      DEBUG_TRACE ("")
      memDC.SelectObject(wxNullBitmap);
      memDC.EndDrawing ();

//      Refresh();
    }
    DEBUG_TRACE ("end")
}

// Define the repainting behaviour
void CenterCanvas::OnPaint(wxPaintEvent & dc)
{
    if ( img.Ok() )
    {
        wxPaintDC paintDC( this );

        wxMemoryDC memDC;
        memDC.SelectObject(c_img);

//      DEBUG_INFO( "Width "<< img.GetWidth() <<" Height "<<img.GetHeight());
        paintDC.Blit(0, 0, c_img.GetWidth(), c_img.GetHeight(), & memDC,
                     0, 0, wxCOPY, TRUE);
        memDC.SelectObject(wxNullBitmap);
    }
}

void CenterCanvas::ChangeView ( wxImage & s_img )
{
    DEBUG_TRACE("");

    img = s_img;
    wxSizeEvent e;
    Resize (e);

    DEBUG_TRACE("");
}

//#define MAX(a,b) ( ( a > b ) ? a : b)
//#define MIN(a,b) ( ( a < b ) ? a : b)

void CenterCanvas::OnMouse ( wxMouseEvent & e )
{
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
}

void CenterCanvas::ChangePano ( void )
{
    DEBUG_TRACE("");
    if (imgNr.size() > 0) {
        VariableMapVector vars(imgNr.size());
        int i=0;
        for(UIntSet::iterator it = imgNr.begin(); it != imgNr.end(); ++it) {
            DEBUG_INFO( "setting shift for image "<< *it );
            vars[i] = pano.getImageVariables(*it);
            map_get(vars[i],"d").setValue(pt_d);
            map_get(vars[i],"e").setValue(pt_e);
            i++;
        }

        // go into the history
        GlobalCmdHist::getInstance().addCommand(
            new PT::UpdateImagesVariablesCmd( pano, imgNr, vars )
            );
    }
    DEBUG_INFO( "shift d/e "<< pt_d <<"/"<< pt_e );
}


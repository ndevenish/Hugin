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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/xrc/xmlres.h>          // XRC XML resouces

#include "PT/PanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/ImageCenter.h"
#include "PT/Panorama.h"

#include "common/utils.h"
using namespace PT;
using namespace utils;

BEGIN_EVENT_TABLE(ImgCenter, wxDialog)
    EVT_SIZE   ( ImgCenter::FitParent )
    EVT_BUTTON ( XRCID("image_center_b_apply"), ImgCenter::OnApply )
    EVT_BUTTON ( XRCID("image_center_b_close"), ImgCenter::OnClose )
END_EVENT_TABLE()

// Define a constructor for my canvas
ImgCenter::ImgCenter(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama& pano, unsigned int* i )//UIntSet& i)
  : wxDialog(parent, -1, _("Center dialog"), pos, size, wxRESIZE_BORDER),
    pano(pano)
{
    DEBUG_TRACE("");
    wxTheXmlResource->LoadPanel(this, wxT("image_center_dialog"));

    c_canvas = new CenterCanvas ((wxWindow*)this, 
               XRCCTRL(*this, "image_center_view", wxPanel)->GetPosition(),
               XRCCTRL(*this, "image_center_view", wxPanel)->GetSize(), pano,i);

    c_canvas->Show();
    DEBUG_TRACE("");
}

ImgCenter::~ImgCenter(void)
{
    DEBUG_TRACE("");
}

void ImgCenter::FitParent( wxSizeEvent & e )
{
    wxSize new_size = GetSize();
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
CenterCanvas::CenterCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama& pano, unsigned int* i )
  : wxPanel(parent, -1, pos, size, wxALL|wxRESIZE_BORDER|wxGROW|wxEXPAND),
    pano(pano),
    imgNr(i)
{
    DEBUG_TRACE("");

    c_img = wxBitmap(0,0);

    first_x = -1;
    first_y = -1;

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

          c_img = img.Scale (new_width, new_height)
                                         .ConvertToBitmap();

      dirty_img = c_img.ConvertToImage().ConvertToBitmap();
      DEBUG_TRACE ("")

      // now show the position of current PT shift (d,e)
      wxPaintDC paintDC( this );
      wxMemoryDC memDC;
      memDC.SelectObject (dirty_img);
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
#if 0
      for(UIntSet::iterator it = imgNr.begin(); it != imgNr.end(); ++it) {
        unsigned int imageNr = *it;
#endif
      for(unsigned int i = 1; i <= imgNr[0]; i++) {
        unsigned int imageNr = imgNr[i];
        DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1], imgNr[2], imgNr[3], imgNr[4]) )
        DEBUG_INFO ( "imageNr = "<< imageNr <<" i="<< i )
        ImageVariables new_var = pano.getVariable(imageNr);
        pt_d = new_var. d .getValue();
        pt_e = new_var. e .getValue();
        DEBUG_INFO( "image("<< imageNr <<") with d/e: "<< pt_d <<"/"<< pt_e );
        // paint a cross / text with values
        int c = 5; // size of midpoint cross
        int mid_x = (int)((pt_d + (float)img.GetWidth()) * zoom);
        int mid_y = (int)((pt_e + (float)img.GetHeight()) * zoom);
          memDC.DrawLine( mid_x + c, mid_y + c,
                          mid_x - c, mid_y - c);
          memDC.DrawLine( mid_x - c, mid_y + c,
                          mid_x + c, mid_y - c);
          memDC.DrawText( wxString::Format("%d,%d",(int)pt_d,(int)pt_e),
                          mid_x+10,mid_y-10);
      }
      DEBUG_TRACE ("")
      paintDC.Blit(0, 0, c_img.GetWidth(), c_img.GetHeight(), & memDC,
                   0, 0, wxCOPY, TRUE);
      memDC.SelectObject(wxNullBitmap);
      memDC.EndDrawing ();

      Refresh();
    }
    DEBUG_TRACE ("end")
}

// Define the repainting behaviour
void CenterCanvas::OnPaint(wxDC & dc)
{
    if ( img.Ok() )
    {
      wxPaintDC paintDC( this );

      wxMemoryDC memDC;
      memDC.SelectObject(c_img);

      // Transparent blitting if there's a mask in the bitmap
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

#define MAX(a,b) ( ( a > b ) ? a : b)
#define MIN(a,b) ( ( a < b ) ? a : b)

void CenterCanvas::OnMouse ( wxMouseEvent & e )
{
    // calculate drawing variables
    // vertical and horicontal distance between two mouse clicks
    int c_x = (MAX(e.m_x,first_x)-MIN(e.m_x,first_x))/2;
    int c_y = (MAX(e.m_y,first_y)-MIN(e.m_y,first_y))/2;
    // vertical and horicontal middle point between two mouse clicks
    int mid_x = MIN(e.m_x,first_x) + c_x;
    int mid_y = MIN(e.m_y,first_y) + c_y;
    // half distance between two mouse clicks
    int radius = (int)sqrt(c_x*c_x + c_y*c_y);
    // show some drawing
    if ( first_x >= 0 ) {
      // select an fresh image to draw on (real copy!)

      wxPaintDC paintDC( this );
      wxMemoryDC memDC;
      memDC.SelectObject (dirty_img);
      memDC.BeginDrawing ();

      // recover the old image
#if 1
      wxMemoryDC sourceDC;
      sourceDC.SelectObject (c_img);
      // clear the drawing image
      memDC.Blit(0, 0, c_img.GetWidth(), c_img.GetHeight(), & sourceDC,
                 0, 0, wxCOPY, FALSE);

      // need to cover the whole circle
/*      memDC.Blit( MIN(first_x,second_x),  MIN(first_y,second_y), 
                    MAX(first_x, second_x) - MIN(first_x, second_x),
                    MAX(first_y,second_y) - MIN(first_y,second_y),
                    & sourceDC,
                    MIN(first_x,second_x),  MIN(first_y,second_y), wxCOPY, FALSE);
*///      DEBUG_INFO( "m"<< e.m_x <<"x"<< e.m_y <<" w/h"<<first_x <<"x"<< first_y <<" center"<< second_x <<"x"<< second_y <<" min_pos"<< MIN(first_x,second_x) <<"x"<< MIN(first_y,second_y) <<" size"<< MAX(first_x, second_x) - MIN(first_x, second_x) <<"x"<< MAX(first_y,second_y) - MIN(first_y,second_y) )
      sourceDC.SelectObject(wxNullBitmap);
#else
      wxRect rect ( MIN(first_x,second_x), MIN(first_y,second_y),
                    MAX(first_x,second_x)-MIN(first_x,second_x),
                    MAX(first_y,second_y)-MIN(first_y,second_y));

      Refresh (TRUE,&rect);
#endif
      OnPaint (sourceDC);

      // draw all areas without fillings
      wxBrush brush = memDC.GetBrush ();
      brush.SetStyle (wxTRANSPARENT);
      memDC.SetBrush (brush);
      memDC.SetLogicalFunction (wxINVERT); 
      // draw text without background
      memDC.SetBackgroundMode (wxTRANSPARENT);
      // draw an diagonal line
      memDC.DrawLine( e.m_x, e.m_y, first_x, first_y );
      memDC.DrawCircle ( mid_x, mid_y, radius );
      // draw the midpoint and the coordinates
      int c = 5; // size of midpoint cross
      // calculate PT variables - lens shift
      pt_d = (zoom * (float)mid_x) - (float)img.GetWidth()/2.0;
      pt_e = (zoom * (float)mid_y) - (float)img.GetHeight()/2.0;
      if ( (e.m_x > first_x && e.m_y < first_y)  // 1th and 3th quadrants
           || (e.m_x < first_x && e.m_y > first_y) ) {
        memDC.DrawLine( mid_x + c, mid_y + c,
                        mid_x - c, mid_y - c );
        memDC.DrawText( wxString::Format("%d,%d", (int)pt_d, (int)pt_e),
                        mid_x + 10, mid_y + 10 );
      } else {
        memDC.DrawLine( mid_x - c, mid_y + c,
                        mid_x + c, mid_y - c );
        memDC.DrawText( wxString::Format("%d,%d", (int)pt_d, (int)pt_e),
                        mid_x + 10, mid_y - 15 );
      }
      memDC.EndDrawing ();
      // Transparent blitting if there's a mask in the bitmap
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

      second_x = e.m_x;
      second_y = e.m_y;
    }
    // calculate the results
    if ( e.m_leftDown ) {
      float scale = (float)img.GetHeight()/(float)c_img.GetHeight();
      if ( (e.m_x < c_img.GetWidth()) && (e.m_y < c_img.GetHeight()) ) {
        int pos_x = (int)((float)e.m_x * scale) + 1;
        int pos_y = (int)((float)e.m_y * scale) + 1;
        int x = (int)((float)c_img.GetWidth() * scale);
        int y = (int)((float)c_img.GetHeight() * scale);
        DEBUG_INFO( "m"<< e.m_x <<"x"<< e.m_y <<" pos"<<pos_x <<"x"<< pos_y <<" image"<< x <<"x"<< y <<" inside" )
      } else {
        DEBUG_INFO( "outside" )
      }
      if ( first_x == -1 ) { 
        first_x = e.m_x;
        first_y = e.m_y;
      } else {
        int mid_x_orig = c_img.GetWidth()/2;
        int mid_y_orig = c_img.GetHeight()/2;
        DEBUG_INFO( "horiz. shift (d) = "<< mid_x - mid_x_orig <<"vert. shift (e) = "<< mid_y - mid_y_orig )
        first_x = -1;
        first_y = -1;
      }
    }
//    DEBUG_INFO ( "Mouse " << e.m_x <<"x"<< e.m_y);
}

void CenterCanvas::ChangePano ( void )
{
    DEBUG_TRACE("");
    if ( imgNr[0] > 0 ) { // dont work on an empty image
      // we ask for each images Lens
      Lens new_Lens;
      UIntSet lensesNr;
      LensVector lenses;
      for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        lensesNr.insert(imgNr[i]);
        new_Lens = pano.getLens(imgNr[i]);
        // set values
        new_Lens.d = pt_d;
        new_Lens.e = pt_e;

        lenses.insert (lenses.begin(), new_Lens);
        DEBUG_INFO( "shift for image "<< imgNr[i] );
      }

      // go into the history
      GlobalCmdHist::getInstance().addCommand(
          new PT::ChangeLensesCmd( pano, lensesNr, lenses )
          );
    }
    DEBUG_INFO( "shift d/e "<< pt_d <<"/"<< pt_e );
}


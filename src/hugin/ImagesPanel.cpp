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
//#include <wx/image.h>               // wxImage
//#include <wx/imagpng.h>             // for about html
#include <wx/listctrl.h>	// needed on mingw
#include <wx/imaglist.h>

#include "PT/PanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/List.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "PT/Panorama.h"

using namespace PT;

// Image Icons
wxImageList * img_icons;
// Image Icons biger
wxImageList * img_bicons;

// image preview
wxBitmap * p_img = (wxBitmap *) NULL;

ImgPreview * canvas;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImagesPanel, ImagesPanel)
    EVT_LEFT_DOWN ( ImagesPanel::ChangePreview )
END_EVENT_TABLE()


// Define a constructor for the Images Panel
ImagesPanel::ImagesPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    : pano(*pano)
{
    pano->addObserver(this);

    images_list = new List (parent, pano, images_layout);
//    CPEditorPanel * cpe = new CPEditorPanel(parent,pano);
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_list_unknown"),
               images_list );
//               cpe );

    // Image Preview

    img_icons = new wxImageList(24,24);
    img_bicons = new wxImageList(64,64);
    images_list->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );//_NORMAL );

    p_img = new wxBitmap( 0, 0 );
    wxPanel * img_p = XRCCTRL(*parent, "img_preview_unknown", wxPanel);
    wxPaintDC dc (img_p);

    canvas = new ImgPreview(img_p, wxPoint(0, 0), wxSize(128, 128));
    DEBUG_TRACE("");;

}


ImagesPanel::~ImagesPanel(void)
{
    pano.removeObserver(this);
//    delete p_img; // Dont know how to check for existing
    p_img = (wxBitmap *) NULL;
    delete images_list;
    DEBUG_TRACE("");
}


//void ImagesPanel::panoramaChanged (PT::Panorama &pano)
void ImagesPanel::panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr)
{
    img_icons->RemoveAll();
    img_bicons->RemoveAll();
//    DEBUG_INFO( "after delete are " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")
    // start the loop for every selected filename
    for ( int i = 0 ; i <= (int)pano.getNrOfImages() - 1 ; i++ ) {
        wxFileName fn = (wxString)pano.getImage(i).getFilename().c_str();

        wxImage * img = ImageCache::getInstance().getImage(
            pano.getImage(i).getFilename());

        // preview selected images
        wxImage * s_img = ImageCache::getInstance().getImageSmall(
            pano.getImage(i).getFilename());

        // right image preview
        wxImage r_img;
        // left list control big icon
        wxImage b_img;
        // left list control icon
        wxImage i_img;
        if ( s_img->GetHeight() > s_img->GetWidth() ) {
          r_img = s_img->Scale( (int)((float)s_img->GetWidth()/
                                      (float)s_img->GetHeight()*128.0),
                                128);
          b_img = s_img->Scale( (int)((float)s_img->GetWidth()/
                                      (float)s_img->GetHeight()*64.0),
                                64);
          i_img = s_img->Scale( (int)((float)s_img->GetWidth()/
                                      (float)s_img->GetHeight()*20.0),
                                20);
        } else {
          r_img = s_img->Scale( 128, 
                                (int)((float)s_img->GetHeight()/
                                      (float)s_img->GetWidth()*128.0));
          b_img = r_img.Scale( 64,
                                (int)((float)r_img.GetHeight()/
                                      (float)r_img.GetWidth()*64.0));
          i_img = r_img.Scale( 20,
                                (int)((float)r_img.GetHeight()/
                                      (float)r_img.GetWidth()*20.0));
        }
        delete p_img;
        p_img = new wxBitmap( r_img.ConvertToBitmap() );
        canvas->Refresh();

        img_icons->Add( i_img.ConvertToBitmap() );
        img_bicons->Add( b_img.ConvertToBitmap() );
//        DEBUG_INFO( "now " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")

    }

    if ( pano.getNrOfImages() == 0 ) {
        delete p_img;
        p_img = new wxBitmap(0,0);
    }
    canvas->Refresh();
    DEBUG_TRACE("");
}

void ImagesPanel::ChangePreview ( wxListEvent & e )
{
    long item (1);
//    wxPoint pos = e.GetPosition();
//    long item = HitTest( e.m_x ,e.m_y );
//    DEBUG_INFO ( "hier:" << wxString::Format(" %d is item %ld", e.GetPosition(), item) );
    DEBUG_INFO ( "hier: is item %ld" << wxString::Format("%ld", item) );
}


//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImgPreview, wxScrolledWindow)
    //EVT_PAINT(ImgPreview::OnPaint)
    EVT_MOUSE_EVENTS ( ImgPreview::ChangePreview )
END_EVENT_TABLE()

// Define a constructor for my canvas
ImgPreview::ImgPreview(wxWindow *parent, const wxPoint& pos, const wxSize& size):
 wxScrolledWindow(parent, -1, pos, size)
{
    DEBUG_TRACE("");
}

ImgPreview::~ImgPreview(void)
{
    p_img = (wxBitmap *) NULL;
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

    //img_icons->Draw( 0, dc, 0, 0, wxIMAGELIST_DRAW_NORMAL, FALSE);

    memDC.SelectObject(wxNullBitmap);
  }
}

void ImgPreview::ChangePreview ( wxMouseEvent & e )
{
    //wxPoint pos = e.GetPosition();
//    long item = HitTest( e.m_x , e.m_y );
//    frame->SetStatusText ( wxString::Format("%d,%d",e.m_x , e.m_y ), 1 );
}


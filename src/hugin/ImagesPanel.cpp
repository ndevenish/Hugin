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

#include "PT/PanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
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

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImagesPanel, ImagesPanel)
//    EVT_PAINT(ImagesPanel::OnDraw)
    EVT_LIST_ITEM_SELECTED ( XRCID("images_list"), ImagesPanel::ChangePreview )
    EVT_MOUSE_EVENTS ( ImagesPanel::ChangePreview )
    EVT_MOTION ( ImagesPanel::ChangePreview )
    EVT_LEFT_DOWN ( ImagesPanel::ChangePreview )
END_EVENT_TABLE()


// Define a constructor for the Images Panel
ImagesPanel::ImagesPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano) //, wxStatusBar* s_bar
    //  : wxPanel(parent, -1 , pos, size)
    : pano(*pano)
{
    DEBUG_TRACE("");
//    wxPanel * panel = new wxPanel(images_list, -1);
//    wxWindow * window = new wxWindow(parent, -1, pos, size);
    wxBoxSizer * box = new wxBoxSizer( wxVERTICAL );
    DEBUG_TRACE("");
    images_list = XRCCTRL(*parent, "images_list", wxListCtrl);

    DEBUG_TRACE("");
    box->Add( images_list, 0, wxGROW|wxALIGN_CENTER_VERTICAL|wxALL, 5 );
    DEBUG_TRACE("");
    box->Fit( parent );
    DEBUG_TRACE("");
    box->SetSizeHints( parent );
    

    DEBUG_TRACE("");
    images_list->InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25 );
    images_list->InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 255 );
    images_list->InsertColumn( 2, _("width"), wxLIST_FORMAT_RIGHT, 60 );
    images_list->InsertColumn( 3, _("height"), wxLIST_FORMAT_RIGHT, 60 );
    images_list->InsertColumn( 4, _("No."), wxLIST_FORMAT_RIGHT, 30 );

    DEBUG_TRACE("");
    img_icons = new wxImageList(24,24);
    img_bicons = new wxImageList(64,64);
    DEBUG_TRACE("");
    images_list->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );//_NORMAL );

    // Image Preview
    DEBUG_TRACE("");
    p_img = new wxBitmap( 0, 0 );
    wxPanel * img_p = XRCCTRL(*parent, "img_preview_unknown", wxPanel);
    wxPaintDC dc (img_p);
    DEBUG_TRACE("");

    canvas = new ImgPreview(img_p, wxPoint(0, 0), wxSize(128, 128));
    DEBUG_TRACE("");;

    pano->addObserver(this);
}


ImagesPanel::~ImagesPanel(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
//    delete p_img; // Dont know how to check for existing
    p_img = (wxBitmap *) NULL;
    delete images_list;
    DEBUG_TRACE("");
}


void ImagesPanel::panoramaChanged (PT::Panorama &pano)
{
    DEBUG_TRACE("");
    images_list->DeleteAllItems() ; // very radical
    img_icons->RemoveAll();
    img_bicons->RemoveAll();
    DEBUG_INFO( "after delete are " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")
    // start the loop for every selected filename
    for ( int i = 0 ; i <= (int)pano.getNrOfImages() - 1 ; i++ ) {
        wxFileName fn = (wxString)pano.getImage(i).getFilename().c_str();
        wxImage * image = ImageCache::getInstance().getImage(
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

        int ind = img_icons->Add( i_img.ConvertToBitmap() );
        img_bicons->Add( b_img.ConvertToBitmap() );
        DEBUG_INFO( "now " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")

        // fill in the table
        char Nr[8] ;
        sprintf(Nr ,"%d", i + 1);
        images_list->InsertItem ( i, Nr, ind );
        DEBUG_INFO( " icon at Item:" << wxString::Format("%d", ind) << "/" << wxString::Format("%d", pano.getNrOfImages()) )
        images_list->SetItem ( i, 1, fn.GetFullName() );
        sprintf(Nr, "%d", image->GetHeight() );
        images_list->SetItem ( i, 2, Nr );
        sprintf(Nr, "%d", image->GetWidth() );
        images_list->SetItem ( i, 3, Nr );
        sprintf(Nr, "%d", image->GetImageCount ( fn.GetFullPath() ) );
        images_list->SetItem ( i, 4, Nr );
        images_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
        images_list->SetColumnWidth(1, wxLIST_AUTOSIZE);
        images_list->SetColumnWidth(2, wxLIST_AUTOSIZE);
        images_list->SetColumnWidth(3, wxLIST_AUTOSIZE);
        images_list->SetColumnWidth(4, wxLIST_AUTOSIZE);
//        lst->SetItemImage( pano.getNrOfImages(), pano.getNrOfImages(), pano.getNrOfImages() );

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
    DEBUG_TRACE ("")
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
    DEBUG_TRACE("");
    p_img = (wxBitmap *) NULL;
    DEBUG_TRACE("");
}

// Define the repainting behaviour
void ImgPreview::OnDraw(wxDC & dc)
{
  DEBUG_TRACE("");
  if ( p_img && p_img->Ok() )
  {
    wxMemoryDC memDC;
    memDC.SelectObject(* p_img);

    // Transparent blitting if there's a mask in the bitmap
    dc.Blit(0, 0, p_img->GetWidth(), p_img->GetHeight(), & memDC,
      0, 0, wxCOPY, TRUE);

    //img_icons->Draw( 0, dc, 0, 0, wxIMAGELIST_DRAW_NORMAL, FALSE);

    DEBUG_TRACE("");
    memDC.SelectObject(wxNullBitmap);
  }
}

void ImgPreview::ChangePreview ( wxMouseEvent & e )
{
//    DEBUG_TRACE ("")
//    wxPoint pos = e.GetPosition();
//    long item = HitTest( e.m_x ,e.m_y /*wxLIST_HITTEST_ONITEM*/ );
//    DEBUG_INFO ( "hier: is  " << wxString::Format("%d,%d",e.m_x , e.m_y ) );
//    wxWindow * frame ( wxTheApp->GetTopWindow() );
    frame->SetStatusText ( wxString::Format("%d,%d",e.m_x , e.m_y ), 1 );
}


// -*- c-basic-offset: 4 -*-

/** @file List.cpp
 *
 *  @brief implementation of List Class
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

#include "common/utils.h"
//#include "PT/PanoCommand.h"
//#include "PT/PanoramaMemento.h"
#include "hugin/config.h"
//#include "hugin/CommandHistory.h"
#include "hugin/ImageCache.h"
#include "hugin/List.h"
#include "hugin/LensPanel.h"
#include "hugin/ImagesPanel.h"
//#include "hugin/MainFrame.h"
#include "hugin/Events.h"
#include "hugin/huginApp.h"
//#include "PT/Panorama.h"

using namespace PT;
using namespace utils;

// image preview
extern wxBitmap * p_img;

// ImagesPanel
extern ImagesPanel* images_panel;
// Image Icons
extern wxImageList * img_icons;
// Image Icons biger
extern wxImageList * img_bicons;

long int prevItem (-1);

// LensPanel
extern    LensPanel* lens_panel;
// LensPanel images list
extern    List* images_list2;
// Image Preview
extern ImgPreview *canvas;


//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(List, wxListCtrl)
//    EVT_PAINT(ImagesPanel::OnDraw)
//    EVT_LIST_ITEM_SELECTED ( XRCID("images_list"), List::Change )
//    EVT_MOUSE_EVENTS ( List::Change )
    EVT_MOTION ( List::Change )
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"), List::selectItemVeto)
    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), List::selectItemVeto)
    EVT_LIST_ITEM_SELECTED( XRCID("images_list_unknown"), List::itemSelected )
    EVT_LIST_ITEM_SELECTED( XRCID("images_list2_unknown"), List::itemSelected )
//    EVT_LEFT_DOWN ( List::Change )
END_EVENT_TABLE()

// Define a constructor for the Images Panel
List::List( wxWindow* parent, Panorama* pano, int layout)
    : wxListCtrl(parent, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxSUNKEN_BORDER),   //|wxLC_HRULES),
      pano(*pano), list_layout (layout)
{
    DEBUG_TRACE("");

    if ( list_layout == images_layout ) {
      InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25 );
      InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 255 );
      InsertColumn( 2, _("width"), wxLIST_FORMAT_RIGHT, 60 );
      InsertColumn( 3, _("height"), wxLIST_FORMAT_RIGHT, 60 );
      InsertColumn( 4, _("No."), wxLIST_FORMAT_RIGHT, 30 );
      InsertColumn( 5, _("yaw (y)"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 6, _("pitch (p)"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 7, _("roll (r)"), wxLIST_FORMAT_RIGHT, 40 );
      DEBUG_INFO( " images_layout" )
    } else {
//      SetSingleStyle(wxLC_SINGLE_SEL, true);
      InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25 );
      InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 180 );
      InsertColumn( 2, _("Lens type"), wxLIST_FORMAT_LEFT, 100 );
      InsertColumn( 3, _("Focal length"), wxLIST_FORMAT_RIGHT, 80 );
      InsertColumn( 4, _("a"), wxLIST_FORMAT_RIGHT, 30 );
      InsertColumn( 5, _("b"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 6, _("c"), wxLIST_FORMAT_RIGHT, 50 );
      InsertColumn( 7, _("d"), wxLIST_FORMAT_RIGHT, 60 );
      InsertColumn( 8, _("e"), wxLIST_FORMAT_RIGHT, 60 );
      DEBUG_INFO( " else _layout" )
    }

    // intermediate event station
    PushEventHandler( new MyEvtHandler((size_t) 1) );

    pano->addObserver(this);
    DEBUG_TRACE("");

    selectItems = TRUE;
    for ( int i = 0 ; i < 512 ; i++ )
      imgNr[i] = 0;

    DEBUG_TRACE("end");
}


List::~List(void)
{
    DEBUG_TRACE("");
    pano.removeObserver(this);
//    delete p_img; // Dont know how to check for existing
    p_img = (wxBitmap *) NULL;
    DEBUG_INFO( wxString::Format("modus %d", list_layout) )
}


//void List::panoramaChanged (PT::Panorama &pano) {}

void List::panoramaImagesChanged(Panorama &pano, const UIntSet &changed)
{
    DEBUG_TRACE("");

    DeleteAllItems() ;
//    DEBUG_INFO( wxString::Format("modus %d", list_layout) )

    // start the loop for every selected filename
    DEBUG_INFO ("nr = " << pano.getNrOfImages() )
    for ( int i = 0 ; i <= (int)pano.getNrOfImages() - 1 ; i++ ) {
        wxFileName fn = (wxString)pano.getImage(i).getFilename().c_str();
        wxImage * image = ImageCache::getInstance().getImage(
            pano.getImage(i).getFilename());

        // fill in the table
        char Nr[128] ;
        std::string number;
        sprintf(Nr ,"%d", i);
        InsertItem ( i, Nr, i );
        EnsureVisible(i);
        DEBUG_INFO( " icon at Item: " << i << "/" <<  pano.getNrOfImages() )
        SetItem ( i, 1, fn.GetFullName() );
        if ( list_layout == images_layout ) {
          sprintf(Nr, "%d", image->GetHeight() );
          SetItem ( i, 2, Nr );
          sprintf(Nr, "%d", image->GetWidth() );
          SetItem ( i, 3, Nr );
          sprintf(Nr, "%d", image->GetImageCount ( fn.GetFullPath() ) );
          SetItem ( i, 4, Nr );
          number = doubleToString ( pano.getVariable(i). yaw.getValue() );
          SetItem ( i, 5, number.c_str() );
          number = doubleToString ( pano.getVariable(i). pitch.getValue() );
          SetItem ( i, 6, number.c_str() );
          number = doubleToString ( pano.getVariable(i). roll.getValue() );
          SetItem ( i, 7, number.c_str() );
//        DEBUG_INFO( " images_layout" )
          for ( int j=0; j < GetColumnCount() ; j++ ) {
            SetColumnWidth(j, wxLIST_AUTOSIZE);
            if ( GetColumnWidth(j) < 40 )
              SetColumnWidth(j, 40);
          }
        } else {
          switch ( (int) pano.getLens(pano.getImage(i).getLens()).projectionFormat ) {
            case Lens::RECTILINEAR:   sprintf(Nr, _("Normal (rectlinear)") ); break;
            case Lens::PANORAMIC:          sprintf(Nr, _("Panoramic") ); break;
            case Lens::CIRCULAR_FISHEYE:   sprintf(Nr, _("Circular") ); break;
            case Lens::FULL_FRAME_FISHEYE: sprintf(Nr, _("Full frame") ); break;
            case Lens::EQUIRECTANGULAR_LENS: sprintf(Nr,_("Equirectangular") ); break;
          }
          SetItem ( i, 2, Nr );
          number = doubleToString (
                       pano.getLens (pano.getImage(i).getLens()). focalLength );
          SetItem ( i, 3, number.c_str() );
          number = doubleToString (
                       pano.getLens (pano.getImage(i).getLens()). a );
          SetItem ( i, 4, number.c_str() );
          number = doubleToString (
                       pano.getLens (pano.getImage(i).getLens()). b );
          SetItem ( i, 5, number.c_str() );
          number = doubleToString (
                       pano.getLens (pano.getImage(i).getLens()). c );
          SetItem ( i, 6, number.c_str() );
          number = doubleToString (
                       pano.getLens (pano.getImage(i).getLens()). d );
          SetItem ( i, 7, number.c_str() );
          number = doubleToString (
                       pano.getLens (pano.getImage(i).getLens()). e );
          SetItem ( i, 8, number.c_str() );
          for ( int j=0; j< GetColumnCount() ; j++ ) {
            SetColumnWidth(j, wxLIST_AUTOSIZE);
            if ( GetColumnWidth(j) < 40 )
              SetColumnWidth(j, 40);
          }
//        DEBUG_INFO( " else _layout" )
        }
//        lst->SetItemImage( pano.getNrOfImages(), pano.getNrOfImages(), pano.getNrOfImages() );

    }

    selectItems = FALSE;   // The following causes an event. skip itemSelected()
    for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        SetItemState( imgNr[i], wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
    }
    selectItems = TRUE;

    imgNr[0] = 0;             // reset the counter
    for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
      if ( GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
        imgNr[0] += 1;        // set the counter
        imgNr[imgNr[0]] = Nr; //(unsigned int)Nr;
      }
    }
/*
    if ( pano.getNrOfImages() == 0 ) {
        delete p_img;
        p_img = new wxBitmap(0,0);
    }*/

    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}

bool List::selectItemVeto( wxListEvent & e )
{
    DEBUG_TRACE("");
    if ( selectItems ) {
      itemSelected (e);
    }

    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
    DEBUG_TRACE("end");
    return TRUE;
}

void List::itemSelected ( wxListEvent & e )
{
    DEBUG_TRACE("");
    // prepare an status message
    std::string e_msg ( _("images(") );
    char Nr[8];
    sprintf (Nr, "%d", GetSelectedItemCount() );
    e_msg.append( Nr );
    e_msg.append( "):  " );

    imgNr[0] = 0;             // reset the counter
    for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
      if ( GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
//    DEBUG_TRACE("");
        e_msg += "  " + GetItemText(Nr);
        imgNr[0] += 1;        // set the counter
        imgNr[imgNr[0]] = Nr; //(unsigned int)Nr;
      }
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );

    frame->SetStatusText(e_msg.c_str(), 0);

//    selectedItem = e.GetIndex();
    if ( list_layout == images_layout ) {
//        DEBUG_TRACE ("images_layout")
//        images_panel->SetImages (e);
    }
    // let others recieve the event too
    e.Skip(true);
    DEBUG_TRACE("end");
}

void List::Change ( wxMouseEvent & e )
{
    if ( list_layout == images_layout ) {
//    DEBUG_TRACE ("")
        int flags = wxLIST_HITTEST_ONITEM;
        long item = HitTest( e.GetPosition(),  flags);

        if ( item != -1 && item != prevItem ) {
          // preview selected images
          wxImage * s_img = ImageCache::getInstance().getImageSmall(
              pano.getImage((int)item).getFilename());


          // right image preview
          wxImage r_img;
          if ( s_img->GetHeight() > s_img->GetWidth() ) {
            r_img = s_img->Scale( (int)((float)s_img->GetWidth()/
                                        (float)s_img->GetHeight()*128.0),
                                128);
          } else {
            r_img = s_img->Scale( 128,
                                  (int)((float)s_img->GetHeight()/
                                        (float)s_img->GetWidth()*128.0));
          }
          delete p_img;
          p_img = new wxBitmap( r_img.ConvertToBitmap() );
          canvas->Refresh();
//          frame->SetStatusText(wxString::Format(" %d,%d / %ld",e.m_x,e.m_y, item), 1);
        }

        if (item != -1)
          prevItem = item;
    } else {
    }
}


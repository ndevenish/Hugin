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
      InsertColumn( 8, _("optimize"), wxLIST_FORMAT_RIGHT, 80 );
      DEBUG_INFO( " images_layout" )
    } else {
//      SetSingleStyle(wxLC_SINGLE_SEL, true);
      InsertColumn( 0, _("#"), wxLIST_FORMAT_RIGHT, 25 );
      InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 180 );
      InsertColumn( 2, _("Lens type (f)"), wxLIST_FORMAT_LEFT, 100 );
      InsertColumn( 3, _("hfov (v)"), wxLIST_FORMAT_RIGHT, 80 );
      InsertColumn( 4, _("a"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 5, _("b"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 6, _("c"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 7, _("d"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 8, _("e"), wxLIST_FORMAT_RIGHT, 40 );
      InsertColumn( 9, _("optimize"), wxLIST_FORMAT_RIGHT, 80 );
#if 0
      InsertColumn( 10, _("lens No."), wxLIST_FORMAT_RIGHT, 80 );
#endif
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
    unsigned int imageNr (0);
    selectItems = FALSE;

    DEBUG_INFO( wxString::Format("modus %d", list_layout) )
/*      // publicate the numbers of the selected images
      imgNr[0] = 0;             // reset the counter
      for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
        if ( GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
          imgNr[0] += 1;        // set the counter
          imgNr[imgNr[0]] = Nr; //(unsigned int)Nr;
        }
      }*/
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );

    // first the internal changes without any new or removed item
    if ( (int)pano.getNrOfImages() == GetItemCount() ) {

#if 1
      for(UIntSet::iterator it = changed.begin(); it != changed.end(); ++it){
        imageNr = *it;
#else
      // We assume only the lens list selected images had changed.
      for ( unsigned int i=1 ; i <= imgNr[0] ; i++ ) { //dummy }
        imageNr = imgNr[i];
#endif
        char Nr[128] ;
        sprintf(Nr ,"%d", imageNr);
        SetItem ( imageNr, 0, Nr );
        fillRow ( imageNr );
        DEBUG_INFO( "filling row: " << imageNr )
      }

    // Here we set all items as selected wich may forgot it.
/*      selectItems = FALSE; // The following causes an event.Skip itemSelected()
      for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
        SetItemState( imgNr[i], wxLIST_STATE_SELECTED, wxLIST_STATE_SELECTED );
      }
      */
    } else { // Now we take long way, to setup the table from ground.
      DeleteAllItems() ;

      // start the loop for every selected filename
      DEBUG_INFO ("nr = " << pano.getNrOfImages() )
      for ( imageNr = 0 ; imageNr <= pano.getNrOfImages() - 1 ; imageNr++ ) {
//        lst->SetItemImage( pano.getNrOfImages(), pano.getNrOfImages(), pano.getNrOfImages() );
        char Nr[128] ;
        sprintf(Nr ,"%d", imageNr);
        InsertItem ( imageNr, Nr, imageNr ); // create the new row
        fillRow ( imageNr );
      }

    }
    EnsureVisible(imageNr);
/*
    if ( pano.getNrOfImages() == 0 ) {
        delete p_img;
        p_img = new wxBitmap(0,0);
    }*/

    selectItems = TRUE;
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
}

void List::fillRow (unsigned int imageNr)
{
        wxFileName fn = (wxString)pano.getImage(imageNr).getFilename().c_str();
        wxImage * image = ImageCache::getInstance().getImage(
            pano.getImage(imageNr).getFilename());
        #define ITEM_TEXT(streamin)  number.str(""); number << streamin ;
        #define ITEM_OUT number.str().c_str()
        // fill in the table
        DEBUG_INFO( " Item: "<< imageNr <<"/"<<  pano.getNrOfImages() )
        std::stringstream number;
        SetItem ( imageNr, 1, fn.GetFullName() );
        ImageVariables new_var = pano.getVariable(imageNr);
        std::string opt_str = _("x"); // the small char wich show -> optimized
        if ( list_layout == images_layout ) {
          ITEM_TEXT( image->GetHeight() )
          SetItem ( imageNr, 2, ITEM_OUT );
          ITEM_TEXT( image->GetWidth() )
          SetItem ( imageNr, 3, ITEM_OUT );
          ITEM_TEXT( image->GetImageCount ( fn.GetFullPath() ) )
          SetItem ( imageNr, 4, ITEM_OUT );
          ITEM_TEXT( doubleToString( new_var. yaw.getValue() ) )
          SetItem ( imageNr, 5, ITEM_OUT );
          ITEM_TEXT( doubleToString( new_var. pitch.getValue() ) )
          SetItem ( imageNr, 6, ITEM_OUT );
          ITEM_TEXT( doubleToString( new_var. roll.getValue() ) )
          SetItem ( imageNr, 7, ITEM_OUT );
          number.str("");
          if ( new_var. yaw.isLinked() )
            number << doubleToString ((double)new_var. yaw.getLink());
          else if ( optset->at(imageNr). yaw )
            number << opt_str;
          else
            number << "-";
          number << "|";
          if ( new_var. pitch.isLinked() )
            number << doubleToString ((double)new_var. pitch.getLink());
          else if ( optset->at(imageNr). pitch )
            number << opt_str;
          else
            number << "-";
          number << "|";
          if ( new_var. roll.isLinked() )
            number << doubleToString ((double)new_var. roll.getLink());
          else if ( optset->at(imageNr). roll )
            number << opt_str;
          else
            number << "-";
          SetItem ( imageNr, 8, ITEM_OUT );
//        DEBUG_INFO( " images_layout" )
          for ( int j=0; j < GetColumnCount() ; j++ ) {
            SetColumnWidth(j, wxLIST_AUTOSIZE);
            if ( GetColumnWidth(j) < 40 )
              SetColumnWidth(j, 40);
          }
        } else {
//          Lens lens ( pano.getLens( pano.getImage(imageNr).getLens()) );
          switch ( (int) lens.  projectionFormat  ) {
            case Lens::RECTILINEAR:          number << _("Normal (rectlinear)"); break;
            case Lens::PANORAMIC:            number << _("Panoramic"); break;
            case Lens::CIRCULAR_FISHEYE:     number << _("Circular"); break;
            case Lens::FULL_FRAME_FISHEYE:   number << _("Full frame"); break;
            case Lens::EQUIRECTANGULAR_LENS: number << _("Equirectangular"); break;
          }
          SetItem ( imageNr, 2, ITEM_OUT );
          ITEM_TEXT( doubleToString ( new_var. HFOV .getValue() ))//lens. HFOV))
          SetItem ( imageNr, 3, ITEM_OUT );
          ITEM_TEXT( doubleToString ( new_var. a .getValue() )) //lens. a ))
          SetItem ( imageNr, 4, ITEM_OUT );
          ITEM_TEXT( doubleToString ( new_var. b .getValue() ))
          SetItem ( imageNr, 5, ITEM_OUT );
          ITEM_TEXT( doubleToString ( new_var. c .getValue() ))
          SetItem ( imageNr, 6, ITEM_OUT );
          ITEM_TEXT( doubleToString ( new_var. d .getValue() ))
          SetItem ( imageNr, 7, ITEM_OUT );
          ITEM_TEXT( doubleToString ( new_var. e .getValue() ))
          SetItem ( imageNr, 8, ITEM_OUT );
          number.str("");
          if ( new_var. HFOV.isLinked() )
            number << doubleToString ((double)new_var. HFOV.getLink());
          else if ( optset->at(imageNr). HFOV )
            number << opt_str;
          else
            number << "-";
          number << "|";
          if ( new_var. a.isLinked() )
            number << doubleToString ((double)new_var. a.getLink());
          else if ( optset->at(imageNr). a )
            number << opt_str;
          else
            number << "-";
          number << "|";
          if ( new_var. b.isLinked() )
            number << doubleToString ((double)new_var. b.getLink());
          else if ( optset->at(imageNr). b )
            number << opt_str;
          else
            number << "-";
          number << "|";
          if ( new_var. c.isLinked() )
            number << doubleToString ((double)new_var. c.getLink());
          else if ( optset->at(imageNr). c )
            number << opt_str;
          else
            number << "-";
          number << "|";
          if ( new_var. d.isLinked() )
            number << doubleToString ((double)new_var. d.getLink());
          else if ( optset->at(imageNr). d )
            number << opt_str;
          else
            number << "-";
          number << "|";
          if ( new_var. e.isLinked() )
            number << doubleToString ((double)new_var. e.getLink());
          else if ( optset->at(imageNr). e )
            number << opt_str;
          else
            number << "-";
          SetItem ( imageNr, 9, ITEM_OUT );
#if 0
          ITEM_TEXT( pano.getImage(imageNr).getLens() )
          SetItem ( imageNr, 10, ITEM_OUT );
#endif
          for ( int j=0; j< GetColumnCount() ; j++ ) {
            SetColumnWidth(j, wxLIST_AUTOSIZE);
            if ( GetColumnWidth(j) < 40 )
              SetColumnWidth(j, 40);
          }
//        DEBUG_INFO( " else _layout" )
        }
}

bool List::selectItemVeto( wxListEvent & e )
{
    DEBUG_TRACE("");
    if ( selectItems ) { // if items are selectable do something about
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
    std::string e_msg ( _("selected images(") );
    char Nr[8];
    sprintf (Nr, "%d", GetSelectedItemCount() );
    e_msg.append( Nr );
    e_msg.append( "):  " );

    // publicate the selected items
    imgNr[0] = 0;             // reset the counter
    for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
      if ( GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
//    DEBUG_TRACE("");
        imgNr[0] += 1;        // set the counter
        imgNr[imgNr[0]] = Nr; //(unsigned int)Nr;
      }
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );

    for ( int i = imgNr[0] ; i>0 ; --i ) {
        e_msg += "  " + GetItemText(imgNr[i]);
    }
    frame->SetStatusText(e_msg.c_str(), 0);

//    selectedItem = e.GetIndex();
    if ( list_layout == images_layout ) {
//        DEBUG_TRACE ("images_layout")
        images_panel->SetImages (e);
    } else {
        lens_panel->SetImages (e);
    }

    // let others recieve the event too
    //e.Skip(true);
    DEBUG_TRACE("end");
}

// set a new image of the actual item the mouse is over
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

          int new_width;
          int new_height;
          if ( ((float)s_img->GetWidth() / (float)s_img->GetHeight())
                > 2.0 ) {
            new_width =  256;
            new_height = (int)((float)s_img->GetHeight()/
                                        (float)s_img->GetWidth()*256.0);
          } else {
            new_width = (int)((float)s_img->GetWidth()/
                                        (float)s_img->GetHeight()*128.0);
            new_height = 128;
          }

          r_img = s_img->Scale( new_width, new_height );
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


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
#include <wx/image.h>               // wxImage
#include <wx/imagpng.h>             // for about html
#include <wx/listctrl.h>            // wxListCtrl

#include "PT/PanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "PT/Panorama.h"

using namespace PT;

// Image Icons
wxImageList* img_icons;

// image preview
wxBitmap *p_img = (wxBitmap *) NULL;

//------------------------------------------------------------------------------

BEGIN_EVENT_TABLE(ImagesPanel, ImagesPanel)
//    EVT_PAINT(ImagesPanel::OnDraw)
/*    EVT_MENU(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_BUTTON(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_MENU(XRCID("action_remove_images"),  MainFrame::OnRemoveImages)
    EVT_BUTTON(XRCID("action_remove_images"),  MainFrame::OnRemoveImages)*/
END_EVENT_TABLE()


// Define a constructor for the Images Panel
ImagesPanel::ImagesPanel(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama* pano)
    //  : wxPanel(parent, -1 , pos, size)
{
    DEBUG_INFO(__FUNCTION__)
    images_list = XRCCTRL(*parent, "images_list", wxListCtrl); 

    DEBUG_INFO(__FUNCTION__)
    images_list->InsertColumn( 0, _("#"), wxLIST_FORMAT_LEFT, 25 );
    images_list->InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 255 );
    images_list->InsertColumn( 2, _("width"), wxLIST_FORMAT_LEFT, 60 );
    images_list->InsertColumn( 3, _("height"), wxLIST_FORMAT_LEFT, 60 );
    images_list->InsertColumn( 4, _("No."), wxLIST_FORMAT_LEFT, 30 );

    DEBUG_INFO(__FUNCTION__)
    img_icons = new wxImageList(24,24);
    DEBUG_INFO(__FUNCTION__)
    images_list->AssignImageList(img_icons, wxIMAGE_LIST_SMALL );//_NORMAL );
 
    // (2) Insert some items into the listctrl
/*    XRCCTRL(*this, "images_list", wxListCtrl)->InsertItem(0,wxT("0"),0);
    XRCCTRL(*this, "images_list", wxListCtrl)->SetItem(0,1,"Todd Hope");
    XRCCTRL(*this, "images_list", wxListCtrl)->InsertItem(1,wxT("1"),0);
    XRCCTRL(*this, "images_list", wxListCtrl)->SetItem(1,1,"Kim Wynd");
    XRCCTRL(*this, "images_list", wxListCtrl)->InsertItem(2,wxT("2"),0);
    XRCCTRL(*this, "images_list", wxListCtrl)->SetItem(2,1,"Leon Li");
*/

    // Image Preview
    DEBUG_INFO(__FUNCTION__)
    p_img = new wxBitmap( 128, 128 );
    wxPanel * img_p = XRCCTRL(*parent, "img_preview_unknown", wxPanel);
    wxPaintDC dc (img_p);
    DEBUG_INFO(__FUNCTION__)
//    dc = dc_;
//    dc.SetPen(* wxRED_PEN);
//    dc.DrawLine(45, 0, 45, 100);

    canvas = new ImgPreview(img_p, wxPoint(0, 0), wxSize(128, 128));
    DEBUG_INFO(__FUNCTION__)

}


ImagesPanel::~ImagesPanel(void)
{
    DEBUG_INFO(__FUNCTION__)
    delete p_img;
}

/*
ImagesPanel::OnAddImages( wxCommandEvent& WXUNUSED(event) )
{
    // Write something in the statusline
    char e_stat[128] = "Number of panoimages(";
    sprintf (e_stat,"%s%d)", e_stat, pano.getNrOfImages() );
    SetStatusText( e_stat, 0);

    // To get users path we do following:
    // a get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    // b store current path
    wxString current;
    current = wxFileName::GetCwd();
    DEBUG_INFO ( (wxString)"get Path: " + wxFileName::GetCwd().c_str() )
    // c remember the last location from config
    if (config->HasEntry(wxT("actualPath"))){
      wxFileName::SetCwd(  config->Read("actualPath").c_str() );
      DEBUG_INFO((wxString)"set Cwd to: " + config->Read("actualPath").c_str() )
    }

    // call the file dialog
    wxFileDialog *dlg = new wxFileDialog(this,_("Add images"), "", "",
        "Images files (*.jpg)|*.jpg|Images files (*.png)|*.png|Images files (*.tif)|*.tif|All files (*.*)|*.*", wxOPEN|wxMULTIPLE , wxDefaultPosition);
    if (dlg->ShowModal() == wxID_OK) {
      // get the list to write to
      wxListCtrl* lst = XRCCTRL(*this, "images_list", wxListCtrl);
      // get the selections
      wxArrayString Filenames;
      wxArrayString Pathnames;
      dlg->GetFilenames(Filenames);
      dlg->GetPaths(Pathnames);
      sprintf(e_stat,"Add images(%d): ", Filenames.GetCount());

      // we got some images to add.
      if (Filenames.GetCount() > 0) {
          // use a Command to ensure proper undo and updating of GUI
          // parts
          std::vector<std::string> filesv;
          for (unsigned int i=0; i< Filenames.GetCount(); i++) {
              filesv.push_back(Filenames[i].c_str());
          }
          GlobalCmdHist::getInstance().addCommand(
              new PT::AddImagesCmd(pano,filesv)
              );
      }

      // remember the added images;
      // FIXME: do not update the view here.
      // the panorama should will call panoramaChanged or
      // panoramaImageAdded whenever an image has been added
      //
      // other parts that might add images do not know what needs
      // to be updated.
      int added ( pano.getNrOfImages() );
      // start the loop for every selected filename
      for ( int i = 0 ; i <= (int)Filenames.GetCount() - 1 ; i++ ) {
        wxImage img (Pathnames[i]);
        // preview first selected image
//        if ( i == 0 ) { // taking only the first one
          wxImage s_img;
          if ( img.GetHeight() > img.GetWidth() ) {
            s_img = img.Scale(
                 (int)((float)img.GetWidth()/(float)img.GetHeight()*128.0),128);
          } else {
            s_img = img.Scale(
                 128,(int)((float)img.GetHeight()/(float)img.GetWidth()*128.0));
          }
          delete p_img;
          p_img = new wxBitmap(s_img);
          canvas->Refresh();

          wxImage i_img;
          if ( s_img.GetHeight() > s_img.GetWidth() ) {
            i_img = s_img.Scale(
               (int)((float)s_img.GetWidth()/(float)s_img.GetHeight()*20.0),20);
          } else {
            i_img = s_img.Scale(
               20,(int)((float)s_img.GetHeight()/(float)s_img.GetWidth()*20.0));
          }
          int ind = img_icons->Add( i_img.ConvertToBitmap() );
        DEBUG_INFO(__FUNCTION__ << "now " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")

//        }
        // fill in the table
        DEBUG_INFO(__FUNCTION__)
        char Nr[8] ;
        sprintf(Nr ,"%d", pano.getNrOfImages()+1);
        DEBUG_INFO( __FUNCTION__ << " icon at Item:" << wxString::Format("%d", ind) )
        lst->InsertItem ( pano.getNrOfImages(), Nr, ind );
        lst->SetColumnWidth(0, wxLIST_AUTOSIZE);
        lst->SetItem ( pano.getNrOfImages(), 1, Filenames[i] );
        sprintf(Nr, "%d", img.GetHeight() );
        lst->SetItem ( pano.getNrOfImages(), 2, Nr );
        sprintf(Nr, "%d", img.GetWidth() );
        lst->SetItem ( pano.getNrOfImages(), 3, Nr );
        sprintf(Nr, "%d", img.GetImageCount (Pathnames[i]) );
        lst->SetItem ( pano.getNrOfImages(), 4, Nr );
//        lst->SetItemImage( pano.getNrOfImages(), pano.getNrOfImages(), pano.getNrOfImages() );

        sprintf( e_stat,"%s %s", e_stat, Filenames[i].c_str() );
        added++;
        DEBUG_INFO(__FUNCTION__)
      }
      SetStatusText( e_stat, 0);

      // update CP panel
      // do not call this by hand. the pano will do this!
//      cpe->ImagesAdded(pano, added);

      // d There we are now?
      wxString str;
      str = dlg->GetDirectory();
      // e safe the current path to config
      config->Write("actualPath", str);  // remember for later
      DEBUG_INFO( (wxString)"save Cwd - " + str )
    } else {
      // nothing to open
      SetStatusText( _("Add Image: cancel"));
    }
    canvas->Refresh();
    // f restore the path previous this dialog
    wxFileName::SetCwd( current );
    DEBUG_INFO ( (wxString)"set Cwd to: " + current )
    dlg->Destroy();
    DEBUG_INFO(__FUNCTION__)
}*/



void ImagesPanel::panoramaChanged (PT::Panorama &pano)
{
      images_list->DeleteAllItems() ; // very radical
      // start the loop for every selected filename
      for ( int i = 0 ; i <= (int)pano.getNrOfImages() - 1 ; i++ ) {
          wxString fn = pano.getImage(i).getFilename().c_str();
          wxImage img ( fn );
        // preview first selected image
//        if ( i == 0 ) { // taking only the first one
          wxImage s_img;
          if ( img.GetHeight() > img.GetWidth() ) {
            s_img = img.Scale(
                (int)((float)img.GetWidth()/(float)img.GetHeight()*128.0), 128);
          } else {
            s_img = img.Scale(
                128, (int)((float)img.GetHeight()/(float)img.GetWidth()*128.0));
          }
          delete p_img;
          p_img = new wxBitmap(s_img);
          canvas->Refresh();

          wxImage i_img;
          if ( s_img.GetHeight() > s_img.GetWidth() ) {
            i_img = s_img.Scale(
               (int)((float)s_img.GetWidth()/(float)s_img.GetHeight()*20.0),20);
          } else {
            i_img = s_img.Scale(
               20,(int)((float)s_img.GetHeight()/(float)s_img.GetWidth()*20.0));
          }
          int ind = img_icons->Add( i_img.ConvertToBitmap() );
        DEBUG_INFO(__FUNCTION__ << "now " << wxString::Format("%d", img_icons->GetImageCount() ) << " images inside img_icons")

//        }
        // fill in the table
        DEBUG_INFO(__FUNCTION__)
        char Nr[8] ;
        sprintf(Nr ,"%d", ind + 1);
        DEBUG_INFO( __FUNCTION__ << " icon at Item:" << wxString::Format("%d", ind) << "/" << wxString::Format("%d", pano.getNrOfImages()) )
        images_list->InsertItem ( i, Nr, ind );
        images_list->SetColumnWidth(0, wxLIST_AUTOSIZE);
        images_list->SetItem ( i, 1, fn );
        sprintf(Nr, "%d", img.GetHeight() );
        images_list->SetItem ( i, 2, Nr );
        sprintf(Nr, "%d", img.GetWidth() );
        images_list->SetItem ( i, 3, Nr );
        sprintf(Nr, "%d", img.GetImageCount ( fn ) );
        images_list->SetItem ( i, 4, Nr );
//        lst->SetItemImage( pano.getNrOfImages(), pano.getNrOfImages(), pano.getNrOfImages() );

        DEBUG_INFO(__FUNCTION__)
      }

}



//------------------------------------------------------------------------------

/*BEGIN_EVENT_TABLE(ImgPreview, wxScrolledWindow)
    EVT_PAINT(ImgPreview::OnPaint)
END_EVENT_TABLE()
*/
// Define a constructor for my canvas
ImgPreview::ImgPreview(wxWindow *parent, const wxPoint& pos, const wxSize& size):
 wxScrolledWindow(parent, -1, pos, size)
{
    DEBUG_INFO(__FUNCTION__)
}

ImgPreview::~ImgPreview(void)
{
    DEBUG_INFO(__FUNCTION__)
    delete p_img;
}

// Define the repainting behaviour
void ImgPreview::OnDraw(wxDC & dc)
{
  DEBUG_INFO(__FUNCTION__)
  if ( p_img && p_img->Ok() )
  {
    wxMemoryDC memDC;
    memDC.SelectObject(* p_img);

    // Transparent blitting if there's a mask in the bitmap
    dc.Blit(0, 0, p_img->GetWidth(), p_img->GetHeight(), & memDC,
      0, 0, wxCOPY, TRUE);

    //img_icons->Draw( 0, dc, 0, 0, wxIMAGELIST_DRAW_NORMAL, FALSE); 

    DEBUG_INFO(__FUNCTION__)
    memDC.SelectObject(wxNullBitmap);
  }
}



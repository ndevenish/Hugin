// -*- c-basic-offset: 4 -*-

/** @file MainFrame.cpp
 *
 *  @brief implementation of MainFrame Class
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
#include <wx/image.h>               // wxImage
#include <wx/imagpng.h>             // for about html
#include <wx/wxhtml.h>              // for about html
#include <wx/listctrl.h>            // wxListCtrl

#include "PT/PanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"
#include "PT/Panorama.h"

using namespace PT;

#if defined(__WXGTK__) || defined(__WXX11__) || defined(__WXMOTIF__) || defined(__WXMAC__) || defined(__WXMGL__)
    #include "xrc/data/gui.xpm"
#endif

// event table. this frame will recieve mostly global commands.
BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(XRCID("action_new_project"),  MainFrame::OnNewProject)
    EVT_MENU(XRCID("action_load_project"),  MainFrame::OnLoadProject)
    EVT_MENU(XRCID("action_save_project"),  MainFrame::OnSaveProject)
    EVT_MENU(XRCID("action_exit_hugin"),  MainFrame::OnExit)
    EVT_MENU(XRCID("action_show_about"),  MainFrame::OnAbout)
    EVT_MENU(XRCID("ID_EDITUNDO"), MainFrame::OnUndo)
    EVT_MENU(XRCID("ID_EDITREDO"), MainFrame::OnRedo)
    EVT_MENU(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_BUTTON(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_MENU(XRCID("action_remove_images"),  MainFrame::OnRemoveImages)
    EVT_BUTTON(XRCID("action_remove_images"),  MainFrame::OnRemoveImages)
    EVT_MENU(XRCID( "action_edit_text_dialog"),  MainFrame::OnTextEdit)
    EVT_CLOSE(  MainFrame::OnExit)
END_EVENT_TABLE()

// change this variable definition
wxTextCtrl *itemProjTextMemo;

MainFrame::MainFrame(wxWindow* parent)
{
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("main_frame"));

    // load our menu bar
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(this, wxT("main_menubar")));

    // create tool bar
    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, wxT("main_toolbar")));

    // image_panel - cryptic light
    // put an "unknown" object in an xrc file and
    // take as wxObject (second argument) the return value of wxXmlResource::Get
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_panel_unknown"),
               wxXmlResource::Get()->LoadPanel (this, wxT("images_panel")) );

    DEBUG_INFO("")
    wxListCtrl* images_list;
    images_list = XRCCTRL(*this, "images_list", wxListCtrl);

    images_list->InsertColumn( 0, _("#"), wxLIST_FORMAT_LEFT, 25 );
    images_list->InsertColumn( 1, _("Filename"), wxLIST_FORMAT_LEFT, 255 );
    images_list->InsertColumn( 2, _("width"), wxLIST_FORMAT_LEFT, 60 );
    images_list->InsertColumn( 3, _("height"), wxLIST_FORMAT_LEFT, 60 );
    images_list->InsertColumn( 4, _("rotate"), wxLIST_FORMAT_LEFT, 60 );

    // (2) Insert some items into the listctrl
/*    XRCCTRL(*this, "images_list", wxListCtrl)->InsertItem(0,wxT("0"),0);
    XRCCTRL(*this, "images_list", wxListCtrl)->SetItem(0,1,"Todd Hope");
    XRCCTRL(*this, "images_list", wxListCtrl)->InsertItem(1,wxT("1"),0);
    XRCCTRL(*this, "images_list", wxListCtrl)->SetItem(1,1,"Kim Wynd");
    XRCCTRL(*this, "images_list", wxListCtrl)->InsertItem(2,wxT("2"),0);
    XRCCTRL(*this, "images_list", wxListCtrl)->SetItem(2,1,"Leon Li");
*/
    // create the custom widget referenced by the main_frame XRC
    cpe = new CPEditorPanel(this,&pano);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_panel_unknown"),
                                               cpe);
    // set the minimize icon
    SetIcon(wxICON(gui));

    // set ourselfs as our dnd handler
    // lets hope wxwindows doesn't try to delete the drop handlers
    SetDropTarget(this);

    // create a status bar
    // I hope that we can also add other widget (like a
    // progress dialog to the status bar
    CreateStatusBar(1);
    SetStatusText("Started");

    // observe the panorama
    pano.setObserver(this);

    // show the frame.
    Show(TRUE);
}

MainFrame::~MainFrame()
{
}

void MainFrame::panoramaChanged(PT::Panorama &panorama)
{
    DEBUG_TRACE("panoramaChanged");
    assert(&pano == &panorama);
    cpe->panoramaChanged(pano);
}


bool MainFrame::OnDropFiles(wxCoord x, wxCoord y,
                            const wxArrayString& filenames)
{
    DEBUG_TRACE("OnDropFiles");
    std::vector<std::string> filesv;
    for (unsigned int i=0; i< filenames.GetCount(); i++) {
        filesv.push_back(filenames[i].c_str());
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::AddImagesCmd(pano,filesv)
        );
    return true;
}

void MainFrame::OnExit(wxCommandEvent & e)
{
    // FIXME ask to save is panorama if unsaved changes exist
    //Close(TRUE);
    this->Destroy();
}


void MainFrame::OnSaveProject(wxCommandEvent & e)
{
    wxLogError("not implemented");
}


void MainFrame::OnLoadProject(wxCommandEvent & e)
{
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxString current;
    current = wxFileName::GetCwd();

  // remember the last location from config
  if (config->HasEntry(wxT("actualPath"))){
    wxFileName::SetCwd(  config->Read("actualPath").c_str() );
    DEBUG_INFO ( (wxString)"set Cwd to: " + config->Read("actualPath").c_str() )
  }

  wxString str;         // look data at debug
  wxFileDialog *dlg = new wxFileDialog(this,_("Open project file"), "", "",
        "Project files (*.pto)|*.pto|All files (*.*)|*.*", wxOPEN, wxDefaultPosition);
  if (dlg->ShowModal() == wxID_OK) {
        str = dlg->GetFilename();
        SetStatusText( _("Open project:   ") + str);
        // read to memo
        str = dlg->GetDirectory();
        // If we load here out project, we should have all other as well in place.
        wxFileName::SetCwd( str );
        config->Write("actualPath", str);  // remember for later
        DEBUG_INFO( (wxString)"save Cwd to - " + str )
//        itemProjTextMemo->LoadFile( dlg->GetFilename() );
        // parse read project
        //TProjectParser parser;
        //wxTextCtrl *m1,*m2,*m3;
        //edit_dialog->GetTextControls(&m1,&m2,&m3);
        //parser->SetControls(m1,m2,m3);
        //parser->ParseScript(&(itemProjTextMemo->GetValue()),&Fainf,true,true);
        //edit_dialog->ParseText2Struct(&(itemProjTextMemo->GetValue()),&Fainf);
//        wxString input_text;
//        input_text = itemProjTextMemo->GetValue();
        //if (edit_dialog->ParseText2Struct(&(input_text),&Fainf,true)) {
        //  itemProjTextMemo->SetValue(input_text);
        //}
  } else {
        // do not close old project
        // nothing to open
        SetStatusText( _("Open project: cancel"));
  }
  dlg->Destroy();
  if (str == wxT("")) {
    wxFileName::SetCwd( current );
    DEBUG_INFO ( (wxString)"set Cwd to: " + current )
        return;
  }
  // parse project
  //OpenProject(str);
  // store current project name as last opened project
    wxFileName::SetCwd( current );
    DEBUG_INFO ( (wxString)"set Cwd to: " + current )
}

void MainFrame::OnNewProject(wxCommandEvent & e)
{
    wxLogError("not implemented");
}

void MainFrame::OnAddImages( wxCommandEvent& WXUNUSED(event) )
{
    char e_stat[128] = "Number of panoimages(";
    sprintf (e_stat,"%s%d)", e_stat, pano.getNrOfImages() );
    SetStatusText( e_stat, 0);

    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    DEBUG_INFO ( (wxString)"get Path: " + config->GetPath().c_str() )

    wxString current;
    current = wxFileName::GetCwd();

    // remember the last location from config
    if (config->HasEntry(wxT("actualPath"))){
      wxFileName::SetCwd(  config->Read("actualPath").c_str() );
      DEBUG_INFO((wxString)"set Cwd to: " + config->Read("actualPath").c_str() )
    }

    wxString str;
    wxFileDialog *dlg = new wxFileDialog(this,_("Add images"), "", "",
        "Images files (*.jpg)|*.jpg|Images files (*.png)|*.png|Images files (*.tif)|*.tif|All files (*.*)|*.*", wxOPEN|wxMULTIPLE , wxDefaultPosition);
    // do the dialog
    if (dlg->ShowModal() == wxID_OK) {
      // get the list to write to
      wxListCtrl* lst = XRCCTRL(*this, "images_list", wxListCtrl);
      char Nr[8] ;
      wxArrayString Filenames;
      wxArrayString Pathnames;
      // get the selections
      dlg->GetFilenames(Filenames);
      dlg->GetPaths(Pathnames);
      sprintf(e_stat,"Add images(%d): ", Filenames.GetCount());

      for ( int i = 0 ; i <= (int)Filenames.GetCount() - 1 ; i++ ) {
        sprintf(Nr ,"%d", pano.getNrOfImages()+1);
        lst->InsertItem ( pano.getNrOfImages(), Nr );
        lst->SetItem ( pano.getNrOfImages(), 1, Filenames[i] );
        pano.addImage( Pathnames[i].c_str() );
        sprintf( e_stat,"%s %s", e_stat, Filenames[i].c_str() );
      }
      SetStatusText( e_stat, 0);

      // If we load here out project, we await all other as well in place.
      str = dlg->GetDirectory();
      wxFileName::SetCwd( str );
      config->Write("actualPath", str);  // remember for later
      DEBUG_INFO( (wxString)"save Cwd - " + str )
      // read in the images
    } else {
      // nothing to open
      SetStatusText( _("Add Image: cancel"));
    }
    dlg->Destroy();
    if (str == wxT("")) {
      wxFileName::SetCwd( current );
      DEBUG_INFO ( (wxString)"set Cwd to: " + current )
      return;
    }
    wxFileName::SetCwd( current );
    DEBUG_INFO ( (wxString)"set Cwd to: " + current )
}

void MainFrame::OnRemoveImages(wxCommandEvent & e)
{
    wxListCtrl* lst =  XRCCTRL(*this, "images_list", wxListCtrl);
    wxString e_msg;
    int sel_I = lst->GetSelectedItemCount();
    if ( sel_I == 1 )
      e_msg = _("Remove image:   ");
    else
      e_msg = _("Remove images:   ");

    for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
      if ( lst->GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
        e_msg += "  " + lst->GetItemText(Nr);
        pano.removeImage(Nr);
        lst->DeleteItem(Nr);
      }
    }

    if ( sel_I == 0 )
      SetStatusText( _("Nothing selected"), 0);
    else
      SetStatusText( e_msg, 0);
}

void MainFrame::OnTextEdit( wxCommandEvent& WXUNUSED(event) )
{
  wxDialog dlg;
  wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("text_edit_dialog"));
  dlg.ShowModal();
  dlg.Show (TRUE);
}

void MainFrame::OnAbout(wxCommandEvent & e)
{
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("about_dlg"));
    dlg.ShowModal();
}

void MainFrame::OnUndo(wxCommandEvent & e)
{
    DEBUG_TRACE("OnUndo");
    GlobalCmdHist::getInstance().undo();
}

void MainFrame::OnRedo(wxCommandEvent & e)
{
    DEBUG_TRACE("OnRedo");
    GlobalCmdHist::getInstance().redo();
}

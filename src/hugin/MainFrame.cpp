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

#include "hugin/config.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"

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
    EVT_MENU(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_BUTTON(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_MENU(XRCID("action_remove images"),  MainFrame::OnRemoveImages)
    EVT_BUTTON(XRCID("action_remove images"),  MainFrame::OnRemoveImages)
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
    DEBUG_INFO("")
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_panel_unknown"),
               wxXmlResource::Get()->LoadPanel (this, wxT("images_panel")) );

    DEBUG_INFO("")
    wxPanel dlg;
    wxXmlResource::Get()->LoadPanel(&dlg, this, wxT("images_panel"));
    DEBUG_INFO("")
    wxListCtrl* images_list = XRCCTRL(dlg, "images_list", wxListCtrl);

//    wxPanel img_panel XRCCTRL(dlg, "images_panel", wxPanel);
    DEBUG_INFO("")
//    XRCCTRL(dlg, "images_list", wxListCtrl)
    images_list->InsertColumn( 0,
                                                       _("Name"),
                                                       wxLIST_FORMAT_LEFT,
                                                       ( 200 )
                                                       );
    DEBUG_INFO("");
    // (2) Insert some items into the listctrl
/*    XRCCTRL(this, "images_list", wxListCtrl)->InsertItem(0,wxT("Todd Hope"));
    XRCCTRL(this, "images_list", wxListCtrl)->InsertItem(1,wxT("Kim Wynd"));
    XRCCTRL(this, "images_list", wxListCtrl)->InsertItem(2,wxT("Leon Li"));
    DEBUG_INFO("");
*/


    //img_panel->

    // create the custom widget referenced by the main_frame XRC
    cpe = new CPEditorPanel(this);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_panel_unknown"),
                                               cpe);
    // set the minimize icon
    SetIcon(wxICON(gui));

    // create a status bar
    // I hope that we can also add other widget (like a
    // progress dialog to the status bar
    CreateStatusBar(1);
    SetStatusText("Started");

    // show the frame.
    Show(TRUE);
}

MainFrame::~MainFrame()
{
}


void MainFrame::OnExit(wxCommandEvent & e)
{
    // FIXME ask to save is panorama is true
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
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    DEBUG_INFO ( (wxString)"get Path: " + config->GetPath().c_str() )

    wxString current;
    current = wxFileName::GetCwd();

  // remember the last location from config
  if (config->HasEntry(wxT("actualPath"))){
    wxFileName::SetCwd(  config->Read("actualPath").c_str() );
    DEBUG_INFO ( (wxString)"set Cwd to: " + config->Read("actualPath").c_str() )
  }

  wxString str;         // look data at debug
  wxFileDialog *dlg = new wxFileDialog(this,_("Add images"), "", "",
        "Images files (*.jpg)|*.jpg,*.JPG|(*.png)|*.png,*.PNG|(*.tif)|*.tif,*.tiff,*.TIF,*.TIFF|All files (*.*)|*.*", wxOPEN, wxDefaultPosition);
  if (dlg->ShowModal() == wxID_OK) {
        str = dlg->GetFilename();
        SetStatusText( _("Add images:   ") + str);
        // If we load here out project, we await all other as well in place.
        str = dlg->GetDirectory();
        wxFileName::SetCwd( str );
        config->Write("actualPath", str);  // remember for later
        DEBUG_INFO( (wxString)"save Cwd - " + str )
        // read in the images
  } else {
        // do not close old project
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
    wxLogError("not implemented");
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
    wxString msg;
    msg << _("Hugin (version")
        << HUGIN_VERSION << _(", compiled on ") << __DATE__")\n"
        << _("A program to generate panoramic images.\n"
             "Uses Panorama Tools to do the actual work\n"
             "\n"
             "Licenced under the GPL\n"
             "Authors (in alphabetical order): \n"
             "Pablo d'Angelo\n"
             "Kai-Uwe Behrmann\n"
             "Juha Helminen\n\n"
             "Homepage: http://hugin.sourceforge.net");
    wxMessageBox(msg, _("About XML resources demo"),
                 wxOK | wxICON_INFORMATION, this);
//------------------------------------------------------------------------------
    wxImage::AddHandler(new wxPNGHandler);

    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("about_dlg"));
    dlg.ShowModal();
}

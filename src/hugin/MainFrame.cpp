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
//#include <wx/image.h>               // wxImage
//#include <wx/imagpng.h>             // for about html
#include <wx/wxhtml.h>              // for about html
#include <wx/listctrl.h>            // wxListCtrl

#include <fstream>

#include "hugin/MainFrame.h"

#include "PT/PanoCommand.h"
#include "hugin/wxPanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/PanoPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/LensPanel.h"
#include "hugin/OptimizeFrame.h"
#include "hugin/huginApp.h"
#include "hugin/CPEditorPanel.h"
#include "PT/Panorama.h"


using namespace PT;

ImagesPanel * images_panel;
LensPanel * lens_panel;
OptimizeVector * optset;

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
    EVT_MENU(XRCID("ID_SHOW_OPTIMIZE_FRAME"), MainFrame::OnToggleOptimizeFrame)
    EVT_BUTTON(XRCID("ID_SHOW_OPTIMIZE_FRAME"),MainFrame::OnToggleOptimizeFrame)
    EVT_MENU(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_BUTTON(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_MENU(XRCID("action_remove_images"),  MainFrame::OnRemoveImages)
    EVT_BUTTON(XRCID("action_remove_images"),  MainFrame::OnRemoveImages)
    EVT_MENU(XRCID( "action_edit_text_dialog"),  MainFrame::OnTextEdit)
    EVT_NOTEBOOK_PAGE_CHANGED(XRCID( "controls_notebook"), MainFrame::UpdatePanels)
    EVT_CLOSE(  MainFrame::OnExit)
END_EVENT_TABLE()

// change this variable definition
//wxTextCtrl *itemProjTextMemo;
// image preview
//wxBitmap *p_img = (wxBitmap *) NULL;
//WX_DEFINE_ARRAY()

MainFrame::MainFrame(wxWindow* parent)
{
    DEBUG_TRACE("");
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
    // finish the images_panel
    DEBUG_TRACE("");
    images_panel = new ImagesPanel( this, wxDefaultPosition, wxDefaultSize,
                                          &pano);

    wxXmlResource::Get()->AttachUnknownControl (
               wxT("images_panel_unknown"),
               images_panel );

    DEBUG_TRACE("");

    // the lens_panel, see as well images_panel
    lens_panel = new LensPanel( this, wxDefaultPosition,
                                wxDefaultSize, &pano);
    // show the lens_panel
    wxXmlResource::Get()->AttachUnknownControl (
               wxT("lens_panel_unknown"),
               lens_panel );


    // the pano_panel
    // The xrc resources are loaded by the class itself.
    DEBUG_TRACE("");
    pano_panel = new PanoPanel(this, &pano);
    wxXmlResource::Get()->AttachUnknownControl (
       wxT("panorama_panel_unknown"),
       pano_panel);

    pano_panel->panoramaChanged (pano); // initialize from pano

    // create the custom widget referenced by the main_frame XRC
    DEBUG_TRACE("");
    cpe = new CPEditorPanel(this,&pano);
    wxXmlResource::Get()->AttachUnknownControl(wxT("cp_editor_panel_unknown"),
                                               cpe);

    opt_frame = new OptimizeFrame(this, &pano);

    // set the minimize icon
    SetIcon(wxICON(gui));

    // set ourselfs as our dnd handler
    // lets hope wxwindows doesn't try to delete the drop handlers
    SetDropTarget(this);
    DEBUG_TRACE("");

    // create a status bar
    // FIXME add a progress dialog to the status bar
    const int fields (2);
    CreateStatusBar(fields);
    int widths[fields] = {-1, 85};
    SetStatusWidths( fields, &widths[0]);
    SetStatusText("Started", 0);
    DEBUG_TRACE("");

    // observe the panorama
    pano.addObserver(this);

    // optimize settings
    optset = new OptimizeVector();


    // show the frame.
//    Show(TRUE);

    DEBUG_TRACE("");
}

MainFrame::~MainFrame()
{
    DEBUG_TRACE("");
//    delete cpe;
    DEBUG_TRACE("");
//    delete images_panel;
    DEBUG_TRACE("");
    pano.removeObserver(this);

    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    // Saves only the size of the window.
    // the position is more problematic, since it might
    // not include a title, making the window move down
    // on every start of hugin, because setPosition sets
    // the upper left title bar position (at least in kwm)
    // netscape navigator had the same problem..
    config->Write("MainFrameSize_x", wxString::Format("%d",GetRect().width)),
    config->Write("MainFrameSize_y", wxString::Format("%d",GetRect().height));
    DEBUG_INFO( "saved last size and position" )

    config->Flush();

    DEBUG_TRACE("");
}


//void MainFrame::panoramaChanged(PT::Panorama &panorama)
void MainFrame::panoramaImagesChanged(PT::Panorama &panorama, const PT::UIntSet & changed)
{
    DEBUG_TRACE("");
    assert(&pano == &panorama);
}


bool MainFrame::OnDropFiles(wxCoord x, wxCoord y,
                            const wxArrayString& filenames)
{
    // FIXME check for Images / project files
    DEBUG_TRACE("OnDropFiles");
    std::vector<std::string> filesv;
    for (unsigned int i=0; i< filenames.GetCount(); i++) {
        filesv.push_back(filenames[i].c_str());
    }
    GlobalCmdHist::getInstance().addCommand(
        new PT::wxAddImagesCmd(pano,filesv)
        );
    return true;
}

void MainFrame::OnExit(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // FIXME ask to save is panorama if unsaved changes exist
    //Close(TRUE);
    this->Destroy();
    DEBUG_TRACE("");
}


void MainFrame::OnSaveProject(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxFileDialog dlg(this,_("Save project file"),
                     wxConfigBase::Get()->Read("actualPath",""),
                     "", "Project files (*.pto)|*.pto|All files (*.*)|*.*",
                     wxSAVE, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        // print as optimizer script..
        std::ofstream script(dlg.GetPath());
        int nImages = pano.getNrOfImages();
        // create fake optimize settings
        PT::OptimizeVector optvec(nImages);
        pano.printOptimizerScript(script, optvec, pano.getOptions());
        script.close();
        wxConfig::Get()->Write("actualPath", dlg.GetDirectory());  // remember for later
    }
}


void MainFrame::OnLoadProject(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxFileDialog dlg(this,_("Open project file"),
                     config->Read("actualPath",""), "",
                     "Project files (*.pto)|*.pto|All files (*.*)|*.*",
                     wxOPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        wxString filename = dlg.GetPath();
        SetStatusText( _("Open project:   ") + filename);
        config->Write("actualPath", dlg.GetDirectory());  // remember for later
        // open project.
        std::ifstream file(filename.c_str());
        if (file.good()) {
            GlobalCmdHist::getInstance().addCommand(
                new LoadPTProjectCmd(pano,file)
                );
            DEBUG_DEBUG("project contains " << pano.getNrOfImages() << " after load");
        } else {
            DEBUG_ERROR("Could not open file " << filename);
        }
    } else {
        // do not close old project
        // nothing to open
        SetStatusText( _("Open project: cancel"));
    }
}

void MainFrame::OnNewProject(wxCommandEvent & e)
{
    GlobalCmdHist::getInstance().addCommand( new NewPanoCmd(pano));
    ImageCache::getInstance().clear();
}

void MainFrame::OnAddImages( wxCommandEvent& WXUNUSED(event) )
{
    // Write something in the statusline
    char e_stat[128] = "";
    sprintf (e_stat,_("Number of panoimages"), e_stat);
    sprintf (e_stat,"%s(%d)", e_stat, pano.getNrOfImages() );
    SetStatusText( e_stat, 0);

    // To get users path we do following:
    // a get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxString wildcard ("Images files (*.jpg)|*.jpg|Images files (*.png)|*.png|Images files (*.tif)|*.tif|All files (*.*)|*.*");
    wxFileDialog *dlg = new wxFileDialog(this,_("Add images"),
                                         config->Read("actualPath",""), "",
                                         wildcard, wxOPEN|wxMULTIPLE , wxDefaultPosition);
    // remember the image extension
    wxString img_ext ("");
    if (config->HasEntry(wxT("lastImageType"))){
      img_ext = config->Read("lastImageType").c_str();
    }
    if      (img_ext == "png")
      dlg->SetFilterIndex(1);
    else if (img_ext == "tif")
      dlg->SetFilterIndex(2);
    else if (img_ext == "all")
      dlg->SetFilterIndex(3);
    DEBUG_TRACE ( img_ext )

    // call the file dialog
    if (dlg->ShowModal() == wxID_OK) {
        // get the selections
        wxArrayString Pathnames;
        wxArrayString Filenames;
        dlg->GetPaths(Pathnames);
        dlg->GetFilenames(Filenames);
        sprintf(e_stat,_("Add images"));
        sprintf(e_stat,"%s(%d): ", e_stat, Pathnames.GetCount());

        // e safe the current path to config
        config->Write("actualPath", dlg->GetDirectory());  // remember for later

        // we got some images to add.
        if (Pathnames.GetCount() > 0) {
            // use a Command to ensure proper undo and updating of GUI
            // parts
            std::vector<std::string> filesv;
            for (unsigned int i=0; i< Pathnames.GetCount(); i++) {
                filesv.push_back(Pathnames[i].c_str());
                // fill the statusline
                sprintf( e_stat,"%s %s", e_stat, Filenames[i].c_str() );
            }
            GlobalCmdHist::getInstance().addCommand(
                new wxAddImagesCmd(pano,filesv)
                );
        }
        SetStatusText( e_stat, 0);
    } else {
        // nothing to open
        SetStatusText( _("Add Image: cancel"));
    }

    DEBUG_TRACE ( wxString::Format("img_ext: %d", dlg->GetFilterIndex()) )
    // save the image extension
    switch ( dlg->GetFilterIndex() ) {
      case 0: config->Write("lastImageType", "jpg"); break;
      case 1: config->Write("lastImageType", "png"); break;
      case 2: config->Write("lastImageType", "tif"); break;
      case 3: config->Write("lastImageType", "all"); break;
    }

    dlg->Destroy();
    config->Flush();
    DEBUG_TRACE("");
}

void MainFrame::OnRemoveImages(wxCommandEvent & e)
{

    DEBUG_TRACE("");
    // get the list to read from
    wxListCtrl* lst =  XRCCTRL(*this, "images_list_unknown", wxListCtrl);

    // prepare an status message
    wxString e_msg;
    int sel_I = lst->GetSelectedItemCount();
    if ( sel_I == 1 )
      e_msg = _("Remove image:   ");
    else
      e_msg = _("Remove images:   ");

    unsigned int imgNr[512] = {0,0};
    for ( int Nr=pano.getNrOfImages()-1 ; Nr>=0 ; --Nr ) {
//      DEBUG_INFO( wxString::Format("now test if removing item %d/%d",Nr,pano.getNrOfImages()) );
      if ( lst->GetItemState( Nr, wxLIST_STATE_SELECTED ) ) {
//    DEBUG_TRACE("");
        e_msg += "  " + lst->GetItemText(Nr);
//    DEBUG_TRACE("");
//        GlobalCmdHist::getInstance().addCommand(
//            new PT::RemoveImageCmd(pano,Nr)
//            );
//    DEBUG_TRACE("");
        imgNr[0] += 1;
        imgNr[imgNr[0]] = Nr; //(unsigned int)Nr;
//        DEBUG_INFO( wxString::Format("will remove %d",Nr) );
//    DEBUG_INFO( wxString::Format("rrrr %d", imgNr[0]) );
//    DEBUG_INFO( wxString::Format("rrrr %d/%d",imgNr[imgNr[0]], imgNr[0]) );
      }
    }
    DEBUG_INFO( wxString::Format("%d+%d+%d+%d+%d",imgNr[0], imgNr[1],imgNr[2], imgNr[3],imgNr[4]) );
    for ( unsigned int i = 1; imgNr[0] >= i ; i++ ) {
//        DEBUG_INFO( wxString::Format("to remove: %d",imgNr[0]) );
        GlobalCmdHist::getInstance().addCommand(
            new PT::RemoveImageCmd(pano,imgNr[i])
            );
//        DEBUG_INFO( wxString::Format("removed %d/%d",imgNr[i], i) );
    }

    if ( sel_I == 0 )
      SetStatusText( _("Nothing selected"), 0);
    else {
      SetStatusText( e_msg, 0);
    }
    DEBUG_TRACE("");
}

void MainFrame::OnTextEdit( wxCommandEvent& WXUNUSED(event) )
{
        DEBUG_TRACE("");
  wxDialog dlg;
  wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("text_edit_dialog"));
  dlg.ShowModal();
//  dlg.Show (TRUE);
        DEBUG_TRACE("");
}

void MainFrame::OnAbout(wxCommandEvent & e)
{
        DEBUG_TRACE("");
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("about_dlg"));
    dlg.ShowModal();
}

void MainFrame::UpdatePanels( wxCommandEvent& WXUNUSED(event) )
{   // Maybe this can be invoced by the Panorama::Changed() or
    // something like this. So no everytime update would be needed.
    DEBUG_TRACE("");
}


void MainFrame::OnToggleOptimizeFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    opt_frame->Show();
    opt_frame->Raise();
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

MainFrame* MainFrame::GetFrame(void)
{
    DEBUG_TRACE("");
//    MainFrame* dummy( parent);//, -1, wxDefaultPosition, wxDefaultPosition);
    return this;
}



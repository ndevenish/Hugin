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
#include <wx/splash.h>
#include <wx/file.h>

#include <fstream>

#include "hugin/MainFrame.h"

#include "PT/PanoCommand.h"
#include "hugin/wxPanoCommand.h"
#include "hugin/config.h"
#include "hugin/CommandHistory.h"
#include "hugin/PanoPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/LensPanel.h"
#include "hugin/OptimizePanel.h"
#include "hugin/PreviewFrame.h"
#include "hugin/huginApp.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/CPListFrame.h"

#include "jhead/jhead.h"

#include "PT/Panorama.h"


using namespace PT;

//ImagesPanel * images_panel;
//LensPanel * lens_panel;
//OptimizeVector * optset;


/** file drag and drop handler method */
bool PanoDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
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


// event table. this frame will recieve mostly global commands.
BEGIN_EVENT_TABLE(MainFrame, wxFrame)
    EVT_MENU(XRCID("action_new_project"),  MainFrame::OnNewProject)
    EVT_MENU(XRCID("action_load_project"),  MainFrame::OnLoadProject)
    EVT_MENU(XRCID("action_save_project"),  MainFrame::OnSaveProject)
    EVT_MENU(XRCID("action_save_as_project"),  MainFrame::OnSaveProjectAs)
    EVT_MENU(XRCID("action_exit_hugin"),  MainFrame::OnExit)
    EVT_MENU(XRCID("action_show_about"),  MainFrame::OnAbout)
    EVT_MENU(XRCID("action_show_help"),  MainFrame::OnHelp)
    EVT_MENU(XRCID("ID_EDITUNDO"), MainFrame::OnUndo)
    EVT_MENU(XRCID("ID_EDITREDO"), MainFrame::OnRedo)
//    EVT_MENU(XRCID("ID_SHOW_OPTIMIZE_FRAME"), MainFrame::OnToggleOptimizeFrame)
//    EVT_BUTTON(XRCID("ID_SHOW_OPTIMIZE_FRAME"),MainFrame::OnToggleOptimizeFrame)
    EVT_MENU(XRCID("ID_SHOW_PREVIEW_FRAME"), MainFrame::OnTogglePreviewFrame)
    EVT_BUTTON(XRCID("ID_SHOW_PREVIEW_FRAME"),MainFrame::OnTogglePreviewFrame)

    EVT_MENU(XRCID("action_optimize"),  MainFrame::OnOptimize)
    EVT_BUTTON(XRCID("action_optimize"),  MainFrame::OnOptimize)

    EVT_MENU(XRCID("ID_CP_TABLE"), MainFrame::OnToggleCPFrame)
    EVT_BUTTON(XRCID("ID_CP_TABLE"),MainFrame::OnToggleCPFrame)

    EVT_MENU(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_BUTTON(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_MENU(XRCID("action_add_time_images"),  MainFrame::OnAddTimeImages)
    EVT_BUTTON(XRCID("action_add_time_images"),  MainFrame::OnAddTimeImages)
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

MainFrame::MainFrame(wxWindow* parent, Panorama & pano)
    : pano(pano)
{
    wxConfigBase* config = wxConfigBase::Get();
    if ( wxFile::Exists("xrc/data/splash.png") ) {
        DEBUG_INFO("using local xrc files");
        wxString currentDir = wxFileName::GetCwd();
        m_xrcPrefix = currentDir + "/xrc/";
    } else if ( wxFile::Exists((wxString)wxT(INSTALL_XRC_DIR) + wxT("/data/splash.png")) ) {
        DEBUG_INFO("using installed xrc files");
        m_xrcPrefix = (wxString)wxT(INSTALL_XRC_DIR) + wxT("/");
    } else {
        DEBUG_INFO("using xrc prefix from config")
        m_xrcPrefix = config->Read("xrc_path") + wxT("/");
    }

    wxBitmap bitmap;
    wxSplashScreen* splash = 0;
    if (bitmap.LoadFile(m_xrcPrefix + "data/splash.png", wxBITMAP_TYPE_PNG))
    {
#ifdef __unix__
        splash = new wxSplashScreen(bitmap,
                              wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_NO_TIMEOUT,
                              0, NULL, -1, wxDefaultPosition,
                          wxDefaultSize,
                  wxSIMPLE_BORDER|wxSTAY_ON_TOP);
#else
        splash = new wxSplashScreen(bitmap,
                           wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_TIMEOUT,
                           2000, NULL, -1, wxDefaultPosition,
                       wxDefaultSize,
                   wxSIMPLE_BORDER|wxSTAY_ON_TOP);
#endif
    } else {
        DEBUG_ERROR("splash image not found");
    }
    wxYield();

    // save our pointer
    m_this = this;

    DEBUG_TRACE("");
    // load our children. some children might need special
    // initialization. this will be done later.
    wxXmlResource::Get()->LoadFrame(this, parent, wxT("main_frame"));
    DEBUG_TRACE("");

    // load our menu bar
    SetMenuBar(wxXmlResource::Get()->LoadMenuBar(this, wxT("main_menubar")));

    // create tool bar
    SetToolBar(wxXmlResource::Get()->LoadToolBar(this, wxT("main_toolbar")));

    // image_panel
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

    m_notebook = XRCCTRL(*this, "controls_notebook", wxNotebook);
    DEBUG_ASSERT(m_notebook);

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

    opt_panel = new OptimizePanel(this, &pano);
    // create the custom widget referenced by the main_frame XRC
    DEBUG_TRACE("");
    wxXmlResource::Get()->AttachUnknownControl(wxT("optimizer_panel_unknown"),
                                               opt_panel);

    preview_frame = new PreviewFrame(this, pano);

    cp_frame = new CPListFrame(this, pano);

    // set the minimize icon
    SetIcon(wxIcon(m_xrcPrefix + "/data/icon.png", wxBITMAP_TYPE_PNG));

    // create a new drop handler. wxwindows deletes the automaticall
    SetDropTarget(new PanoDropTarget(pano));
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


#ifdef __WXMSW__
    int w = config->Read("MainFrame/width",-1l);
    int h = config->Read("MainFrame/height",-1l);
    int x = config->Read("MainFrame/positionX",-1l);
    int y = config->Read("MainFrame/positionY",-1l);
    if (y != -1) {
        SetSize(x,y,w,h);
    }
#else
    // remember the last size from config
    int w = config->Read("MainFrame/width",-1l);
    int h = config->Read("MainFrame/height",-1l);
    if (w != -1) {
        SetClientSize(w,h);
    } else {
        Fit();
    }
#endif

    // set progress display for image cache.
    ImageCache::getInstance().setProgressDisplay(this);

#ifdef __unix__
    if(splash) {
        delete splash;
    }
#endif

    DEBUG_TRACE("");
}

MainFrame::~MainFrame()
{
    DEBUG_TRACE("dtor");
    ImageCache::getInstance().setProgressDisplay(0);
//    delete cpe;
//    delete images_panel;
    DEBUG_DEBUG("removing observer");
    pano.removeObserver(this);

    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();


#ifdef __WXMSW__
    wxRect sz = GetRect();
    config->Write("/MainFrame/width", sz.width);
    config->Write("/MainFrame/height", sz.height);

    config->Write("/MainFrame/positionX", sz.x);
    config->Write("/MainFrame/positionY", sz.y);
#else

    // Saves only the size of the window.
    // the position is more problematic, since it might
    // not include a title, making the window move down
    // on every start of hugin, because setPosition sets
    // the upper left title bar position (at least in kwm)
    // netscape navigator had the same problem..
    wxSize sz = GetClientSize();
    config->Write("/MainFrame/width", sz.GetWidth());
    config->Write("/MainFrame/height", sz.GetHeight());
#endif
    DEBUG_INFO( "saved last size and position" )

    config->Flush();

    DEBUG_TRACE("dtor end");
}


//void MainFrame::panoramaChanged(PT::Panorama &panorama)
void MainFrame::panoramaImagesChanged(PT::Panorama &panorama, const PT::UIntSet & changed)
{
    DEBUG_TRACE("");
    assert(&pano == &panorama);
}

void MainFrame::OnExit(wxCloseEvent & e)
{
    DEBUG_TRACE("");
    // FIXME ask to save is panorama if unsaved changes exist
    if (pano.isDirty()) {
        int answer = wxMessageBox(_("The panorama has been changed\nSave changes?"), _("Save Panorama?"), wxYES_NO | wxCANCEL | wxICON_EXCLAMATION, this);
        switch (answer){
        case wxYES:
        {
            wxCommandEvent dummy;
            OnSaveProject(dummy);
            break;
        }
        case wxCANCEL:
        {
            // try to cancel quit
            if (e.CanVeto()) {
                e.Veto();
                return;
            }
            wxLogError(_("forced close"));
            break;
        }
        default:
        {
            // just quit
        }
        }
    }
    ImageCache::getInstance().flush();
    //Close(TRUE);
    this->Destroy();
    DEBUG_TRACE("");
}

void MainFrame::OnSaveProject(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (m_filename == "") {
        OnSaveProjectAs(e);
    } else {
        // the project file is just a PTOptimizer script...
        wxFileName scriptName = m_filename;
        std::string path(
            scriptName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
        DEBUG_DEBUG("stripping " << path << " from image filenames");
        std::ofstream script(scriptName.GetFullPath());
        PT::OptimizeVector optvec = opt_panel->getOptimizeVector();
        pano.printOptimizerScript(script, optvec, pano.getOptions(), path);
        script.close();
    }
    SetStatusText(wxString::Format(_("saved project %s"), m_filename.c_str()),0);
    pano.clearDirty();
}

void MainFrame::OnSaveProjectAs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxFileDialog dlg(this,
                     _("Save project file"),
                     wxConfigBase::Get()->Read("actualPath",""), "",
                     "Project files (*.pto)|*.pto|"
                     "All files (*.*)|*.*",
                     wxSAVE, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        m_filename = dlg.GetPath();
        OnSaveProject(e);
        wxConfig::Get()->Write("actualPath", dlg.GetDirectory());  // remember for later
    }
}

void MainFrame::OnLoadProject(wxCommandEvent & e)
{
    DEBUG_TRACE("");

    // remove old images from cache
    ImageCache::getInstance().flush();

    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxFileDialog dlg(this,
                     _("Open project file"),
                     config->Read("actualPath",""), "",
                     "Project files (*.pto)|*.pto|"
                     "All files (*.*)|*.*",
                     wxOPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        wxFileName scriptName = dlg.GetPath();
        m_filename = dlg.GetPath();
        std::string prefix(scriptName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
        wxString filename = dlg.GetPath();
        SetStatusText( _("Open project:   ") + filename);
        config->Write("actualPath", dlg.GetDirectory());  // remember for later
        // open project.
        std::ifstream file(filename.c_str());
        if (file.good()) {
            wxBusyCursor();
            GlobalCmdHist::getInstance().addCommand(
                new LoadPTProjectCmd(pano,file, prefix)
                );
            DEBUG_DEBUG("project contains " << pano.getNrOfImages() << " after load");
        opt_panel->setOptimizeVector(pano.getOptimizeVector());
            SetStatusText(_("Project opened"));
        } else {
            DEBUG_ERROR("Could not open file " << filename);
        }
    } else {
        // do not close old project
        // nothing to open
        SetStatusText( _("Open project: cancel"));
    }
    pano.clearDirty();
}

void MainFrame::OnNewProject(wxCommandEvent & e)
{
    m_filename = "";
    GlobalCmdHist::getInstance().addCommand( new NewPanoCmd(pano));
    // remove old images from cache
    ImageCache::getInstance().flush();
    pano.clearDirty();
}

void MainFrame::OnAddImages( wxCommandEvent& event )
{
    wxString e_stat;

    // To get users path we do following:
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxString wildcard ("Image files (*.jpg)|*.jpg;*.JPG|"
                       "Image files (*.png)|*.png;*.PNG|"
                       "Image files (*.tif)|*.tif;*.TIF|"
                       "All files (*.*)|*.*");
    wxFileDialog *dlg =
        new wxFileDialog(this,_("Add images"),
                         config->Read("actualPath",""), "",
                         wildcard, wxOPEN|wxMULTIPLE , wxDefaultPosition);

    // remember the image extension
    wxString img_ext ("");
    if (config->HasEntry(wxT("lastImageType"))){
      img_ext = config->Read("lastImageType").c_str();
    }
    if (img_ext == "png")
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

        // e safe the current path to config
        config->Write("actualPath", dlg->GetDirectory());  // remember for later

        std::vector<std::string> filesv;
        for (unsigned int i=0; i< Pathnames.GetCount(); i++) {
            filesv.push_back(Pathnames[i].c_str());
        }

        // we got some images to add.
        if (filesv.size() > 0) {
            // use a Command to ensure proper undo and updating of GUI
            // parts
            wxBusyCursor();
            GlobalCmdHist::getInstance().addCommand(
                new wxAddImagesCmd(pano,filesv)
                );
        }
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
    DEBUG_TRACE("");
}

time_t ReadJpegTime(const char* filename)
{
    // Skip it if it isn't JPEG or there's no EXIF.
    ResetJpgfile();
    ImageInfo_t exif;
    memset(&exif, 0, sizeof(exif));
    if (!ReadJpegFile(exif, filename, READ_EXIF))
        return 0;
    // Remember the file and a shutter timestamp.
    time_t stamp;
    struct tm when;
    memset(&when, 0, sizeof(when));
    Exif2tm(&when, exif.DateTime);
    stamp = mktime(&when);
    if (stamp == (time_t)(-1))
        return 0;
    return stamp;
}

void MainFrame::OnAddTimeImages( wxCommandEvent& event )
{
    DEBUG_TRACE("");

    // If no images already loaded, offer user a chance to pick one.
    int images = pano.getNrOfImages();
    if (!images) {
        OnAddImages(event);
        images = pano.getNrOfImages();
        if (!images)
            return;
    }

    DEBUG_TRACE("seeking similarly timed images");

    // Collect potential image-mates.
    WX_DECLARE_STRING_HASH_MAP(time_t, StringToPointerHash);
    StringToPointerHash filenames;
    while (images)
    {
        --images;

        // Get the filename.  Naively assumes there are no Unicode issues.
        const PanoImage& image = pano.getImage(images);
        std::string filename = image.getFilename();
        wxString file = filename.c_str();
        filenames[file] = 0;

        // Glob for all files of same type in same directory.
        wxString name = filename.c_str();
        wxString path = ::wxPathOnly(name) + "/*";
        file = ::wxFindFirstFile(path);
        while (!file.IsEmpty())
        {
            // Associated with a NULL dummy timestamp for now.
            filenames[file] = 0;
            file = ::wxFindNextFile();
        }
    }

    DEBUG_TRACE("found " << filenames.size() <<
                " candidate files to search.");

    // For each globbed or loaded file,
    StringToPointerHash::iterator found;
    for (found = filenames.begin(); found != filenames.end(); found++)
    {
        wxString file = found->first;
        // Check the time if it's got a camera EXIF timestamp.
        time_t stamp = ReadJpegTime(file.c_str());
        if (stamp)
            filenames[file] = stamp;
    }

    //TODO: sorting the filenames keys by timestamp would be useful

    int changed;
    do
    {
        changed = FALSE;

        // For each timestamped file,
        for (found = filenames.begin(); found != filenames.end(); found++)
        {
            wxString recruit = found->first;
            time_t pledge = filenames[recruit];
            if (!pledge)
                continue;

            // For each other image already loaded,
            images = pano.getNrOfImages();
            while (images)
            {
                --images;
                const PanoImage& image = pano.getImage(images);
                std::string filename = image.getFilename();
                wxString file = filename.c_str();
                if (file == recruit)
                    continue;

                // If it is within threshold time,
                //TODO: user preference, adjustable threshold?
                time_t stamp = filenames[file];
                if (abs(pledge - stamp) < 60)
                {
                    // Load this file, and remember it.
                    DEBUG_TRACE("Recruited " << recruit.c_str());
                    std::vector<std::string> filesv;
                    std::string file = recruit.c_str();
                    filesv.push_back(file);
                    wxBusyCursor();
                    GlobalCmdHist::getInstance().addCommand(
                        new wxAddImagesCmd(pano,filesv)
                        );
                    // Don't recruit it again.
                    filenames[recruit] = 0;
                    // Count this as a change.
                    changed++;
                    break;
                }
            }
        }
    } while (changed);

    // Load all of the named files.
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

void MainFrame::OnHelp(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("help_dlg"));
    dlg.ShowModal();
}

void MainFrame::UpdatePanels( wxCommandEvent& WXUNUSED(event) )
{   // Maybe this can be invoced by the Panorama::Changed() or
    // something like this. So no everytime update would be needed.
    DEBUG_TRACE("");
}


#if 0
void MainFrame::OnToggleOptimizeFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    opt_frame->Show();
    opt_frame->Raise();
}
#endif

void MainFrame::OnTogglePreviewFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    preview_frame->Show();
    preview_frame->Raise();
}

void MainFrame::OnToggleCPFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    cp_frame->Show();
    cp_frame->Raise();
}

void MainFrame::OnOptimize(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxCommandEvent dummy;
    opt_panel->OnOptimizeButton(dummy);
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

void MainFrame::ShowCtrlPoint(unsigned int cpNr)
{
    DEBUG_DEBUG("Showing control point " << cpNr);
    m_notebook->SetSelection(2);
    cpe->ShowControlPoint(cpNr);
}

void MainFrame::progressMessage(const std::string & msg,
                                int progress)
{
    SetStatusText(msg.c_str());
    if (progress >= 0) {
        SetStatusText(wxString::Format("%d%%",progress),1);
    } else {
        SetStatusText("",1);
    }
    wxYield();
}

    /// hack.. kind of a pseudo singleton...
MainFrame * MainFrame::Get()
{
    if (m_this) {
        return m_this;
    } else {
        DEBUG_FATAL("MainFrame not yet created");
        DEBUG_ASSERT(m_this);
        return 0;
    }
}

MainFrame * MainFrame::m_this = 0;

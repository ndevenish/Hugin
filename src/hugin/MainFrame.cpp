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

#include <config.h>
#include "panoinc_WX.h"
#include <wx/tipdlg.h>
#include "panoinc.h"

//Mac bundle code by Ippei
#ifdef __WXMAC__
#include <CFBundle.h>
#include "wx/mac/private.h"
#endif

#include "vigra_ext/Correlation.h"

#include "jhead/jhead.h"

#include "hugin/config_defaults.h"
#include "hugin/PreferencesDialog.h"
#include "hugin/MainFrame.h"
#include "hugin/wxPanoCommand.h"
#include "hugin/CommandHistory.h"
#include "hugin/PanoPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/LensPanel.h"
#include "hugin/OptimizePanel.h"
#include "hugin/PreviewFrame.h"
#include "hugin/huginApp.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/CPListFrame.h"
#include "hugin/MyProgressDialog.h"
#include "hugin/ImageCache.h"



using namespace PT;
using namespace utils;
using namespace std;

//ImagesPanel * images_panel;
//LensPanel * lens_panel;
//OptimizeVector * optset;

#ifdef __MINGW32__
// fixes for mingw compilation...
#undef FindWindow
#endif

/** file drag and drop handler method */
bool PanoDropTarget::OnDropFiles(wxCoord x, wxCoord y, const wxArrayString& filenames)
{
    DEBUG_TRACE("OnDropFiles");
    if (filenames.GetCount() == 1) {
        wxFileName file(filenames[0]);
        if (file.GetExt().CmpNoCase(wxT("pto")) == 0 ||
            file.GetExt().CmpNoCase(wxT("ptp")) == 0 ||
            file.GetExt().CmpNoCase(wxT("pts")) == 0 )
        {
            if (MainFrame::Get()) {
                // load project
                MainFrame::Get()->LoadProjectFile(file.GetFullPath());
            }
            return true;
        }
    }
    // try to add as images
    std::vector<std::string> filesv;
    for (unsigned int i=0; i< filenames.GetCount(); i++) {
        wxFileName file(filenames[0]);

        if (file.GetExt().CmpNoCase(wxT("jpg")) == 0 ||
            file.GetExt().CmpNoCase(wxT("tif")) == 0 ||
            file.GetExt().CmpNoCase(wxT("tiff")) == 0 ||
            file.GetExt().CmpNoCase(wxT("png")) == 0 ||
            file.GetExt().CmpNoCase(wxT("bmp")) == 0 ||
            file.GetExt().CmpNoCase(wxT("gif")) == 0 ||
            file.GetExt().CmpNoCase(wxT("pnm")) == 0 ||
            file.GetExt().CmpNoCase(wxT("sun")) == 0 ||
            file.GetExt().CmpNoCase(wxT("viff")) == 0 )
        {
            filesv.push_back((const char *)filenames[i].mb_str());
        }
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
    EVT_MENU(XRCID("action_save_as_ptstitcher"),  MainFrame::OnSavePTStitcherAs)
    EVT_MENU(XRCID("action_exit_hugin"),  MainFrame::OnUserQuit)
    EVT_MENU(XRCID("action_show_about"),  MainFrame::OnAbout)
    EVT_MENU(XRCID("action_show_help"),  MainFrame::OnHelp)
    EVT_MENU(XRCID("action_show_tip"),  MainFrame::OnTipOfDay)
    EVT_MENU(XRCID("action_show_shortcuts"),  MainFrame::OnKeyboardHelp)
    EVT_MENU(XRCID("action_show_faq"),  MainFrame::OnFAQ)
    EVT_MENU(XRCID("action_show_prefs"), MainFrame::OnShowPrefs)
    EVT_MENU(XRCID("ID_EDITUNDO"), MainFrame::OnUndo)
    EVT_MENU(XRCID("ID_EDITREDO"), MainFrame::OnRedo)
    EVT_MENU(XRCID("ID_SHOW_PREVIEW_FRAME"), MainFrame::OnTogglePreviewFrame)
    EVT_BUTTON(XRCID("ID_SHOW_PREVIEW_FRAME"),MainFrame::OnTogglePreviewFrame)

    EVT_MENU(XRCID("action_optimize"),  MainFrame::OnOptimize)
    EVT_BUTTON(XRCID("action_optimize"),  MainFrame::OnOptimize)
    EVT_MENU(XRCID("action_finetune_all_cp"), MainFrame::OnFineTuneAll)
    EVT_BUTTON(XRCID("action_finetune_all_cp"), MainFrame::OnFineTuneAll)

    EVT_MENU(XRCID("ID_CP_TABLE"), MainFrame::OnToggleCPFrame)
    EVT_BUTTON(XRCID("ID_CP_TABLE"),MainFrame::OnToggleCPFrame)

    EVT_MENU(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_BUTTON(XRCID("action_add_images"),  MainFrame::OnAddImages)
    EVT_MENU(XRCID("action_add_time_images"),  MainFrame::OnAddTimeImages)
    EVT_BUTTON(XRCID("action_add_time_images"),  MainFrame::OnAddTimeImages)
    EVT_MENU(XRCID( "action_edit_text_dialog"),  MainFrame::OnTextEdit)
    //EVT_NOTEBOOK_PAGE_CHANGED(XRCID( "controls_notebook"), MainFrame::UpdatePanels)
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
    wxString splashPath;
    wxFileName::SplitPath( wxTheApp->argv[0], &splashPath, NULL, NULL );
		
    wxConfigBase* config = wxConfigBase::Get();
    if ( wxFile::Exists(splashPath + wxT("/xrc/data/splash.png")) ) {
        DEBUG_INFO("using local xrc files");
        m_xrcPrefix = splashPath + wxT("/xrc");
    } else if ( wxFile::Exists((wxString)wxT(INSTALL_XRC_DIR) + wxT("/data/splash.png")) ) {
        DEBUG_INFO("using installed xrc files");
        m_xrcPrefix = (wxString)wxT(INSTALL_XRC_DIR);
    } else {
        DEBUG_INFO("using xrc prefix from config")
        m_xrcPrefix = config->Read(wxT("xrc_path"));
    }
#ifndef __WXMSW__
    // MakeAbsolute creates an invalid path out of a perfectly valid,
    // absolute path under windows.  Observed with wxWindows 2.4.2.
    // do not try to transform into an absolute path under windows.
    // (hugin is started with an absolute path there anyway).
	{
  	  // make sure the path is absolute to avoid problems if the cwd is changed
  	  wxFileName fn(m_xrcPrefix + wxT("/about.xrc"));
  	  if (fn.IsRelative())
	  {
		fn.MakeAbsolute();
	  }
  	  m_xrcPrefix = fn.GetPath() + wxT("/");
	}
#else
    m_xrcPrefix = m_xrcPrefix + wxT("/");
#endif
    DEBUG_INFO("XRC prefix set to: " << m_xrcPrefix.mb_str());

    /* start: Mac bundle code by Ippei*/
#ifdef __WXMAC__

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(!mainbundle)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyResourceURL(mainbundle, CFSTR("xrc"), NULL, NULL);
        if(!XRCurl)
        {
            DEBUG_INFO("Mac: Cannot locate xrc in the bundle.");
        }
        else
        {
            CFStringRef pathInCFString = CFURLCopyFileSystemPath(XRCurl, kCFURLPOSIXPathStyle);
            if(!pathInCFString)
            {
                CFRelease( XRCurl );
                DEBUG_INFO("Mac: Failed to get URL in CFString");
            }
            else
            {
                CFRelease( XRCurl );
                wxString pathInWXString = wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                
                m_xrcPrefix = pathInWXString + wxT("/");
                DEBUG_INFO("Mac: overriding xrc prefix; using mac bundled xrc files");
            }
        }
    }
    /* end: Mac bundle code by Ippei*/
#endif

    wxBitmap bitmap;
    wxSplashScreen* splash = 0;
    wxYield();
    if (bitmap.LoadFile(m_xrcPrefix + wxT("data/splash.png"), wxBITMAP_TYPE_PNG))
    {
#ifdef __unix__
        splash = new wxSplashScreen(bitmap,
                              wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_NO_TIMEOUT,
                              0, NULL, -1, wxDefaultPosition,
                                    wxDefaultSize,
                                    wxSIMPLE_BORDER|wxSTAY_ON_TOP);
#else
        splash = new wxSplashScreen(bitmap,
                                    wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_NO_TIMEOUT,
                                    0, NULL, -1, wxDefaultPosition,
                                    wxDefaultSize,
                                    wxSIMPLE_BORDER);
#endif
    } else {
        wxLogFatalError(_("Fatal installation error\nThe xrc directory was not found.\nPlease ensure it is placed in the same directory as hugin.exe"));
        abort();
    }
    splash->Refresh();
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

    m_notebook = XRCCTRL((*this), "controls_notebook", wxNotebook);
//    m_notebook = ((wxNotebook*) ((*this).FindWindow(XRCID("controls_notebook"))));
//    m_notebook = ((wxNotebook*) (FindWindow(XRCID("controls_notebook"))));
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

    pref_dlg = new PreferencesDialog(this);

    // set the minimize icon
#if __WXMSW__
    wxIcon myIcon(m_xrcPrefix + wxT("data/icon.ico"),wxBITMAP_TYPE_ICO);
#else
    wxIcon myIcon(m_xrcPrefix + wxT("data/icon.png"),wxBITMAP_TYPE_PNG);
#endif
    SetIcon(myIcon);

    // create a new drop handler. wxwindows deletes the automaticall
    SetDropTarget(new PanoDropTarget(pano));
    DEBUG_TRACE("");

    // create a status bar
    // FIXME add a progress dialog to the status bar
    const int fields (2);
    CreateStatusBar(fields);
    int widths[fields] = {-1, 85};
    SetStatusWidths( fields, &widths[0]);
    SetStatusText(_("Started"), 0);
    wxYield();
    DEBUG_TRACE("");

    // observe the panorama
    pano.addObserver(this);

    // Set sizing characteristics
#ifdef USE_WX26x
    SetSizeHints(600,400);
#endif
    bool maximized = config->Read(wxT("/MainFrame/maximized"), 0l) != 0;
    if (maximized) {
        this->Maximize();
    } else {
        //size
        int w = config->Read(wxT("/MainFrame/width"),-1l);
        int h = config->Read(wxT("/MainFrame/height"),-1l);
        if (w >0) {
            SetClientSize(w,h);
        } else {
            Fit();
        }
        //position
        int x = config->Read(wxT("/MainFrame/positionX"),-1l);
        int y = config->Read(wxT("/MainFrame/positionY"),-1l);
        if ( y > 0) {
            Move(x, y);
        } else {
            Move(0, 44);
        }
    }

    // set progress display for image cache.
    ImageCache::getInstance().setProgressDisplay(this);

    if(splash) {
        splash->Close();
    }
    wxYield();
	
#ifdef DEBUG
#ifdef __WXMSW__

    freopen("c:\\hugin_stdout.txt", "w", stdout);    // redirect stdout to file
    freopen("c:\\hugin_stderr.txt", "w", stderr);    // redirect stderr to file

#endif
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

    if (! this->IsMaximized() ) {
        wxSize sz = GetClientSize();
        config->Write(wxT("/MainFrame/width"), sz.GetWidth());
        config->Write(wxT("/MainFrame/height"), sz.GetHeight());
        wxPoint ps = GetPosition();
        config->Write(wxT("/MainFrame/positionX"), ps.x);
        config->Write(wxT("/MainFrame/positionY"), ps.y);
        config->Write(wxT("/MainFrame/maximized"), 0);
    } else {
        config->Write(wxT("/MainFrame/maximized"), 1l);
    }
        
    config->Flush();

    DEBUG_TRACE("dtor end");
}

void MainFrame::RestoreLayout()
{
	DEBUG_TRACE("");	
    // restore layout of child widgets, now that all widgets have been created,
    // are of similar size
    cpe->RestoreLayout();
    lens_panel->RestoreLayout();
    images_panel->RestoreLayout();
	DEBUG_TRACE("");	
}

//void MainFrame::panoramaChanged(PT::Panorama &panorama)
void MainFrame::panoramaImagesChanged(PT::Panorama &panorama, const PT::UIntSet & changed)
{
    DEBUG_TRACE("");
    assert(&pano == &panorama);
}

void MainFrame::OnUserQuit(wxCommandEvent & e)
{
    Close();
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
    if (pano.getNrOfImages() == 0) {
	    wxMessageBox(_("No images - Nothing to do"),_("Warning"), wxOK | wxICON_EXCLAMATION);
		return;
	}
    wxFileName scriptName = m_filename;
    if (m_filename == wxT("")) {
        OnSaveProjectAs(e);
        scriptName = m_filename;
    } else {
        // the project file is just a PTOptimizer script...
        std::string path(
            scriptName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR).mb_str());
        DEBUG_DEBUG("stripping " << path << " from image filenames");
        std::ofstream script(scriptName.GetFullPath().mb_str());
        PT::OptimizeVector optvec = opt_panel->getOptimizeVector();
        PT::UIntSet all;
        fill_set(all, 0, pano.getNrOfImages()-1);
        pano.printOptimizerScript(script, optvec, pano.getOptions(), all, path);
        script.close();
    }
    SetStatusText(wxString::Format(_("saved project %s"), m_filename.c_str()),0);
    this->SetTitle(scriptName.GetName() + wxT(".") + scriptName.GetExt() + wxT(" - hugin"));
    pano.clearDirty();
}

void MainFrame::OnSaveProjectAs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (pano.getNrOfImages() == 0) {
	    wxMessageBox(_("No images - Nothing to do"),_("Warning"), wxOK | wxICON_EXCLAMATION);
		return;
	}
    wxFileDialog dlg(this,
                     _("Save project file"),
                     wxConfigBase::Get()->Read(wxT("actualPath"),wxT("")), wxT(""),
                     _("Project files (*.pto)|*.pto|All files (*.*)|*.*"),
                     wxSAVE, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        m_filename = dlg.GetPath();
        OnSaveProject(e);
        wxConfig::Get()->Write(wxT("actualPath"), dlg.GetDirectory());  // remember for later
    }
}

void MainFrame::OnSavePTStitcherAs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (pano.getNrOfImages() == 0) {
	    wxMessageBox(_("No images - Nothing to do"),_("Warning"), wxOK | wxICON_EXCLAMATION);
		return;
	}
    wxFileDialog dlg(this,
                     _("Save PTStitcher script file"),
                     wxConfigBase::Get()->Read(wxT("actualPath"),wxT("")), wxT(""),
                     _("PTStitcher files (*.txt)|*.txt"),
                     wxSAVE, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        wxString fname = dlg.GetPath();
        // the project file is just a PTStitcher script...
        wxFileName scriptName = fname;
        PT::UIntSet all;
        fill_set(all, 0, pano.getNrOfImages()-1);
        std::ofstream script(scriptName.GetFullPath().mb_str());
        pano.printStitcherScript(script, pano.getOptions(), all);
        script.close();
    }

}

void MainFrame::LoadProjectFile(const wxString & filename)
{
    DEBUG_TRACE("");
    m_filename = filename;
    // remove old images from cache
    // hmm probably not a good idea, if the project is reloaded..
    //ImageCache::getInstance().flush();

    wxFileName fname(filename);
    wxString path = fname.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR);

    SetStatusText( _("Open project:   ") + filename);

    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();
    std::ifstream file((const char *)filename.mb_str());
    if (file.good()) {
        wxBusyCursor();
        GlobalCmdHist::getInstance().addCommand(
            new wxLoadPTProjectCmd(pano,file, (const char *)path.mb_str())
            );
        DEBUG_DEBUG("project contains " << pano.getNrOfImages() << " after load");
        opt_panel->setOptimizeVector(pano.getOptimizeVector());
        SetStatusText(_("Project opened"));
        config->Write(wxT("actualPath"), path);  // remember for later
        this->SetTitle(fname.GetName() + wxT(".") + fname.GetExt() + wxT(" - hugin"));
        if (! (fname.GetExt() == wxT("pto"))) {
            // do not remember filename if its not a hugin project
            // to avoid overwriting the original project with an
            // incompatible one
            m_filename = wxT("");
        }
    } else {
        SetStatusText( _("Error opening project:   ") + filename);
        DEBUG_ERROR("Could not open file " << filename);
    }

    pano.clearDirty();
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
                     config->Read(wxT("actualPath"),wxT("")), wxT(""),
                     _("Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|All files (*.*)|*.*"),
                     wxOPEN, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        wxString filename = dlg.GetPath();
        LoadProjectFile(filename);
    } else {
        // do not close old project
        // nothing to open
        SetStatusText( _("Open project: cancel"));
    }
}

void MainFrame::OnNewProject(wxCommandEvent & e)
{
    m_filename = wxT("");
    GlobalCmdHist::getInstance().addCommand( new NewPanoCmd(pano));
    // remove old images from cache
    ImageCache::getInstance().flush();
    pano.clearDirty();
}

void MainFrame::OnAddImages( wxCommandEvent& event )
{
    DEBUG_TRACE("");
    wxString e_stat;

    // To get users path we do following:
    // get the global config object
    wxConfigBase* config = wxConfigBase::Get();

    wxString wildcard (_("Image files (*.jpg)|*.jpg;*.JPG|Image files (*.png)|*.png;*.PNG|Image files (*.tif)|*.tif;*.TIF|All files (*.*)|*.*"));
    wxFileDialog dlg(this,_("Add images"),
                     config->Read(wxT("actualPath"),wxT("")), wxT(""),
                     wildcard, wxOPEN|wxMULTIPLE , wxDefaultPosition);

    // remember the image extension
    wxString img_ext (wxT(""));
    if (config->HasEntry(wxT("lastImageType"))){
      img_ext = config->Read(wxT("lastImageType")).c_str();
    }
    if (img_ext == wxT("png"))
      dlg.SetFilterIndex(1);
    else if (img_ext == wxT("tif"))
      dlg.SetFilterIndex(2);
    else if (img_ext == wxT("all"))
      dlg.SetFilterIndex(3);
    DEBUG_INFO ( "Image extention: " << img_ext.mb_str() )

    // call the file dialog
    if (dlg.ShowModal() == wxID_OK) {
        // get the selections
        wxArrayString Pathnames;
        wxArrayString Filenames;
        dlg.GetPaths(Pathnames);
        dlg.GetFilenames(Filenames);

        // e safe the current path to config
        config->Write(wxT("actualPath"), dlg.GetDirectory());  // remember for later

        std::vector<std::string> filesv;
        for (unsigned int i=0; i< Pathnames.GetCount(); i++) {
            filesv.push_back((const char *)Pathnames[i].mb_str());
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

    DEBUG_INFO ( wxString::Format(wxT("img_ext: %d"), dlg.GetFilterIndex()).mb_str() )
    // save the image extension
    switch ( dlg.GetFilterIndex() ) {
      case 0: config->Write(wxT("lastImageType"), wxT("jpg")); break;
      case 1: config->Write(wxT("lastImageType"), wxT("png")); break;
      case 2: config->Write(wxT("lastImageType"), wxT("tif")); break;
      case 3: config->Write(wxT("lastImageType"), wxT("all")); break;
    }

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

WX_DECLARE_STRING_HASH_MAP(time_t, StringToPointerHash);
WX_DECLARE_STRING_HASH_MAP(int, StringToFlagHash);

struct sortbytime
{
    sortbytime(map<string, time_t> & h)
        : m_time(h)
    { }

    bool operator()(const std::string & s1, const std::string & s2)
    {
        time_t t1 = m_time[s1];
        time_t t2 = m_time[s2];
        return t1 < t2;
    }

    map<string, time_t> & m_time;
};


void MainFrame::OnAddTimeImages( wxCommandEvent& event )
{
    DEBUG_TRACE("");

    int maxtimediff = wxConfigBase::Get()->Read(wxT("CaptureTimeSpan"), HUGIN_CAPTURE_TIMESPAN);

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
    StringToPointerHash filenames;
    StringToFlagHash preloaded;
    while (images)
    {
        --images;

        // Get the filename.  Naively assumes there are no Unicode issues.
        const PanoImage& image = pano.getImage(images);
        std::string filename = image.getFilename();
        wxString file(filename.c_str(), *wxConvCurrent);
        preloaded[file] = 1;

        // Glob for all files of same type in same directory.
        wxString name(filename.c_str(), *wxConvCurrent);
        wxString path = ::wxPathOnly(name) + wxT("/*");
        file = ::wxFindFirstFile(path);
        while (!file.IsEmpty())
        {
            // Associated with a NULL dummy timestamp for now.
			filenames[file] = 0;
			file = ::wxFindNextFile();
        }
    }

    DEBUG_INFO("found " << filenames.size() <<
                " candidate files to search.");

    // For each globbed or loaded file,
    StringToPointerHash::iterator found;
    std::map<std::string, time_t> timeMap;
    for (found = filenames.begin(); found != filenames.end(); found++)
    {
        wxString file = found->first;
        // Check the time if it's got a camera EXIF timestamp.
		  time_t stamp = ReadJpegTime(file.mb_str());
      	  if (stamp) {
            filenames[file] = stamp;
            timeMap[(const char *)file.mb_str()] = stamp;
      	  }
    }

    //TODO: sorting the filenames keys by timestamp would be useful

    std::vector<std::string> filesv;
    int changed;
    do
    {
        changed = FALSE;

        // For each timestamped file,
        for (found = filenames.begin(); found != filenames.end(); found++)
        {
            wxString recruit = found->first;
			if (preloaded[recruit] == 1)
			  continue;
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
                wxString file(filename.c_str(), *wxConvCurrent);
                if (file == recruit)
                    continue;

                // If it is within threshold time,
                time_t stamp = filenames[file];
                if (abs(pledge - stamp) < maxtimediff)
                {
                    // Load this file, and remember it.
                    DEBUG_TRACE("Recruited " << recruit.mb_str());
                    std::string file = (const char *)recruit.mb_str();
                    filesv.push_back(file);
                    // Don't recruit it again.
                    filenames[recruit] = 0;
                    // Count this as a change.
                    changed++;
                    break;
                }
            }
        }
    } while (changed);

    // sort files by date
    sortbytime spred(timeMap);
    sort(filesv.begin(), filesv.end(), spred);

    // Load all of the named files.
    wxBusyCursor();
    GlobalCmdHist::getInstance().addCommand(
        new wxAddImagesCmd(pano,filesv)
        );
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
	wxString strFile;
	wxString langCode;
	
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("about_dlg"));

    //if the language is not default, load custom About file (if exists)
	langCode = huginApp::Get()->GetLocale().GetName().Left(2).Lower();
	DEBUG_INFO("Lang Code: " << langCode.mb_str());
	if(langCode != wxString(wxT("en")))
	{
		strFile = m_xrcPrefix + wxT("data/about_") + langCode + wxT(".htm");
		if(wxFile::Exists(strFile))
		{
			DEBUG_TRACE("Using About: " << strFile.mb_str());
  			XRCCTRL(dlg,"about_html",wxHtmlWindow)->LoadPage(strFile);
		}
	}
    dlg.ShowModal();
}

void MainFrame::OnHelp(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxDialog dlg;
	wxString strFile;
	wxString langCode;
	bool bHelpExists = false;

    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("help_dlg"));

    //if the language is not default, load custom FAQ file (if exists)
	langCode = huginApp::Get()->GetLocale().GetName().Left(2).Lower();
	DEBUG_TRACE("Lang Code: " << langCode.mb_str());
	if(langCode != wxString(wxT("en")))
	{
		strFile = m_xrcPrefix + wxT("data/manual_") + langCode + wxT(".html");
		if(wxFile::Exists(strFile))
			bHelpExists = true;
	}
	if(!bHelpExists)
		strFile = m_xrcPrefix + wxT("data/manual.html");  //load default file

	DEBUG_TRACE("Using help: " << strFile.mb_str());
    XRCCTRL(dlg,"help_html",wxHtmlWindow)->LoadPage(strFile);

    dlg.ShowModal();
}

void MainFrame::OnTipOfDay(wxCommandEvent& WXUNUSED(e))
{
	wxString strFile;
	wxString langCode;
	bool bShowAtStartup;
	bool bTipsExist = false;
	int nValue;

    wxConfigBase * config = wxConfigBase::Get();
	nValue = config->Read(wxT("/MainFrame/ShowStartTip"),1l);

    //if the language is not default, load custom tip file (if exists)
	langCode = huginApp::Get()->GetLocale().GetName().Left(2).Lower();
	DEBUG_INFO("Lang Code: " << langCode.mb_str());
	DEBUG_INFO("Tip index: " << nValue);
	if(langCode != wxString(wxT("en")))
	{
#ifdef UNICODE
		strFile = m_xrcPrefix + wxT("data/tips_") + langCode + wxT("-UTF8.txt");
#else
		strFile = m_xrcPrefix + wxT("data/tips_") + langCode + wxT(".txt");
#endif
		if(wxFile::Exists(strFile))
			bTipsExist = true;
	}
	if(!bTipsExist)
		strFile = m_xrcPrefix + wxT("data/tips.txt");  //load default file
	
	DEBUG_INFO("Reading tips from " << strFile.mb_str());
	wxTipProvider *tipProvider = wxCreateFileTipProvider(strFile, nValue);
	bShowAtStartup = wxShowTip(this, tipProvider);

	//store startup preferences
	nValue = (bShowAtStartup ? tipProvider->GetCurrentTip() : 0);
	DEBUG_INFO("Writing tip index: " << nValue);
    config->Write(wxT("/MainFrame/ShowStartTip"), nValue);
	delete tipProvider;
}

void MainFrame::OnKeyboardHelp(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxDialog dlg;
	wxString strFile;
	wxString langCode;
	bool bKBDExists = false;

    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("keyboard_help_dlg"));

    //if the language is not default, load custom FAQ file (if exists)
	langCode = huginApp::Get()->GetLocale().GetName().Left(2).Lower();
	DEBUG_INFO("Lang Code: " << langCode.mb_str());
	if(langCode != wxString(wxT("en")))
	{
		strFile = m_xrcPrefix + wxT("data/keyboard_") + langCode + wxT(".html");
		if(wxFile::Exists(strFile))
			bKBDExists = true;
	}
	if(!bKBDExists)
		strFile = m_xrcPrefix + wxT("data/keyboard.html");  //load default file

	DEBUG_INFO("Using keyboard help: " << strFile.mb_str());
    XRCCTRL(dlg,"keyboard_help_html",wxHtmlWindow)->LoadPage(strFile);

    dlg.ShowModal();
}

void MainFrame::OnFAQ(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxDialog dlg;
	wxString strFile;
	wxString langCode;
	bool bFAQExists = false;
	
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("help_dlg"));

    //if the language is not default, load custom FAQ file (if exists)
	langCode = huginApp::Get()->GetLocale().GetName().Left(2).Lower();
	DEBUG_INFO("Lang Code: " << langCode.mb_str());
	if(langCode != wxString(wxT("en")))
	{
		strFile = m_xrcPrefix + wxT("data/FAQ_") + langCode + wxT(".html");
		if(wxFile::Exists(strFile))
			bFAQExists = true;
	}
	if(!bFAQExists)
		strFile = m_xrcPrefix + wxT("data/FAQ.html");  //load default file

	DEBUG_TRACE("Using FAQ: " << strFile.mb_str());
    XRCCTRL(dlg,"help_html",wxHtmlWindow)->LoadPage(strFile);
    dlg.SetTitle(_("hugin - FAQ"));
    dlg.ShowModal();
}

void MainFrame::OnShowPrefs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
//    wxDialog dlg(this, -1, _("Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxDIALOG_MODAL);
    pref_dlg->Show(true);
}

void MainFrame::UpdatePanels( wxCommandEvent& WXUNUSED(event) )
{   // Maybe this can be invoced by the Panorama::Changed() or
    // something like this. So no everytime update would be needed.
    DEBUG_TRACE("");
}

void MainFrame::OnTogglePreviewFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (preview_frame->IsIconized()) {
        preview_frame->Iconize(false);
    }
    preview_frame->Show();
    preview_frame->Raise();
}

void MainFrame::OnToggleCPFrame(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (cp_frame->IsIconized()) {
        cp_frame->Iconize(false);
    }
    cp_frame->Show();
    cp_frame->Raise();
}


void MainFrame::OnOptimize(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxCommandEvent dummy;
    opt_panel->OnOptimizeButton(dummy);
}

void MainFrame::OnFineTuneAll(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // fine-tune all points

    CPVector cps = pano.getCtrlPoints();

    // create a map of all control points.
    std::set<unsigned int> unoptimized;
    for (unsigned int i=0; i < cps.size(); i++) {
        // create all control points.
        unoptimized.insert(i);
    }

    unsigned int nGood=0;
    unsigned int nBad=0;

    wxConfigBase *cfg = wxConfigBase::Get();
    bool rotatingFinetune = cfg->Read(wxT("/Finetune/RotationSearch"), HUGIN_FT_ROTATION_SEARCH) == 1;
    double startAngle=HUGIN_FT_ROTATION_START_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStartAngle"),&startAngle,HUGIN_FT_ROTATION_START_ANGLE);
    startAngle=DEG_TO_RAD(startAngle);
    double stopAngle=HUGIN_FT_ROTATION_STOP_ANGLE;
    cfg->Read(wxT("/Finetune/RotationStopAngle"),&stopAngle,HUGIN_FT_ROTATION_STOP_ANGLE);
    stopAngle=DEG_TO_RAD(stopAngle);
    int nSteps = cfg->Read(wxT("/Finetune/RotationSteps"), HUGIN_FT_ROTATION_STEPS);

    double corrThresh=HUGIN_FT_CORR_THRESHOLD;
    cfg->Read(wxT("/Finetune/CorrThreshold"), &corrThresh, HUGIN_FT_CORR_THRESHOLD);
    double curvThresh = HUGIN_FT_CURV_THRESHOLD;
    wxConfigBase::Get()->Read(wxT("/Finetune/CurvThreshold"),&curvThresh,
                              HUGIN_FT_CURV_THRESHOLD);

    {
    MyProgressDialog pdisp(_("Fine-tuning all points"), wxT(""), NULL, wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    pdisp.pushTask(ProgressTask((const char *)wxString(_("Finetuning")).mb_str(),"",1.0/unoptimized.size()));

    // do not process the control points in random order,
    // but walk from image to image, to reduce image reloading
    // in low mem situations.
    for (unsigned int imgNr = 0 ; imgNr < pano.getNrOfImages(); imgNr++) {
        std::set<unsigned int>::iterator it=unoptimized.begin();
        while (it != unoptimized.end()) {
            if (cps[*it].image1Nr == imgNr || cps[*it].image2Nr == imgNr) {
                pdisp.increase();
                if (cps[*it].mode == ControlPoint::X_Y) {
                    // finetune only normal points
                    DEBUG_DEBUG("fine tuning point: " << *it);
                    const vigra::BImage & templImg = ImageCache::getInstance().getPyramidImage(
                        pano.getImage(cps[*it].image1Nr).getFilename(),0);
                    const vigra::BImage & searchImg = ImageCache::getInstance().getPyramidImage(
                        pano.getImage(cps[*it].image2Nr).getFilename(),0);

                    // load parameters
                    long templWidth = wxConfigBase::Get()->Read(
                        wxT("/Finetune/TemplateSize"),HUGIN_FT_TEMPLATE_SIZE);
                    long sWidth = templWidth + wxConfigBase::Get()->Read(
                        wxT("/Finetune/LocalSearchWidth"),HUGIN_FT_LOCAL_SEARCH_WIDTH);
                    vigra_ext::CorrelationResult res;
                    vigra::Diff2D roundP1(roundi(cps[*it].x1), roundi(cps[*it].y1));
                    vigra::Diff2D roundP2(roundi(cps[*it].x2), roundi(cps[*it].y2));

                    if (rotatingFinetune) {
                        res = vigra_ext::PointFineTuneRotSearch(
                            templImg,
                            roundP1,
                            templWidth,
                            searchImg,
                            roundP2,
                            sWidth,
                            startAngle, stopAngle, nSteps
                            );

                    } else {
                        res = vigra_ext::PointFineTune(
                            templImg,
                            roundP1,
                            templWidth,
                            searchImg,
                            roundP2,
                            sWidth
                            );

                    }
                    // invert curvature. we always assume its a maxima, the curvature there is negative
                    // however, we allow the user to specify a positive threshold, so we need to
                    // invert it
                    res.curv.x = - res.curv.x;
                    res.curv.y = - res.curv.y;

                    if (res.maxi < corrThresh ||res.curv.x < curvThresh || res.curv.y < curvThresh  )
                    {
                        // Bad correlation result.
                        nBad++;
                        if (res.maxi >= corrThresh) {
                            cps[*it].error = 0;
                        }
                        cps[*it].error = res.maxi;
                        DEBUG_DEBUG("low correlation: " << res.maxi << " curv: " << res.curv);
                    } else {
                        nGood++;
                        // only update if a good correlation was found
                        cps[*it].x1 = roundP1.x;
                        cps[*it].y1 = roundP1.y;
                        cps[*it].x2 = res.maxpos.x;
                        cps[*it].y2 = res.maxpos.y;
                        cps[*it].error = res.maxi;
                    }
                }
                unsigned int rm = *it;
                it++;
                unoptimized.erase(rm);
            } else {
                it++;
            }
        }
    }
    }
    wxString result;
    result.Printf(_("%d points fine-tuned, %d points not updated due to low correlation\n\nHint: The errors of the fine-tuned points have been set to the, correlation coefficient\nProblematic point can be spotted (just after fine-tune, before optimizing)\nby an error <= %.3f.\nThe error of points without a well defined peak (typically in regions with uniform color)\nwill be set to 0\n\nUse the Control Point list (F3) to see all point of the current project\n"),
                  nGood, nBad, corrThresh);
    wxMessageBox(result, _("Fine-tune result"), wxOK);
    // set newly optimized points
    GlobalCmdHist::getInstance().addCommand(
        new UpdateCPsCmd(pano,cps)
        );
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

/** update the display */
void MainFrame::updateProgressDisplay()
{
    wxString msg;
    // build the message:
    for (std::vector<ProgressTask>::iterator it = tasks.begin();
                 it != tasks.end(); ++it)
    {
        wxString cMsg;
        if (it->getProgress() > 0) {
            cMsg.Printf(wxT("%s %s [%3.0f%%]"),
                        wxString(it->getShortMessage().c_str(), *wxConvCurrent).c_str(),
                        wxString(it->getMessage().c_str(), *wxConvCurrent).c_str(),
                        100 * it->getProgress());
        } else {
            cMsg.Printf(wxT("%s %s"),wxString(it->getShortMessage().c_str(), *wxConvCurrent).c_str(),
                        wxString(it->getMessage().c_str(), *wxConvCurrent).c_str());
        }
        // append to main message
        if (it == tasks.begin()) {
            msg = cMsg;
        } else {
            msg.Append(wxT(" | "));
            msg.Append(cMsg);
        }
    }
    wxStatusBar *m_statbar = GetStatusBar();
    DEBUG_TRACE("Statusmb : " << msg.mb_str());
    m_statbar->SetStatusText(msg,0);

    // This is a bad call.. we just want to repaint the window, instead we will
    // process user events as well :( Unfortunately, there is not portable workaround...
#ifdef __WXMSW__
    UpdateWindow(NULL);
#else
    wxYield();
#endif
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


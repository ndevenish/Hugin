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

#include "panoinc.h"

//Mac bundle code by Ippei
#ifdef __WXMAC__
#include <CFBundle.h>
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
        if (file.GetExt().CmpNoCase("pto") == 0 ||
            file.GetExt().CmpNoCase("ptp") == 0 ||
            file.GetExt().CmpNoCase("pts") == 0 )
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

        if (file.GetExt().CmpNoCase("jpg") == 0 ||
            file.GetExt().CmpNoCase("tif") == 0 ||
            file.GetExt().CmpNoCase("tiff") == 0 ||
            file.GetExt().CmpNoCase("png") == 0 ||
            file.GetExt().CmpNoCase("bmp") == 0 ||
            file.GetExt().CmpNoCase("gif") == 0 ||
            file.GetExt().CmpNoCase("pnm") == 0 ||
            file.GetExt().CmpNoCase("sun") == 0 ||
            file.GetExt().CmpNoCase("viff") == 0 )
        {
            filesv.push_back(filenames[i].c_str());
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
        m_xrcPrefix = splashPath + "/xrc/";
    } else if ( wxFile::Exists((wxString)wxT(INSTALL_XRC_DIR) + wxT("/data/splash.png")) ) {
        DEBUG_INFO("using installed xrc files");
        m_xrcPrefix = (wxString)wxT(INSTALL_XRC_DIR) + wxT("/");
    } else {
        DEBUG_INFO("using xrc prefix from config")
        m_xrcPrefix = config->Read("xrc_path") + wxT("/");
    }

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
            CFIndex bufLen = 1024;
            unsigned char buffer[1024];
            if(!CFURLGetFileSystemRepresentation(XRCurl, TRUE, buffer, bufLen))
            {
                CFRelease(XRCurl);
                DEBUG_INFO("Mac: Failed to get file system representation");
            }
            else
            {
                buffer[1023] = '\0';
                CFRelease(XRCurl);
                m_xrcPrefix = (wxString)buffer+ wxT("/");
                DEBUG_INFO("Mac: overriding xrc prefix; using mac bundled xrc files");
            }
        }
    }
    /* end: Mac bundle code by Ippei*/
#endif

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
                                    wxSPLASH_CENTRE_ON_SCREEN|wxSPLASH_NO_TIMEOUT,
                                    0, NULL, -1, wxDefaultPosition,
                                    wxDefaultSize,
                                    wxSIMPLE_BORDER);
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
#elif defined(__WXMAC__)
    //size
    int w = config->Read("MainFrame/width",-1l);
    int h = config->Read("MainFrame/height",-1l);
    if (w != -1) {
        SetClientSize(w,h);
    } else {
        Fit();
    }
    //position
    int x = config->Read("MainFrame/positionX",-1l);
    int y = config->Read("MainFrame/positionY",-1l);
    if ( y != -1) {
        Move(x, y);
    } else {
        Move(0, 44);
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

    if(splash) {
        splash->Close();
    }
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

#ifdef __WXMSW__
    wxRect sz = GetRect();
    config->Write("/MainFrame/width", sz.width);
    config->Write("/MainFrame/height", sz.height);

    config->Write("/MainFrame/positionX", sz.x);
    config->Write("/MainFrame/positionY", sz.y);
#elif defined(__WXMAC__)
    //size
    wxSize sz = GetClientSize();
    config->Write("/MainFrame/width", sz.GetWidth());
    config->Write("/MainFrame/height", sz.GetHeight());
    //position
    wxPoint ps = GetPosition();
    config->Write("/MainFrame/positionX", ps.x);
    config->Write("/MainFrame/positionY", ps.y);
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
    wxFileName scriptName = m_filename;
    if (m_filename == "") {
        OnSaveProjectAs(e);
    } else {
        // the project file is just a PTOptimizer script...
        std::string path(
            scriptName.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR));
        DEBUG_DEBUG("stripping " << path << " from image filenames");
        std::ofstream script(scriptName.GetFullPath());
        PT::OptimizeVector optvec = opt_panel->getOptimizeVector();
        pano.printOptimizerScript(script, optvec, pano.getOptions(), path);
        script.close();
    }
    SetStatusText(wxString::Format(_("saved project %s"), m_filename.c_str()),0);
    this->SetTitle(scriptName.GetName() + "." + scriptName.GetExt() + " - hugin");
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

void MainFrame::OnSavePTStitcherAs(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxFileDialog dlg(this,
                     _("Save PTSticher script file"),
                     wxConfigBase::Get()->Read("actualPath",""), "",
                     "PTSticher files (*.txt)|*.txt",
                     wxSAVE, wxDefaultPosition);
    if (dlg.ShowModal() == wxID_OK) {
        wxString fname = dlg.GetPath();
        // the project file is just a PTSticher script...
        wxFileName scriptName = fname;
        std::ofstream script(scriptName.GetFullPath());
        pano.printStitcherScript(script, pano.getOptions());
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
    std::ifstream file(filename.c_str());
    if (file.good()) {
        wxBusyCursor();
        GlobalCmdHist::getInstance().addCommand(
            new wxLoadPTProjectCmd(pano,file, path.c_str())
            );
        DEBUG_DEBUG("project contains " << pano.getNrOfImages() << " after load");
        opt_panel->setOptimizeVector(pano.getOptimizeVector());
        SetStatusText(_("Project opened"));
        config->Write("actualPath", path);  // remember for later
        this->SetTitle(fname.GetName() + "." + fname.GetExt() + " - hugin");
        if (! (fname.GetExt() == "pto")) {
            // do not remember filename if its not a hugin project
            // to avoid overwriting the original project with an
            // incompatible one
            m_filename = "";
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
                     config->Read("actualPath",""), "",
                     "Project files (*.pto,*.ptp,*.pts,*.oto)|*.pto;*.ptp;*.pts;*.oto;|"
                     "All files (*.*)|*.*",
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
    m_filename = "";
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

    wxString wildcard ("Image files (*.jpg)|*.jpg;*.JPG|"
                       "Image files (*.png)|*.png;*.PNG|"
                       "Image files (*.tif)|*.tif;*.TIF|"
                       "All files (*.*)|*.*");
    wxFileDialog dlg(this,_("Add images"),
                     config->Read("actualPath",""), "",
                     wildcard, wxOPEN|wxMULTIPLE , wxDefaultPosition);

    // remember the image extension
    wxString img_ext ("");
    if (config->HasEntry(wxT("lastImageType"))){
      img_ext = config->Read("lastImageType").c_str();
    }
    if (img_ext == "png")
      dlg.SetFilterIndex(1);
    else if (img_ext == "tif")
      dlg.SetFilterIndex(2);
    else if (img_ext == "all")
      dlg.SetFilterIndex(3);
    DEBUG_TRACE ( img_ext )

    // call the file dialog
    if (dlg.ShowModal() == wxID_OK) {
        // get the selections
        wxArrayString Pathnames;
        wxArrayString Filenames;
        dlg.GetPaths(Pathnames);
        dlg.GetFilenames(Filenames);

        // e safe the current path to config
        config->Write("actualPath", dlg.GetDirectory());  // remember for later

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

    DEBUG_TRACE ( wxString::Format("img_ext: %d", dlg.GetFilterIndex()) )
    // save the image extension
    switch ( dlg.GetFilterIndex() ) {
      case 0: config->Write("lastImageType", "jpg"); break;
      case 1: config->Write("lastImageType", "png"); break;
      case 2: config->Write("lastImageType", "tif"); break;
      case 3: config->Write("lastImageType", "all"); break;
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

    int maxtimediff = wxConfigBase::Get()->Read("CaptureTimeSpan", HUGIN_CAPTURE_TIMESPAN);

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
    std::map<std::string, time_t> timeMap;
    for (found = filenames.begin(); found != filenames.end(); found++)
    {
        wxString file = found->first;
        // Check the time if it's got a camera EXIF timestamp.
        time_t stamp = ReadJpegTime(file.c_str());
        if (stamp) {
            filenames[file] = stamp;
            timeMap[file.c_str()] = stamp;
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
                time_t stamp = filenames[file];
                if (abs(pledge - stamp) < maxtimediff)
                {
                    // Load this file, and remember it.
                    DEBUG_TRACE("Recruited " << recruit.c_str());
                    std::string file = recruit.c_str();
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

void MainFrame::OnKeyboardHelp(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("keyboard_help_dlg"));
    dlg.ShowModal();
}

void MainFrame::OnFAQ(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    wxDialog dlg;
    wxXmlResource::Get()->LoadDialog(&dlg, this, wxT("help_dlg"));
    XRCCTRL(dlg,"help_html",wxHtmlWindow)->LoadPage(m_xrcPrefix + "/data/FAQ.html");
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
    bool rotatingFinetune = cfg->Read("/Finetune/RotationSearch", HUGIN_FT_ROTATION_SEARCH) == 1;
    double startAngle=HUGIN_FT_ROTATION_START_ANGLE;
    cfg->Read("/Finetune/RotationStartAngle",&startAngle,HUGIN_FT_ROTATION_START_ANGLE);
    startAngle=DEG_TO_RAD(startAngle);
    double stopAngle=HUGIN_FT_ROTATION_STOP_ANGLE;
    cfg->Read("/Finetune/RotationStopAngle",&stopAngle,HUGIN_FT_ROTATION_STOP_ANGLE);
    stopAngle=DEG_TO_RAD(stopAngle);
    int nSteps = cfg->Read("/Finetune/RotationSteps", HUGIN_FT_ROTATION_STEPS);

    double corrThresh=HUGIN_FT_CORR_THRESHOLD;
    cfg->Read("/Finetune/CorrThreshold", &corrThresh, HUGIN_FT_CORR_THRESHOLD);
    double curvThresh = HUGIN_FT_CURV_THRESHOLD;
    wxConfigBase::Get()->Read("/Finetune/CurvThreshold",&curvThresh,
                              HUGIN_FT_CURV_THRESHOLD);

    {
    MyProgressDialog pdisp(_("Fine-tuning all points"), "", NULL, wxPD_ELAPSED_TIME | wxPD_AUTO_HIDE | wxPD_APP_MODAL );

    pdisp.pushTask(ProgressTask("Finetuning","",1.0/unoptimized.size()));

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
                        "/Finetune/TemplateSize",HUGIN_FT_TEMPLATE_SIZE);
                    long sWidth = templWidth + wxConfigBase::Get()->Read(
                        "/Finetune/LocalSearchWidth",HUGIN_FT_LOCAL_SEARCH_WIDTH);
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
    result.Printf(_("%d points fine-tuned, %d points not updated due to low correlation\n\n"
                    "Hint: The errors of the fine-tuned points have been set to the, correlation coefficient\n"
                    "Problematic point can be spotted (just after fine-tune, before optimizing)\n"
                    "by an error <= %.3f.\n"
                    "The error of points without a well defined peak (typically in regions with uniform color)\n"
                    "will be set to 0\n\n"
                    "Use the Control Point list (F3) to see all point of the current project\n"),
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
            cMsg.Printf("%s %s [%3.0f%%]",
                        it->getShortMessage().c_str(),
                        it->getMessage().c_str(),
                        100 * it->getProgress());
        } else {
            cMsg.Printf("%s %s",it->getShortMessage().c_str(),
                        it->getMessage().c_str());
        }
        // append to main message
        if (it == tasks.begin()) {
            msg = cMsg;
        } else {
            msg.Append(" | ");
            msg.Append(cMsg);
        }
    }
    SetStatusText(msg,0);
//    wxYield();
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


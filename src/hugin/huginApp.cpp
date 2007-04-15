// -*- c-basic-offset: 4 -*-

/** @file huginApp.cpp
 *
 *  @brief implementation of huginApp Class
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


#include "hugin/config_defaults.h"
#include "hugin/huginApp.h"
#include "hugin/PanoPanel.h"
#include "hugin/PTWXDlg.h"
#include "hugin/CommandHistory.h"
#include "hugin/wxPanoCommand.h"


#include <tiffio.h>

using namespace utils;

// utility functions
bool str2double(wxString s, double & d)
{
    if (!utils::stringToDouble(std::string(s.mb_str()), d)) {
        wxLogError(_("Value must be numeric."));
        return false;
    }
    return true;
}

wxString getDefaultProjectName(const Panorama & pano)
{
    if (pano.getNrOfImages() > 0) {
        
        wxString first_img(stripExtension(stripPath(pano.getImage(0).getFilename())).c_str(), *wxConvCurrent);
        wxString last_img(stripExtension(stripPath(pano.getImage(pano.getNrOfImages()-1).getFilename())).c_str(), *wxConvCurrent);
        return first_img + wxT("-") + last_img;
    } else {
        return wxString(wxT("pano"));
    }
}

#ifdef __WXMAC__
wxString MacGetPathTOBundledResourceFile(CFStringRef filename)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef XRCurl = CFBundleCopyResourceURL(mainbundle, filename, NULL, NULL);
        if(XRCurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the file in bundle.");
        }
        else
        {
            CFStringRef pathInCFString = CFURLCopyFileSystemPath(XRCurl, kCFURLPOSIXPathStyle);
            if(pathInCFString == NULL)
            {
                DEBUG_INFO("Mac: Failed to get URL in CFString");
            }
            else
            {
                CFRetain( pathInCFString );
                theResult = wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                DEBUG_INFO("Mac: returned the resource file's path in the application bundle");
            }
            CFRelease( XRCurl );
        }
    }
    return theResult;
}

wxString MacGetPathTOBundledExecutableFile(CFStringRef filename)
{
    wxString theResult = wxT("");

    CFBundleRef mainbundle = CFBundleGetMainBundle();
    if(mainbundle == NULL)
    {
        DEBUG_INFO("Mac: Not bundled");
    }
    else
    {
        CFURLRef PTOurl = CFBundleCopyAuxiliaryExecutableURL(mainbundle, filename);
        if(PTOurl == NULL)
        {
            DEBUG_INFO("Mac: Cannot locate the file in the bundle.");
        }
        else
        {
            CFURLRef PTOAbsURL = CFURLCopyAbsoluteURL( PTOurl );
            if(PTOAbsURL == NULL)
            {
                DEBUG_INFO("Mac: Cannot convert the file path to absolute");
            }
            else
            {
                CFStringRef pathInCFString = CFURLCopyFileSystemPath(PTOAbsURL, kCFURLPOSIXPathStyle);
                if(pathInCFString == NULL)
                {
                    DEBUG_INFO("Mac: Failed to get URL in CFString");
                }
                else
                {
                    CFRetain( pathInCFString );
                    theResult =  wxMacCFStringHolder(pathInCFString).AsString(wxLocale::GetSystemEncoding());
                    DEBUG_INFO("Mac: returned the executable's full path in the application bundle");
                }
                CFRelease( PTOAbsURL );
            }
            CFRelease( PTOurl );
        }
    }
    return theResult;
}
#endif


// make wxwindows use this class as the main application
IMPLEMENT_APP(huginApp)

huginApp::huginApp()
{
    DEBUG_TRACE("ctor");
    m_this=this;
}

huginApp::~huginApp()
{
    DEBUG_TRACE("dtor");
    // delete temporary dir
//    if (!wxRmdir(m_workDir)) {
//        DEBUG_ERROR("Could not remove temporary directory");
//    }

	// todo: remove all listeners from the panorama object

//    delete frame;
    DEBUG_TRACE("dtor end");
}

bool huginApp::OnInit()
{
    DEBUG_TRACE("=========================== huginApp::OnInit() begin ===================");
    SetAppName(wxT("hugin"));

    // register our custom pano tools dialog handlers
    registerPTWXDlgFcn();

    // required by wxHtmlHelpController
    wxFileSystem::AddHandler(new wxZipFSHandler);

    wxString m_huginPath;
    wxFileName::SplitPath( argv[0], &m_huginPath, NULL, NULL );

    // DEBUG_INFO( GetAppName().c_str() )
    DEBUG_INFO( wxFileName::GetCwd().c_str() )
    // DEBUG_INFO( wxFileName::GetHomeDir().c_str() )
    DEBUG_INFO( "hugin path:" << m_huginPath.mb_str() )


    // here goes and comes configuration
    wxConfigBase * config = wxConfigBase::Get();

    config->SetRecordDefaults(TRUE);

    if ( config->IsRecordingDefaults() ) {
      char e_dbg[128] = "writes in config: ";
      sprintf ( e_dbg ,"%s %d\n", e_dbg, (int) config->GetNumberOfEntries() );
      DEBUG_INFO(e_dbg);
    }
    config->Flush();

    // initialize i18n
    int localeID = config->Read(wxT("language"), (long) HUGIN_LANGUAGE);
	DEBUG_TRACE("localeID: " << localeID);
    {
	bool bLInit;
	bLInit = locale.Init(localeID);
	if (bLInit) {
	  DEBUG_TRACE("locale init OK");
	  DEBUG_TRACE("System Locale: " << locale.GetSysName().mb_str())
	  DEBUG_TRACE("Canonical Locale: " << locale.GetCanonicalName().mb_str())
	} else {
	  DEBUG_TRACE("locale init failed");
	}
	}
	

    // add local Paths
    locale.AddCatalogLookupPathPrefix(m_huginPath + wxT("/locale"));
#if defined __WXMSW__
    locale.AddCatalogLookupPathPrefix(wxT("./locale"));
#elif defined __WXMAC__
    wxString thePath = MacGetPathTOBundledResourceFile(CFSTR("locale"));
    if(thePath != wxT(""))
        locale.AddCatalogLookupPathPrefix(thePath);
#else
    locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
    DEBUG_INFO("add locale path: " << INSTALL_LOCALE_DIR)
#endif
    // add path from config file
    if (config->HasEntry(wxT("locale_path"))){
        locale.AddCatalogLookupPathPrefix(  config->Read(wxT("locale_path")).c_str() );
    }


    // set the name of locale recource to look for
    locale.AddCatalog(wxT("hugin"));

    // initialize image handlers
    wxInitAllImageHandlers();

    // Initialize all the XRC handlers.
    wxXmlResource::Get()->InitAllHandlers();

    // load all XRC files.
#ifdef _INCLUDE_UI_RESOURCES
    InitXmlResource();
#else

#ifdef __WXMAC__
    wxString osxPath = MacGetPathTOBundledResourceFile(CFSTR("xrc"));
#endif

    // try local xrc files first
    // testing for xrc file location
    if ( wxFile::Exists(m_huginPath + wxT("/xrc/main_frame.xrc")) ) {
        DEBUG_INFO("using local xrc files");
        wxFileName f(m_huginPath);
        if (! f.IsAbsolute()) {
            wxString currentDir = wxFileName::GetCwd();
            wxString t = currentDir + wxT("/") + m_huginPath + wxT("/xrc/");
            if (wxFile::Exists(t + wxT("/xrc/main_frame.xrc"))) { 
                m_huginPath = currentDir + wxT("/") + m_huginPath;
            }
        }
        m_xrcPrefix = m_huginPath + wxT("/xrc/");
#ifdef __WXMAC__
    } else if ( wxFile::Exists(osxPath + wxT("/main_frame.xrc")) ) {
        m_xrcPrefix = osxPath + wxT("/");
#endif
#ifdef __WXGTK__
    } else if ( wxFile::Exists((wxString)wxT(INSTALL_XRC_DIR) + wxT("/main_frame.xrc")) ) {
        DEBUG_INFO("using installed xrc files");
        m_xrcPrefix = (wxString)wxT(INSTALL_XRC_DIR) + wxT("/");
#endif
    } else if (config->HasEntry(wxT("xrc_path")) && 
        wxFile::Exists(config->Read(wxT("xrc_path")) + wxT("/main_frame.xrc")) )
    {
        DEBUG_INFO("using xrc prefix from config");
        m_xrcPrefix = config->Read(wxT("xrc_path")) + wxT("/");
    } else {
        std::cerr << "FATAL error: Could not find data directory, exiting\nTo manually specify the xrc directory open ~/.hugin and add the following\nto the top of the file:\nxrc_path=/my/install/prefix/share/hugin/xrc\n";
        wxMessageBox(wxT("FATAL error: Could not find data directory, exiting\nTo manually specify the xrc directory open ~/.hugin and add the following\nto the top of the file:\nxrc_path=/my/install/prefix/share/hugin/xrc"), wxT("Fatal error"), wxOK | wxICON_ERROR);
        
        return false;
    }

    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("crop_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("nona_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("ptstitcher_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cp_list_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("preview_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("run_optimizer_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("edit_script_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("run_stitcher_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("anchor_orientation.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_menu.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_tool.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("edit_text.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("about.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("help.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("keyboard_help.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("pref_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("vig_corr_dlg.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("optimize_photo_panel.xrc"));
#ifdef USE_WX253
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cp_editor_panel-2.5.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("assistant_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("images_panel-2.5.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("lens_panel-2.5.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_frame-2.5.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("optimize_panel-2.5.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("pano_panel-2.5.xrc"));
#else
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cp_editor_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("images_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("lens_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("optimize_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("pano_panel.xrc"));
#endif

#endif

#ifdef __WXMAC__
    // If hugin is starting with file opening AppleEvent, MacOpenFile will be called on first wxYield().
    // Those need to be initialised before first call of Yield which happens in Mainframe constructor.
    m_macInitDone=false;
    m_macOpenFileOnStart=false;
#endif
    // create main frame
    frame = new MainFrame(NULL, pano);
    SetTopWindow(frame);

    // restore layout
    frame->RestoreLayoutOnNextResize();

    // setup main frame size, after it has been created.
    RestoreFramePosition(frame, wxT("MainFrame"));
    
    // show the frame.
    frame->Show(TRUE);

    wxString cwd = wxFileName::GetCwd();
    config->Write( wxT("startDir"), cwd );

    m_workDir = config->Read(wxT("tempDir"),wxT(""));
    // FIXME, make secure against some symlink attacks
    // get a temp dir
#ifdef __WXMSW__
    DEBUG_DEBUG("figuring out windows temp dir");
    if (m_workDir == wxT("")) {
        /* added by Yili Zhao */
        wxChar buffer[MAX_PATH];
        GetTempPath(MAX_PATH, buffer);
        m_workDir = buffer;
    }
#else
    DEBUG_DEBUG("on unix or mac");
    if (m_workDir == wxT("")) {
        // try to read environment variable
        if (!wxGetEnv(wxT("TMPDIR"), &m_workDir)) {
            // still no tempdir, use /tmp
            m_workDir = wxT("/tmp");
        }
    }
#endif

    if (!wxFileName::DirExists(m_workDir)) {
        DEBUG_DEBUG("creating temp dir: " << m_workDir.mb_str());
        if (!wxMkdir(m_workDir)) {
            DEBUG_ERROR("Tempdir could not be created: " << m_workDir.mb_str());
        }
    }
    if (!wxSetWorkingDirectory(m_workDir)) {
        DEBUG_ERROR("could not change to temp. dir: " << m_workDir.mb_str());
    }
    DEBUG_DEBUG("using temp dir: " << m_workDir.mb_str());

    // set some suitable defaults
    PanoramaOptions opts = pano.getOptions();
    opts.outputFormat = PanoramaOptions::TIFF;
    opts.blendMode = PanoramaOptions::ENBLEND_BLEND;
    pano.setOptions(opts);

    if (argc > 1) {
        wxFileName file(argv[1]);
        // if the first file is a project file, open it
        if (file.GetExt().CmpNoCase(wxT("pto")) == 0 ||
            file.GetExt().CmpNoCase(wxT("pts")) == 0 ||
            file.GetExt().CmpNoCase(wxT("ptp")) == 0 )
        {
            wxString filename(argv[1]);
            if (! wxIsAbsolutePath(filename)) {
                filename.Prepend(wxFileName::GetPathSeparator());
                filename.Prepend(cwd);
            }
            frame->LoadProjectFile(filename);
        } else {
            std::vector<std::string> filesv;
            for (int i=1; i< argc; i++) {
                wxFileName file(argv[i]);
                if (file.GetExt().CmpNoCase(wxT("jpg")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("jpeg")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("tif")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("tiff")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("png")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("bmp")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("gif")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("pnm")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("sun")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("hdr")) == 0 ||
                    file.GetExt().CmpNoCase(wxT("viff")) == 0 )
                {
                    filesv.push_back((const char *)(file.GetFullPath().mb_str()));
                }
            }
            GlobalCmdHist::getInstance().addCommand(
                    new PT::wxAddImagesCmd(pano,filesv)
                                                );
        }
    }
#ifdef __WXMAC__
    m_macInitDone = true;
    if(m_macOpenFileOnStart) {frame->LoadProjectFile(m_macFileNameToOpenOnStart);}
    m_macOpenFileOnStart = false;
#endif

    //load tip startup preferences (tips will be started after splash terminates)
	int nValue = config->Read(wxT("/MainFrame/ShowStartTip"), 1l);
		
	//show tips if needed now
	if(nValue > 0)
	{
		wxCommandEvent dummy;
		frame->OnTipOfDay(dummy);
	}

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    pano.changeFinished();
    pano.clearDirty();

    DEBUG_TRACE("=========================== huginApp::OnInit() end ===================");
    return true;
}

int huginApp::OnExit()
{
    DEBUG_TRACE("");
    return wxApp::OnExit();
}

huginApp * huginApp::Get()
{
    if (m_this) {
        return m_this;
    } else {
        DEBUG_FATAL("huginApp not yet created");
        DEBUG_ASSERT(m_this);
        return 0;
    }
}

#ifdef __WXMAC__
void huginApp::MacOpenFile(const wxString &fileName)
{
    if(!m_macInitDone)
    {
        m_macOpenFileOnStart=true;
        m_macFileNameToOpenOnStart = fileName;
        return;
    }

    if(frame) frame->MacOnOpenFile(fileName);
}
#endif

huginApp * huginApp::m_this = 0;


// utility functions

void RestoreFramePosition(wxTopLevelWindow * frame, const wxString & basename)
{
    DEBUG_TRACE(basename.mb_str());

    wxConfigBase * config = wxConfigBase::Get();

#if ( __WXGTK__ ) &&  wxCHECK_VERSION(2,6,0)
// restoring the splitter positions properly when maximising doesn't work.
// Disabling maximise on wxWidgets >= 2.6.0 and gtk
        //size
        int w = config->Read(wxT("/") + basename + wxT("/width"),-1l);
        int h = config->Read(wxT("/") + basename + wxT("/height"),-1l);
        if (w >0) {
            frame->SetClientSize(w,h);
        } else {
            frame->Fit();
        }
        //position
        int x = config->Read(wxT("/") + basename + wxT("/positionX"),-1l);
        int y = config->Read(wxT("/") + basename + wxT("/positionY"),-1l);
        if ( y >= 0 && x >= 0 && x < 4000 && y < 4000) {
            frame->Move(x, y);
        } else {
            frame->Move(0, 44);
        }
#else
    bool maximized = config->Read(wxT("/") + basename + wxT("/maximized"), 0l) != 0;
    if (maximized) {
        frame->Maximize();
    } else {
        //size
        int w = config->Read(wxT("/") + basename + wxT("/width"),-1l);
        int h = config->Read(wxT("/") + basename + wxT("/height"),-1l);
        if (w >0) {
            frame->SetClientSize(w,h);
        } else {
            frame->Fit();
        }
        //position
        int x = config->Read(wxT("/") + basename + wxT("/positionX"),-1l);
        int y = config->Read(wxT("/") + basename + wxT("/positionY"),-1l);
        if ( y >= 0 && x >= 0) {
            frame->Move(x, y);
        } else {
            frame->Move(0, 44);
        }
    }
#endif
}


void StoreFramePosition(wxTopLevelWindow * frame, const wxString & basename)
{
    DEBUG_TRACE(basename);

    wxConfigBase * config = wxConfigBase::Get();

#if ( __WXGTK__ ) &&  wxCHECK_VERSION(2,6,0)
// restoring the splitter positions properly when maximising doesn't work.
// Disabling maximise on wxWidgets >= 2.6.0 and gtk
    
        wxSize sz = frame->GetClientSize();
        config->Write(wxT("/") + basename + wxT("/width"), sz.GetWidth());
        config->Write(wxT("/") + basename + wxT("/height"), sz.GetHeight());
        wxPoint ps = frame->GetPosition();
        config->Write(wxT("/") + basename + wxT("/positionX"), ps.x);
        config->Write(wxT("/") + basename + wxT("/positionY"), ps.y);
        config->Write(wxT("/") + basename + wxT("/maximized"), 0);
#else
    if ( (! frame->IsMaximized()) && (! frame->IsIconized()) ) {
        wxSize sz = frame->GetClientSize();
        config->Write(wxT("/") + basename + wxT("/width"), sz.GetWidth());
        config->Write(wxT("/") + basename + wxT("/height"), sz.GetHeight());
        wxPoint ps = frame->GetPosition();
        config->Write(wxT("/") + basename + wxT("/positionX"), ps.x);
        config->Write(wxT("/") + basename + wxT("/positionY"), ps.y);
        config->Write(wxT("/") + basename + wxT("/maximized"), 0);
    } else if (frame->IsMaximized()){
        config->Write(wxT("/") + basename + wxT("/maximized"), 1l);
    }
#endif
}

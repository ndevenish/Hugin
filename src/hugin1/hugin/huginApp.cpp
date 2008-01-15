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
#include "base_wx/PTWXDlg.h"
#include "hugin/CommandHistory.h"
#include "hugin/wxPanoCommand.h"

#include "base_wx/platform.h"
#include "base_wx/huginConfig.h"


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


#if defined __WXMSW__
    wxString huginExeDir = getExePath(argv[0]);

    wxString huginRoot;
    wxFileName::SplitPath( huginExeDir, &huginRoot, NULL, NULL );

    m_xrcPrefix = huginRoot + wxT("/share/hugin/xrc/");
    m_utilsBinDir = huginRoot + wxT("/bin/");

    // locale setup
    locale.AddCatalogLookupPathPrefix(huginRoot + wxT("/share/locale"));

#elif defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    // initialize paths
    {
        wxString thePath = MacGetPathTOBundledResourceFile(CFSTR("xrc"));
        if (thePath == wxT("")) {
            wxMessageBox(_("xrc directory not found in bundle"), _("Fatal Error"));
            return false;
        }
        m_xrcPrefix = thePath + wxT("/");
    }

    {
        wxString thePath = MacGetPathTOBundledResourceFile(CFSTR("locale"));
        if(thePath != wxT(""))
            locale.AddCatalogLookupPathPrefix(thePath);
        else {
            wxMessageBox(_("Translations not found in bundle"), _("Fatal Error"));
            return false;
        }
    }

#else
    // add the locale directory specified during configure
    m_xrcPrefix = wxT(INSTALL_XRC_DIR);
    locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
#endif

    if ( ! wxFile::Exists(m_xrcPrefix + wxT("/main_frame.xrc")) ) {
        wxMessageBox(_("xrc directory not found, hugin needs to be properly installed\nTried Path:" + m_xrcPrefix ), _("Fatal Error"));
        return false;
    }

    // here goes and comes configuration
    wxConfigBase * config = wxConfigBase::Get();

    config->SetRecordDefaults(TRUE);

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


    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("crop_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("nona_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("ptstitcher_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cp_list_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("preview_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("run_optimizer_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("edit_script_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("run_stitcher_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_menu.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_tool.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("edit_text.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("about.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("help.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("keyboard_help.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("pref_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("vig_corr_dlg.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("optimize_photo_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cp_editor_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("images_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("lens_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("assistant_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("optimize_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("pano_panel.xrc"));
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
    if (m_workDir == wxT("")) {
#if (defined __WXMSW__)
        DEBUG_DEBUG("figuring out windows temp dir");
        /* added by Yili Zhao */
        wxChar buffer[MAX_PATH];
        GetTempPath(MAX_PATH, buffer);
        m_workDir = buffer;
#elif (defined __WXMAC__)
        DEBUG_DEBUG("temp dir on Mac");
        m_workDir = MacGetPathTOUserDomainTempDir();
        if(m_workDir == wxT(""))
            m_workDir = wxT("/tmp");
#else //UNIX
        DEBUG_DEBUG("temp dir on unix");
        // try to read environment variable
        if (!wxGetEnv(wxT("TMPDIR"), &m_workDir)) {
            // still no tempdir, use /tmp
            m_workDir = wxT("/tmp");
        }
#endif
        
    }

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

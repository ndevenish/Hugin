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

#ifdef __WXMAC__
#include <wx/sysopt.h>
#include <wx/dir.h>
#endif

#include "panoinc.h"


#include "hugin/config_defaults.h"
#include "hugin/huginApp.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MaskEditorPanel.h"
#include "hugin/CPEditorPanel.h"
#include "hugin/OptimizePhotometricPanel.h"
#include "hugin/PanoPanel.h"
#include "hugin/ImagesList.h"
#include "hugin/PreviewPanel.h"
#include "hugin/GLPreviewFrame.h"
#include "base_wx/PTWXDlg.h"
#include "hugin/CommandHistory.h"
#include "hugin/wxPanoCommand.h"
#include "hugin/HtmlWindow.h"
#include "hugin/treelistctrl.h"
#include "hugin/ImagesTree.h"

#include "base_wx/platform.h"
#include "base_wx/huginConfig.h"
#ifdef __WXMSW__
#include "wx/dir.h"
#include "wx/cshelp.h"
#endif

#include <tiffio.h>

#include "AboutDialog.h"

#include <hugin_version.h>
//for natural sorting
#include "hugin_utils/alphanum.h"
#include "lensdb/LensDB.h"

bool checkVersion(wxString v1, wxString v2)
{
    return doj::alphanum_comp(std::string(v1.mb_str(wxConvLocal)),std::string(v2.mb_str(wxConvLocal))) < 0;
};

using namespace hugin_utils;

// utility functions
bool str2double(wxString s, double & d)
{
    if (!stringToDouble(std::string(s.mb_str(wxConvLocal)), d)) {
        wxLogError(_("Value must be numeric."));
        return false;
    }
    return true;
}

wxString Components2Str(const CPComponents & comp)
{
    wxString ret;
    for (unsigned i=0; i < comp.size(); i++) {
        ret = ret + wxT("[");
        CPComponents::value_type::const_iterator it;
        size_t c=0;
        for (it = comp[i].begin();
            it != comp[i].end();
            ++it) 
        {
            ret = ret + wxString::Format(wxT("%d"), (*it));
            if (c+1 != comp[i].size()) {
                ret = ret + wxT(", ");
            }
            c++;
        }
        if (i+1 != comp.size())
            ret = ret + wxT("], ");
        else
            ret = ret + wxT("]");
    }
    return ret;
}

#if _WINDOWS && defined Hugin_shared
DEFINE_LOCAL_EVENT_TYPE( EVT_IMAGE_READY )
#else
DEFINE_EVENT_TYPE( EVT_IMAGE_READY )
#endif

BEGIN_EVENT_TABLE(huginApp, wxApp)
    EVT_IMAGE_READY2(-1, huginApp::relayImageLoaded)
END_EVENT_TABLE()

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
    HuginBase::LensDB::LensDB::Clean();
    DEBUG_TRACE("dtor end");
}

bool huginApp::OnInit()
{
    DEBUG_TRACE("=========================== huginApp::OnInit() begin ===================");
    SetAppName(wxT("hugin"));
    
    // Connect to ImageCache: we need to tell it when it is safe to handle UI events.
    ImageCache::getInstance().asyncLoadCompleteSignal = &huginApp::imageLoadedAsync;

#ifdef __WXMAC__
    // do not use the native list control on OSX (it is very slow with the control point list window)
    wxSystemOptions::SetOption(wxT("mac.listctrl.always_use_generic"), 1);
#endif

    // register our custom pano tools dialog handlers
    registerPTWXDlgFcn();

    // required by wxHtmlHelpController
    wxFileSystem::AddHandler(new wxZipFSHandler);


#if defined __WXMSW__
    // initialize help provider
    wxHelpControllerHelpProvider* provider = new wxHelpControllerHelpProvider;
    wxHelpProvider::Set(provider);

    wxString huginExeDir = getExePath(argv[0]);

    wxString huginRoot;
    wxFileName::SplitPath( huginExeDir, &huginRoot, NULL, NULL );

    m_xrcPrefix = huginRoot + wxT("/share/hugin/xrc/");
	m_DataDir = huginRoot + wxT("/share/hugin/data/");
    m_utilsBinDir = huginRoot + wxT("/bin/");

    // locale setup
    locale.AddCatalogLookupPathPrefix(huginRoot + wxT("/share/locale"));
    // lensfun database init
    wxString lensfunDBPath=huginRoot + wxT("/share/lensfun");
    HuginBase::LensDB::LensDB::GetSingleton().SetMainDBPath(std::string(lensfunDBPath.mb_str(HUGIN_CONV_FILENAME)));

#elif defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    // initialize paths
    {
        wxString thePath = MacGetPathToBundledResourceFile(CFSTR("xrc"));
        if (thePath == wxT("")) {
            wxMessageBox(_("xrc directory not found in bundle"), _("Fatal Error"));
            return false;
        }
        m_xrcPrefix = thePath + wxT("/");
        m_DataDir = thePath + wxT("/");
    }

#ifdef HUGIN_HSI
    // Set PYTHONHOME for the hsi module
    {
        wxString pythonHome = MacGetPathToBundledFrameworksDirectory() + wxT("/Python27.framework/Versions/Current");
        if(! wxDir::Exists(pythonHome)){
            wxMessageBox(wxString::Format(_("Directory '%s' does not exists"), pythonHome.c_str()));
        } else {
            wxUnsetEnv(wxT("PYTHONPATH"));
            if(! wxSetEnv(wxT("PYTHONHOME"), pythonHome)){
                wxMessageBox(_("Could not set environment variable PYTHONHOME"));
            } else {
                DEBUG_TRACE("PYTHONHOME set to " << pythonHome);
            }
        }
    }
#endif
    {
        wxString thePath = MacGetPathToBundledResourceFile(CFSTR("locale"));
        if(thePath != wxT(""))
            locale.AddCatalogLookupPathPrefix(thePath);
        else {
            wxMessageBox(_("Translations not found in bundle"), _("Fatal Error"));
            return false;
        }
    }
	
	{
		wxString thePath = MacGetPathToBundledResourceFile(CFSTR("lensfun"));
		if (thePath == wxT("")) {
			wxMessageBox(_("lensfun directory not found in bundle"),
						 _("Fatal Error"));
			return false;
		}
		HuginBase::LensDB::LensDB::GetSingleton().SetMainDBPath(std::string(thePath.mb_str(HUGIN_CONV_FILENAME)));
	}

#else
    // add the locale directory specified during configure
    m_xrcPrefix = wxT(INSTALL_XRC_DIR);
    m_DataDir = wxT(INSTALL_DATA_DIR);
    locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
#endif

    if ( ! wxFile::Exists(m_xrcPrefix + wxT("/main_frame.xrc")) ) {
        wxMessageBox(_("xrc directory not found, hugin needs to be properly installed\nTried Path:" + m_xrcPrefix ), _("Fatal Error"));
        return false;
    }

    // here goes and comes configuration
    wxConfigBase * config = wxConfigBase::Get();
    // do not record default values in the preferences file
    config->SetRecordDefaults(false);

    config->Flush();

    // initialize i18n
    int localeID = config->Read(wxT("language"), (long) HUGIN_LANGUAGE);
    DEBUG_TRACE("localeID: " << localeID);
    {
        bool bLInit;
	    bLInit = locale.Init(localeID);
	    if (bLInit) {
	        DEBUG_TRACE("locale init OK");
	        DEBUG_TRACE("System Locale: " << locale.GetSysName().mb_str(wxConvLocal))
	        DEBUG_TRACE("Canonical Locale: " << locale.GetCanonicalName().mb_str(wxConvLocal))
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

    // add custom XRC handlers
    wxXmlResource::Get()->AddHandler(new ImagesPanelXmlHandler());
    wxXmlResource::Get()->AddHandler(new CPEditorPanelXmlHandler());
    wxXmlResource::Get()->AddHandler(new CPImageCtrlXmlHandler());
    wxXmlResource::Get()->AddHandler(new CPImagesComboBoxXmlHandler());
    wxXmlResource::Get()->AddHandler(new MaskEditorPanelXmlHandler());
    wxXmlResource::Get()->AddHandler(new ImagesListMaskXmlHandler());
    wxXmlResource::Get()->AddHandler(new MaskImageCtrlXmlHandler());
    wxXmlResource::Get()->AddHandler(new OptimizePanelXmlHandler());
    wxXmlResource::Get()->AddHandler(new OptimizePhotometricPanelXmlHandler());
    wxXmlResource::Get()->AddHandler(new PanoPanelXmlHandler());
    wxXmlResource::Get()->AddHandler(new PreviewPanelXmlHandler());
    wxXmlResource::Get()->AddHandler(new HtmlWindowXmlHandler());
    wxXmlResource::Get()->AddHandler(new wxTreeListCtrlXmlHandler());
    wxXmlResource::Get()->AddHandler(new ImagesTreeCtrlXmlHandler());

    // load XRC files
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cp_list_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("preview_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("edit_script_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_menu.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_tool.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("about.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("pref_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cpdetector_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("reset_dialog.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("optimize_photo_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("cp_editor_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("images_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("main_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("optimize_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("pano_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("mask_editor_panel.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("lensdb_dialogs.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("image_variable_dlg.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("dlg_warning.xrc"));
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

    // setup main frame size, after it has been created.
    RestoreFramePosition(frame, wxT("MainFrame"));
#ifdef __WXMSW__
    provider->SetHelpController(&frame->GetHelpController());
    frame->GetHelpController().Initialize(m_xrcPrefix+wxT("data/hugin_help_en_EN.chm"));
    frame->SendSizeEvent();
#endif

    // we are closing Hugin, if the top level window is deleted
    SetExitOnFrameDelete(true);
    // show the frame.
    if(frame->GetGuiLevel()==GUI_SIMPLE)
    {
        SetTopWindow(frame->getGLPreview());
    }
    else
    {
        frame->Show(TRUE);
    };

    wxString cwd = wxFileName::GetCwd();

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
#elif (defined __WXMAC__) && (defined MAC_SELF_CONTAINED_BUNDLE)
        DEBUG_DEBUG("temp dir on Mac");
        m_workDir = MacGetPathToUserDomainTempDir();
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
        DEBUG_DEBUG("creating temp dir: " << m_workDir.mb_str(wxConvLocal));
        if (!wxMkdir(m_workDir)) {
            DEBUG_ERROR("Tempdir could not be created: " << m_workDir.mb_str(wxConvLocal));
        }
    }
    if (!wxSetWorkingDirectory(m_workDir)) {
        DEBUG_ERROR("could not change to temp. dir: " << m_workDir.mb_str(wxConvLocal));
    }
    DEBUG_DEBUG("using temp dir: " << m_workDir.mb_str(wxConvLocal));

    // set some suitable defaults
    GlobalCmdHist::getInstance().addCommand(new wxNewProjectCmd(pano));
    GlobalCmdHist::getInstance().clear();

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    if (argc > 1)
    {
#ifdef __WXMSW__
        //on Windows we need to update the fast preview first
        //otherwise there is an infinite loop when starting with a project file
        //and closed panorama editor aka mainframe
        if(frame->GetGuiLevel()==GUI_SIMPLE)
        {
            frame->getGLPreview()->Update();
        };
#endif
        wxFileName file(argv[1]);
        // if the first file is a project file, open it
        if (file.GetExt().CmpNoCase(wxT("pto")) == 0 ||
            file.GetExt().CmpNoCase(wxT("pts")) == 0 ||
            file.GetExt().CmpNoCase(wxT("ptp")) == 0 )
        {
            if(file.IsRelative())
                file.MakeAbsolute(cwd);
            // Loading the project file with set actualPath to its
            // parent directory.  (actualPath is used as starting
            // directory by many subsequent file selection dialogs.)
            frame->LoadProjectFile(file.GetFullPath());
        } else {
            std::vector<std::string> filesv;
            for (int i=1; i< argc; i++) 
            {
                bool actualPathSet = false;
#if defined __WXMSW__
                //expand wildcards
                wxFileName fileList(argv[i]);
                if(fileList.IsRelative())
                    fileList.MakeAbsolute(cwd);
                wxDir dir;
                wxString foundFile;
                wxFileName file;
                if(fileList.DirExists())
                    if(dir.Open(fileList.GetPath()))
                        if(dir.GetFirst(&foundFile,fileList.GetFullName(),wxDIR_FILES | wxDIR_HIDDEN))
                            do
                            {
                                file=foundFile;
                                file.MakeAbsolute(dir.GetName());
#else
                wxFileName file(argv[i]);
#endif
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
                    if(file.IsRelative())
                        file.MakeAbsolute(cwd);
                    if(!containsInvalidCharacters(file.GetFullPath()))
                    {
                        filesv.push_back((const char *)(file.GetFullPath().mb_str(HUGIN_CONV_FILENAME)));
                    };

                    // Use the first filename to set actualPath.
                    if (! actualPathSet)
                    {
                        config->Write(wxT("/actualPath"), file.GetPath());
                        actualPathSet = true;
                    }
                }
#if defined __WXMSW__
                } while (dir.GetNext(&foundFile));
#endif
            }
            if(filesv.size()>0)
            {
                std::vector<PT::PanoCommand*> cmds;
                cmds.push_back(new PT::wxAddImagesCmd(pano,filesv));
                cmds.push_back(new PT::DistributeImagesCmd(pano));
                cmds.push_back(new PT::CenterPanoCmd(pano));
                GlobalCmdHist::getInstance().addCommand(new PT::CombinedPanoCommand(pano, cmds));
            }
        }
    }
#ifdef __WXMAC__
    m_macInitDone = true;
    if(m_macOpenFileOnStart) {frame->LoadProjectFile(m_macFileNameToOpenOnStart);}
    m_macOpenFileOnStart = false;
#endif

	//check for no tip switch, needed by PTBatcher
	wxString secondParam = argc > 2 ? wxString(argv[2]) : wxString();
	if(secondParam.Cmp(_T("-notips"))!=0)
	{
		//load tip startup preferences (tips will be started after splash terminates)
		int nValue = config->Read(wxT("/MainFrame/ShowStartTip"), 1l);

		//show tips if needed now
		if(nValue > 0)
		{
			wxCommandEvent dummy;
			frame->OnTipOfDay(dummy);
		}
	}

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

MainFrame* huginApp::getMainFrame()
{
    if (m_this) {
        return m_this->frame;
    } else {
        return 0;
    }
}

void huginApp::relayImageLoaded(ImageReadyEvent & event)
{
    ImageCache::getInstance().postEvent(event.request, event.entry);
}

void huginApp::imageLoadedAsync(ImageCache::RequestPtr request, ImageCache::EntryPtr entry)
{
    ImageReadyEvent event(request, entry);
    // AddPendingEvent adds the event to the event queue and returns without
    // processing it. This is necessary since we are probably not in the
    // UI thread, but the event handler must be run in the UI thread since it
    // could update image views.
    Get()->AddPendingEvent(event);
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
    DEBUG_TRACE(basename.mb_str(wxConvLocal));

    wxConfigBase * config = wxConfigBase::Get();

    // get display size
    int dx,dy;
    wxDisplaySize(&dx,&dy);

#if ( __WXGTK__ ) &&  wxCHECK_VERSION(2,6,0)
// restoring the splitter positions properly when maximising doesn't work.
// Disabling maximise on wxWidgets >= 2.6.0 and gtk
        //size
        int w = config->Read(wxT("/") + basename + wxT("/width"),-1l);
        int h = config->Read(wxT("/") + basename + wxT("/height"),-1l);
        if (w > 0 && w <= dx) {
            frame->SetClientSize(w,h);
        } else {
            frame->Fit();
        }
        //position
        int x = config->Read(wxT("/") + basename + wxT("/positionX"),-1l);
        int y = config->Read(wxT("/") + basename + wxT("/positionY"),-1l);
        if ( y >= 0 && x >= 0 && x < dx && y < dy) {
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
        if (w > 0 && w <= dx) {
            frame->SetClientSize(w,h);
        } else {
            frame->Fit();
        }
        //position
        int x = config->Read(wxT("/") + basename + wxT("/positionX"),-1l);
        int y = config->Read(wxT("/") + basename + wxT("/positionY"),-1l);
        if ( y >= 0 && x >= 0 && x < dx && y < dy) {
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

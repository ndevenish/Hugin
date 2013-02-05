// -*- c-basic-offset: 4 -*-
/** @file LensCalApp.cpp
 *
 *  @brief implementation of LensCal application class
 *
 *  @author T. Modes
 *
 */

/* 
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

#include "panoinc_WX.h"
#include "panoinc.h"
#include "base_wx/platform.h"

#include "LensCalApp.h"
#include "LensCalImageCtrl.h"
#include "base_wx/huginConfig.h"
#include "hugin/config_defaults.h"
#include "base_wx/PTWXDlg.h"
#include "lensdb/LensDB.h"

#include <tiffio.h>


// make wxwindows use this class as the main application
IMPLEMENT_APP(LensCalApp)
BEGIN_EVENT_TABLE(LensCalApp, wxApp)
END_EVENT_TABLE()

bool LensCalApp::OnInit()
{
    SetAppName(wxT("hugin"));
    // register our custom pano tools dialog handlers
    registerPTWXDlgFcn();

#if defined __WXMSW__
    wxString huginExeDir = getExePath(argv[0]);
    wxString huginRoot;
    wxFileName::SplitPath( huginExeDir, &huginRoot, NULL, NULL );
    m_xrcPrefix = huginRoot + wxT("/share/hugin/xrc/");
    // lensfun database init
    wxString lensfunDBPath=huginRoot + wxT("/share/lensfun");
    HuginBase::LensDB::LensDB::GetSingleton().SetMainDBPath(std::string(lensfunDBPath.mb_str(HUGIN_CONV_FILENAME)));
    // locale setup
    locale.AddCatalogLookupPathPrefix(huginRoot + wxT("/share/locale"));
#elif defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE
    // initialize paths
    wxString thePath = MacGetPathToBundledResourceFile(CFSTR("xrc"));
    if (thePath == wxT("")) {
        wxMessageBox(_("xrc directory not found in bundle"), _("Fatal Error"));
        return false;
    }
    m_xrcPrefix = thePath + wxT("/");
    thePath = MacGetPathToBundledResourceFile(CFSTR("locale"));
    if(thePath != wxT(""))
        locale.AddCatalogLookupPathPrefix(thePath);
    else {
        wxMessageBox(_("Translations not found in bundle"), _("Fatal Error"));
        return false;
    }
    thePath = MacGetPathToBundledResourceFile(CFSTR("lensfun"));
    if (thePath == wxT("")) {
        wxMessageBox(_("lensfun directory not found in bundle"),
                        _("Fatal Error"));
        return false;
    }
    HuginBase::LensDB::LensDB::GetSingleton().SetMainDBPath(std::string(thePath.mb_str(HUGIN_CONV_FILENAME)));
#else
    // add the locale directory specified during configure
    m_xrcPrefix = wxT(INSTALL_XRC_DIR);
    locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
#endif

    if ( ! wxFile::Exists(m_xrcPrefix + wxT("/lenscal_frame.xrc")) )
    {
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
    wxXmlResource::Get()->AddHandler(new LensCalImageCtrlXmlHandler());
    // load XRC files
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("lenscal_frame.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("lensdb_dialogs.xrc"));
    wxXmlResource::Get()->Load(m_xrcPrefix + wxT("dlg_warning.xrc"));

    // create main frame
    m_frame = new LensCalFrame(NULL);
    SetTopWindow(m_frame);

    // setup main frame size, after it has been created.
    RestoreFramePosition(m_frame, wxT("LensCalFrame"));

    // show the frame.
    m_frame->Show(TRUE);

    // suppress tiff warnings
    TIFFSetWarningHandler(0);

    return true;
}

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

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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include <wx/config.h>              // wx config classes for all systems
#include "wx/image.h"               // wxImage
#include "wx/xrc/xmlres.h"          // XRC XML resouces
#include <wx/filename.h>            // files and dirs
#include <wx/file.h>

#include "hugin/MainFrame.h"

#include "hugin/huginApp.h"


MainFrame * frame;

// make wxwindows use this class as the main application
IMPLEMENT_APP(huginApp)

huginApp::huginApp()
{
    DEBUG_TRACE("");
}

huginApp::~huginApp()
{
    DEBUG_TRACE("");
//    delete frame;
    DEBUG_TRACE("");
}

bool huginApp::OnInit()
{
    DEBUG_INFO( GetAppName().c_str() )
    DEBUG_INFO( wxFileName::GetCwd().c_str() )
    DEBUG_INFO( wxFileName::GetHomeDir().c_str() )


    // here goes and comes configuration
    wxConfigBase* config = new wxConfig ( "hugin",
			"hugin Team", ".huginrc", "huginrc",
			 wxCONFIG_USE_LOCAL_FILE );

    // set as global config, so that other parts of hugin and wxWindows
    // controls can use it easily
    wxConfigBase::Set(config);
    config->SetRecordDefaults(TRUE);

    DEBUG_INFO((wxString)"GetVendorName(): " + config->GetVendorName().c_str());
    config->SetRecordDefaults(TRUE);

    if ( config->IsRecordingDefaults() ) {
      char e_dbg[128] = "writes in config: ";
      sprintf ( e_dbg ,"%s %d\n", e_dbg, config->GetNumberOfEntries() );
      DEBUG_INFO(e_dbg);
    }
    config->Flush();

    // initialize i18n
    locale.Init(wxLANGUAGE_DEFAULT);

    // add local Paths
    locale.AddCatalogLookupPathPrefix(wxT("po"));
    #ifndef INSTALL_LOCALE_DIR          // FIXME
      #define INSTALL_LOCALE_DIR "/opt/kai-uwe/share/locale"
    #endif
    #ifdef INSTALL_LOCALE_DIR
      locale.AddCatalogLookupPathPrefix("/usr/local/share/locale");
      locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
      DEBUG_INFO((wxString)"add locale path: " + INSTALL_LOCALE_DIR)
    #endif
    // add path from config file
    if (config->HasEntry(wxT("locale_path"))){
        locale.AddCatalogLookupPathPrefix(  config->Read("locale_path").c_str() );
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
    // try local xrc files first
    #ifndef INSTALL_XRC_DIR          // FIXME
      #define INSTALL_XRC_DIR "/opt/kai-uwe/share/hugin/xrc"
    #endif
    wxString xrcPrefix;
    // testing for xrc file location
    if ( wxFile::Exists("xrc/main_frame.xrc")/*local_file.IsOpened()*/ ) {
        DEBUG_INFO("using local xrc files")
        xrcPrefix = "xrc/";
    } else if ( wxFile::Exists((wxString)wxT(INSTALL_XRC_DIR) + wxT("/main_frame.xrc")) ) {
        DEBUG_INFO("using installed xrc files")
        xrcPrefix = (wxString)wxT(INSTALL_XRC_DIR) + wxT("/");
    } else if ( wxFile::Exists("/usr/local/share/hugin/xrc/main_frame.xrc") ) {
        DEBUG_INFO("using installed xrc files in standard path")
        xrcPrefix = "/usr/local/share/hugin/xrc/";
    } else {
        DEBUG_INFO("using xrc prefix from config")
        xrcPrefix = config->Read("xrc_path") + wxT("/");
    }
    wxXmlResource::Get()->Load(xrcPrefix + wxT("main_frame.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("images_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("lens_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("lens_dialog.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("pano_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("cp_editor_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("main_menu.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("main_tool.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("edit_text.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("about.xrc"));
#endif

    // create main frame
    frame = new MainFrame();
    SetTopWindow(frame);

    // show the frame.
    frame->Show(TRUE);

    DEBUG_TRACE("");
    return true;
}


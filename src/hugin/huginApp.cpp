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

#include "panoinc.h"
#include "panoinc_WX.h"

#include <wx/config.h>              // wx config classes for all systems
#include <wx/image.h>               // wxImage
#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include <wx/filename.h>            // files and dirs
#include <wx/file.h>

#include "hugin/huginApp.h"
#include "hugin/PanoPanel.h"

#include <tiffio.h>

Server * server;

// make wxwindows use this class as the main application
IMPLEMENT_APP(huginApp)

huginApp::huginApp()
{
    DEBUG_TRACE("ctor");
}

huginApp::~huginApp()
{
    DEBUG_TRACE("dtor");
    // delete temporary dir
//    if (!wxRmdir(m_workDir)) {
//        DEBUG_ERROR("Could not remove temporary directory");
//    }

//    delete frame;
    DEBUG_TRACE("dtor end");
}

bool huginApp::OnInit()
{
    SetAppName("hugin");
//    DEBUG_INFO( GetAppName().c_str() )
//    DEBUG_INFO( wxFileName::GetCwd().c_str() )
//    DEBUG_INFO( wxFileName::GetHomeDir().c_str() )


    // here goes and comes configuration
    wxConfigBase * config = wxConfigBase::Get();
//    wxConfigBase* config = new wxConfig ( "hugin",
//			"hugin Team", ".huginrc", "huginrc",
//			 wxCONFIG_USE_LOCAL_FILE );

    // set as global config, so that other parts of hugin and wxWindows
    // controls can use it easily
    wxConfigBase::Set(config);
    config->SetRecordDefaults(TRUE);

//    DEBUG_INFO((wxString)"GetVendorName(): " + config->GetVendorName().c_str());
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
//    locale.AddCatalogLookupPathPrefix("/usr/local/share/locale");
    locale.AddCatalogLookupPathPrefix(wxT(INSTALL_LOCALE_DIR));
    DEBUG_INFO((wxString)"add locale path: " + INSTALL_LOCALE_DIR)
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
    wxString xrcPrefix;
    // testing for xrc file location
    if ( wxFile::Exists("xrc/main_frame.xrc") ) {
        DEBUG_INFO("using local xrc files");
        wxString currentDir = wxFileName::GetCwd();
        xrcPrefix = currentDir + "/xrc/";
    } else if ( wxFile::Exists((wxString)wxT(INSTALL_XRC_DIR) + wxT("/main_frame.xrc")) ) {
        DEBUG_INFO("using installed xrc files");
        xrcPrefix = (wxString)wxT(INSTALL_XRC_DIR) + wxT("/");
//    } else if ( wxFile::Exists("/usr/local/share/hugin/xrc/main_frame.xrc") ) {
//        DEBUG_INFO("using installed xrc files in standard path")
//        xrcPrefix = "/usr/local/share/hugin/xrc/";
    } else {
        DEBUG_INFO("using xrc prefix from config")
        xrcPrefix = config->Read("xrc_path") + wxT("/");
    }
    wxXmlResource::Get()->Load(xrcPrefix + wxT("main_frame.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("images_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("lens_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("image_center.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("pano_panel.xrc"));
//    wxXmlResource::Get()->Load(xrcPrefix + wxT("preview_frame.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("cp_editor_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("cp_list_frame.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("optimize_panel.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("preview_frame.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("run_optimizer_frame.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("edit_script_dialog.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("run_stitcher_frame.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("anchor_orientation.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("main_menu.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("main_tool.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("edit_text.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("about.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("help.xrc"));
    wxXmlResource::Get()->Load(xrcPrefix + wxT("keyboard_help.xrc"));
#endif

    // create an wxSocketServer for commmunication with panopreview
    server = new Server();

    // create main frame
    frame = new MainFrame(NULL, pano);
    SetTopWindow(frame);


    wxString cwd = wxFileName::GetCwd();
    config->Write( "startDir", cwd );

    m_workDir = config->Read("tempDir","");
    // FIXME, make secure against some symlink attacks
    // get a temp dir
#ifdef __unix__
    DEBUG_DEBUG("on unix..");
    if (m_workDir == "") {
        m_workDir = "/tmp";
    }
#else
#ifdef __WXMSW__
    DEBUG_DEBUG("figuring out windows temp dir");
    if (m_workDir == "") {
        /* added by Yili Zhao */
        char buffer[255];
        GetTempPath(255, buffer);
        m_workDir = buffer;
    }
#else
    DEBUG_ERROR("don't know how to find the temp dir on mac");
    m_workDir = "";
#endif
#endif
    DEBUG_DEBUG("using temp dir: " << m_workDir.c_str());
    if (!wxFileName::DirExists(m_workDir)) {
        DEBUG_DEBUG("creating temp dir: " << m_workDir);
        if (!wxMkdir(m_workDir)) {
            DEBUG_ERROR("Tempdir could not be created: " << m_workDir);
        }
    }
    config->Write("tempDir",m_workDir);
    if (!wxSetWorkingDirectory(m_workDir)) {
        DEBUG_ERROR("could not change to temp. dir: " << m_workDir);
    }

    // show the frame.
    frame->Show(TRUE);

    // TODO: check if we need to load images.
    if (argc == 2) {
        wxString filename(cwd);
        filename.append(wxFileName::GetPathSeparator()).append(argv[1]);
        frame->LoadProjectFile(filename);
    }
    
    // suppress tiff warnings
    TIFFSetWarningHandler(0);
    
    DEBUG_TRACE("");
    return true;
}

int huginApp::OnExit()
{
    DEBUG_TRACE("");
    return wxApp::OnExit();
}

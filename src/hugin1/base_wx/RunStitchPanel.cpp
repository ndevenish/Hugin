// -*- c-basic-offset: 4 -*-

/** @file RunStitchPanel.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: hugin_stitch_project.cpp 2705 2008-01-27 19:56:06Z ippei $
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

#include <hugin_config.h>
#include <hugin_version.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include <wx/wfstream.h>

#include <fstream>
#include <sstream>
#include <vigra/error.hxx>
#include <vigra_ext/MultiThreadOperations.h>
#include "PT/Panorama.h"
#include "PT/utils.h"
#include "base_wx/huginConfig.h"
#include "base_wx/MyProgressDialog.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/platform.h"
#include "common/wxPlatform.h"

#include "RunStitchPanel.h"

using namespace vigra;
using namespace PT;
using namespace std;
using namespace utils;


// ==========================================
// Implementation of stitch window

// event ID's for RunStitchPanel
enum
{
    ID_Quit = 1,
    ID_About   
};


BEGIN_EVENT_TABLE(RunStitchPanel, wxPanel)
    EVT_END_PROCESS(-1, RunStitchPanel::OnProcessTerminate)
END_EVENT_TABLE()

RunStitchPanel::RunStitchPanel(wxWindow * parent)
    : wxPanel(parent)
{
    /*
    wxMenu *menuFile = new wxMenu;

    menuFile->AppendSeparator();
    menuFile->Append( ID_Quit, _("E&xit") );

    wxMenu *menuHelp = new wxMenu;
    menuHelp->Append( ID_About, _("&About...") );

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append( menuFile, _("&File") );
    menuBar->Append( menuHelp, _("&Help") );
    SetMenuBar( menuBar );
    */

    wxBoxSizer * topsizer = new wxBoxSizer( wxVERTICAL );
    m_execPanel = new MyExecPanel(this);

    topsizer->Add(m_execPanel, 1, wxEXPAND, 0);
    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );   // set size hints to honour minimum size
}


bool RunStitchPanel::StitchProject(wxString scriptFile, wxString outname, PTPrograms progs)
{
    wxFileName fname(scriptFile);
    if ( !fname.FileExists() ) {
        wxLogError( _("Could not open project file:") + scriptFile);
        return false;
    }

    wxString pathToPTO;
    wxFileName::SplitPath(scriptFile, &pathToPTO, NULL, NULL);
    pathToPTO.Append(wxT("/"));

    ifstream prjfile((const char *)scriptFile.mb_str());
    if (prjfile.bad()) {
        wxLogError( wxString::Format(_("could not open script : %s"), scriptFile.c_str()) );
        return false;
    }

    PT::Panorama pano;
    PT::PanoramaMemento newPano;

    if (newPano.loadPTScript(prjfile, (const char *)pathToPTO.mb_str())) {
        pano.setMemento(newPano);
    } else {
        wxLogError( wxString::Format(_("error while parsing panos tool script: %s"), scriptFile.c_str()) );
        return false;
    }

    // make sure we got an absolute path
    if (! wxIsAbsolutePath(outname)) {
        outname = wxGetCwd() + wxT("/") + outname;
    }

    DEBUG_DEBUG("output file specified is " << (const char *)outname.mb_str());

    wxString basename;
    wxString outpath;
    wxFileName outputPrefix(outname);
    outpath = outputPrefix.GetPath();
    basename = outputPrefix.GetFullName();
    cout << "output path: " << outpath.mb_str() << " file:" << basename.mb_str() << endl;

    // stitch only active images
    UIntSet activeImgs = pano.getActiveImages();

    try {
        PanoramaOptions  opts = pano.getOptions();

        // copy pto file to temporary file
        m_currentPTOfn = wxFileName::CreateTempFileName(wxT("huginpto_"));
        if(m_currentPTOfn.size() == 0) {
            wxLogError(_("Could not create temporary file"));
        }
        DEBUG_DEBUG("tmpPTOfn file: " << (const char *)m_currentPTOfn.mb_str());
        // copy is not enough, need to adjust image path names...
        ofstream script(m_currentPTOfn.mb_str());
        PT::UIntSet all;
        if (pano.getNrOfImages() > 0) {
            fill_set(all, 0, pano.getNrOfImages()-1);
        }
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, "");
        script.close();

        // produce suitable makefile

        wxFile makeFile;
        //TODO: change to implementatin with config->Read(wxT("tempDir"),wxT(""))
        m_currentMakefn = wxFileName::CreateTempFileName(wxT("huginmk_"), &makeFile);
        if(m_currentMakefn.size() == 0) {
            wxLogError(_("Could not create temporary file"));
            return false;
        }
        DEBUG_DEBUG("makefn file: " << (const char *)m_currentMakefn.mb_str());
        ofstream makeFileStream(m_currentMakefn.mb_str());
        makeFile.Close();

        std::string resultFn(basename.mb_str());
        std::string tmpPTOfnC = (const char *) m_currentPTOfn.mb_str();

        PT::createMakefile(pano,
                           activeImgs,
                           tmpPTOfnC,
                           resultFn,
                           progs,
                           "",
                           makeFileStream);

        // cd to output directory, if one is given.
        wxString oldCWD = wxFileName::GetCwd();
        wxFileName::SetCwd(outpath);

        wxString args = wxT("-f ") + wxQuoteString(m_currentMakefn) + wxT(" all clean");

        wxString caption = wxString::Format(_("Stitching %s"), scriptFile.c_str());

#if defined __WXMAC__ && defined MAC_SELF_CONTAINED_BUNDLE   
        wxString cmd = MacGetPathToBundledExecutableFile(CFSTR("gnumake"));  
        if(cmd != wxT("")) {
            cmd = wxQuoteString(cmd); 
        } else {
            wxMessageBox(wxString::Format(_("External program %s not found in the bundle, reverting to system path"), wxT("gnumake")), _("Error"));
            cmd = wxT("make");  
        }
        cmd += wxT(" ") + args;
#else
        wxString cmd = wxT("make ") + args;  
#endif

        if (m_execPanel->ExecWithRedirect(cmd) == -1) {
            wxMessageBox(wxString::Format(_("Error while stitching project\n%s"), cmd.c_str()),
                         _("Error during stitching"),  wxICON_ERROR | wxOK );
        }
    } catch (std::exception & e) {
        cerr << "caught exception: " << e.what() << std::endl;
        wxMessageBox(wxString(e.what(), wxConvLocal),
                     _("Error during stitching"), wxICON_ERROR | wxOK );

    }
    return true;
}

void RunStitchPanel::OnProcessTerminate(wxProcessEvent & event)
{
    DEBUG_TRACE("");
    // delete temporary files
#ifndef DEBUG
    wxRemoveFile(m_currentMakefn);
    wxRemoveFile(m_currentPTOfn);
#endif
    int ret = event.GetExitCode();
    // notify parent of exit
    if (this->GetParent()) {
        event.SetEventObject( this );
        DEBUG_TRACE("Sending wxProcess event");   
        this->GetParent()->ProcessEvent( event );
    }
}

void RunStitchPanel::CancelStitch()
{
    DEBUG_TRACE("");
    m_execPanel->KillProcess();
}


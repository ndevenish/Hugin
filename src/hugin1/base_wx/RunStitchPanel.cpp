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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <hugin_config.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include <wx/wfstream.h>
#include <wx/stdpaths.h>

#include <fstream>
#include <sstream>
#include <vigra/error.hxx>
#include "base_wx/huginConfig.h"
#include "base_wx/MyExternalCmdExecDialog.h"
#include "base_wx/Executor.h"
#include "base_wx/AssistantExecutor.h"
#include "base_wx/StitchingExecutor.h"

#include "base_wx/platform.h"
#include "base_wx/wxPlatform.h"

#include "RunStitchPanel.h"

#include "hugin/config_defaults.h"

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
	m_paused=false;
	m_overwrite=false;
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

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows, copy color from a child widget
    this->SetBackgroundColour(m_execPanel->GetBackgroundColour());
#endif
    topsizer->Add(m_execPanel, 1, wxEXPAND, 0);
    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );   // set size hints to honour minimum size
}

bool RunStitchPanel::StitchProject(const wxString& scriptFile, const wxString& outname, const wxString& userDefinedOutput)
{
    DEBUG_TRACE("");
    wxFileName fname(scriptFile);
    if ( !fname.FileExists() ) {
        wxLogError( _("Could not open project file:") + scriptFile);
        return false;
    }

    wxString pathToPTO;
    wxFileName::SplitPath(scriptFile, &pathToPTO, NULL, NULL);
    pathToPTO.Append(wxFileName::GetPathSeparator());

    std::ifstream prjfile((const char *)scriptFile.mb_str(HUGIN_CONV_FILENAME));
    if (prjfile.bad()) {
        wxLogError( wxString::Format(_("could not open script: %s"), scriptFile.c_str()) );
        return false;
    }
    HuginBase::Panorama pano;
    HuginBase::PanoramaMemento newPano;
    int ptoVersion = 0;
    if (newPano.loadPTScript(prjfile, ptoVersion, (const char *)pathToPTO.mb_str(HUGIN_CONV_FILENAME))) {
        pano.setMemento(newPano);
        if (pano.getActiveImages().empty())
        {
            wxLogError(wxString::Format(_("Project %s does not contain any active images."), scriptFile.c_str()));
            return false;
        }
        HuginBase::PanoramaOptions opts = pano.getOptions();
        if (ptoVersion < 2) {
            // no options stored in file, use default arguments in config
			
			wxConfig* config = new wxConfig(wxT("hugin"));  //needed for PTBatcher console application
			wxConfigBase::Set(config);                      //
			opts.enblendOptions = wxConfigBase::Get()->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
			opts.enfuseOptions = wxConfigBase::Get()->Read(wxT("/Enfuse/Args"), wxT(HUGIN_ENFUSE_ARGS)).mb_str(wxConvLocal);

        }
        opts.remapUsingGPU = wxConfigBase::Get()->Read(wxT("/Nona/UseGPU"), HUGIN_NONA_USEGPU) == 1;
        pano.setOptions(opts);
    } else {
        wxLogError( wxString::Format(_("error while parsing panotools script: %s"), scriptFile.c_str()) );
        return false;
    }
    // get options and correct for correct makefile
    HuginBase::PanoramaOptions opts = pano.getOptions();
    if (!userDefinedOutput.IsEmpty())
    {
        wxFileInputStream input(userDefinedOutput);
        if (!input.IsOk())
        {
            wxLogError(wxString::Format(_("Can't open user defined output sequence \"%s\"."), userDefinedOutput.c_str()));
            return false;
        }
        wxFileConfig settings(input);
        // disable cropped output if user defined setting is requesting
        long supportsCroppedOutput;
        settings.Read(wxT("/General/SupportsCroppedTIFF"), &supportsCroppedOutput, 1l);
        if (supportsCroppedOutput != 1)
        {
            opts.tiff_saveROI = false;
        };
    };
    opts.outputFormat = HuginBase::PanoramaOptions::TIFF_m;
    if (opts.enblendOptions.length() == 0) {
        // no options stored in file, use default arguments in config file
        opts.enblendOptions = wxConfigBase::Get()->Read(wxT("/Enblend/Args"), wxT(HUGIN_ENBLEND_ARGS)).mb_str(wxConvLocal);
    }
    pano.setOptions(opts);
    DEBUG_DEBUG("output file specified is " << (const char *)outname.mb_str(wxConvLocal));

    wxString basename;
    wxString outpath;
    wxFileName outputPrefix(outname);
    outputPrefix.MakeAbsolute();
    outpath = outputPrefix.GetPath();
    basename = outputPrefix.GetFullName();
    //get temp dir from preferences
    wxString tempDir= wxConfigBase::Get()->Read(wxT("tempDir"),wxT(""));
    if(!tempDir.IsEmpty())
        if(tempDir.Last()!=wxFileName::GetPathSeparator())
            tempDir.Append(wxFileName::GetPathSeparator());

    try {
        // copy pto file to temporary file
        m_currentPTOfn = wxFileName::CreateTempFileName(tempDir+wxT("huginpto_"));
        if(m_currentPTOfn.size() == 0) {
            wxLogError(_("Could not create temporary file"));
        }
        DEBUG_DEBUG("tmpPTOfn file: " << (const char *)m_currentPTOfn.mb_str(wxConvLocal));
        // copy is not enough, need to adjust image path names...
        std::ofstream script(m_currentPTOfn.mb_str(HUGIN_CONV_FILENAME));
        HuginBase::UIntSet all;
        fill_set(all, 0, pano.getNrOfImages()-1);
        pano.printPanoramaScript(script, pano.getOptimizeVector(), pano.getOptions(), all, false, "");
        script.close();

        // cd to output directory, if one is given.
        m_oldCwd = wxFileName::GetCwd();
        wxFileName::SetCwd(outpath);
        const wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        wxArrayString outputFiles;
        wxString statusText;
        m_tempFiles.clear();
        HuginQueue::CommandQueue* commands;
        if (userDefinedOutput.IsEmpty())
        {
            commands = HuginQueue::GetStitchingCommandQueue(pano, exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), m_currentPTOfn, basename, statusText, outputFiles, m_tempFiles);
        }
        else
        {
            commands= HuginQueue::GetStitchingCommandQueueUserOutput(pano, exePath.GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), m_currentPTOfn, basename, userDefinedOutput, statusText, outputFiles, m_tempFiles);
        };
        if (commands->empty())
        {
            wxMessageBox(_("Queue is empty. This should never happen.") , _("Error during stitching"), wxICON_ERROR | wxOK);
            return false;
        };
        // check output directories.
        wxString overwrittenFiles;
        if (!m_overwrite)
        {
            for (size_t i = 0; i < outputFiles.size(); i++)
            {
                wxString fn(outputFiles[i]);
                if (wxFile::Exists(fn))
                {
                    overwrittenFiles.Append(fn + wxT(" "));
                };
            };

            if (!overwrittenFiles.IsEmpty())
            {
                int overwriteret = wxMessageBox(_("Overwrite existing images?\n\n") + overwrittenFiles, _("Overwrite existing images"), wxYES_NO | wxICON_QUESTION);
                // TODO: change button label ok to overwrite
                if (overwriteret != wxYES) {
                    DEBUG_DEBUG("Abort, do not overwrite images!");
                    return false;
                }
                DEBUG_DEBUG("Overwrite existing images!");
            };
        };
        if (!statusText.empty())
        {
            m_execPanel->AddString(statusText);
        };
        if (m_execPanel->ExecQueue(commands) == -1)
        {
            wxMessageBox(wxString::Format(_("Error while stitching project\n%s"), scriptFile.c_str()),
                             _("Error during stitching"),  wxICON_ERROR | wxOK );
        };
    } catch (std::exception & e)
    {
        std::cerr << "caught exception: " << e.what() << std::endl;
        wxMessageBox(wxString(e.what(), wxConvLocal),
                     _("Error during stitching"), wxICON_ERROR | wxOK );
    }
    return true;
}

bool RunStitchPanel::DetectProject(const wxString& scriptFile)
{
    m_currentPTOfn=wxEmptyString;
    wxFileName fname(scriptFile);
    if ( !fname.FileExists() ) {
        wxLogError( _("Could not open project file:") + scriptFile);
        return false;
    }
    // get path to project file
    wxString pathToPTO;
    wxFileName::SplitPath(scriptFile, &pathToPTO, NULL, NULL);
    pathToPTO.Append(wxFileName::GetPathSeparator());

    //read project file
    std::ifstream prjfile((const char *)scriptFile.mb_str(HUGIN_CONV_FILENAME));
    if (prjfile.bad())
    {
        wxLogError( wxString::Format(_("could not open script: %s"), scriptFile.c_str()));
        return false;
    }
    HuginBase::Panorama pano;
    HuginBase::PanoramaMemento newPano;
    int ptoVersion = 0;
    if (!newPano.loadPTScript(prjfile, ptoVersion, (const char *)pathToPTO.mb_str(HUGIN_CONV_FILENAME)))
    {
        wxLogError(wxString::Format(_("error while parsing panotools script: %s"), scriptFile.c_str()));
        return false;
    }
    pano.setMemento(newPano);

    //read settings
    wxString tempDir= wxConfigBase::Get()->Read(wxT("tempDir"),wxT(""));
    if (!tempDir.IsEmpty())
    {
        if (tempDir.Last() != wxFileName::GetPathSeparator())
        {
            tempDir.Append(wxFileName::GetPathSeparator());
        };
    };

    try {
        // prepare running assistant
        fname.Normalize();
        const wxFileName exePath(wxStandardPaths::Get().GetExecutablePath());
        HuginQueue::CommandQueue* commands = HuginQueue::GetAssistantCommandQueue(pano, exePath.GetPath(wxPATH_GET_VOLUME|wxPATH_GET_SEPARATOR), fname.GetFullPath());
        if (commands->empty())
        {
            wxMessageBox(_("Queue is empty. This should never happen."), _("Error during running assistant"), wxICON_ERROR | wxOK);
            return false;
        };
        if (m_execPanel->ExecQueue(commands) == -1)
        {
            wxMessageBox(wxString::Format(_("Error while running assistant\n%s"), scriptFile.c_str()),
                         _("Error during running assistant"),  wxICON_ERROR | wxOK );
        }
    } 
    catch (std::exception & e)
    {
        std::cerr << "caught exception: " << e.what() << std::endl;
        wxMessageBox(wxString(e.what(), wxConvLocal),
                     _("Error during running assistant"), wxICON_ERROR | wxOK );
    }
    return true;
}

void RunStitchPanel::OnProcessTerminate(wxProcessEvent & event)
{
    DEBUG_TRACE("");
#ifndef DEBUG
    if(!m_currentPTOfn.IsEmpty())
    {
        wxRemoveFile(m_currentPTOfn);
    };
#endif
    // delete all temp files
    if (!m_tempFiles.empty())
    {
        for (size_t i = 0; i < m_tempFiles.size(); ++i)
        {
            if (wxFileExists(m_tempFiles[i]))
            {
                wxRemoveFile(m_tempFiles[i]);
            };
        };
    };
    // restore old working directory, if changed
    if (!m_oldCwd.IsEmpty())
    {
        wxFileName::SetCwd(m_oldCwd);
    };
    // notify parent of exit
    if (this->GetParent())
    {
        event.SetEventObject( this );
        DEBUG_TRACE("Sending wxProcess event");   
        this->GetParent()->GetEventHandler()->ProcessEvent( event );
    }
}

void RunStitchPanel::CancelStitch()
{
    DEBUG_TRACE("");
    m_execPanel->KillProcess();
}

bool RunStitchPanel::IsPaused()
{
	return m_paused;
}

void RunStitchPanel::SetOverwrite(bool over)
{
	m_overwrite = over;
}

void RunStitchPanel::PauseStitch()
{
	m_paused=true;
	m_execPanel->PauseProcess();
}

void RunStitchPanel::ContinueStitch()
{
	m_execPanel->ContinueProcess();
	m_paused=false;
}

long RunStitchPanel::GetPid()
{
	return m_execPanel->GetPid();
}

bool RunStitchPanel::SaveLog(const wxString &filename)
{
    return m_execPanel->SaveLog(filename);
};

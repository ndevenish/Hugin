// -*- c-basic-offset: 4 -*-

/** @file RunStitcherFrame.cpp
 *
 *  @brief implementation of RunStitcherFrame Class
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

// For compilers that support precompilation, includes "wx/wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include "wx/wx.h"
#endif

#include "wx/xrc/xmlres.h"              // XRC XML resouces
#include "wx/notebook.h"
#include "wx/listctrl.h"
#include "wx/process.h"
#include "wx/txtstrm.h"
#include "wx/config.h"
#include "wx/file.h"

#include <string>
#include <iostream>
#include <fstream>

#include "common/stl_utils.h"
#include "PT/PanoCommand.h"
#include "hugin/CommandHistory.h"

#include "hugin/RunStitcherFrame.h"


using namespace std;
using namespace PT;

// ============================================================================
// RunStitcherFrame implementation
// ============================================================================

BEGIN_EVENT_TABLE(RunStitcherFrame, wxFrame)
    EVT_TIMER(-1,RunStitcherFrame::OnTimer)
    EVT_BUTTON(XRCID("stitcher_abort"),RunStitcherFrame::OnAbort)
    EVT_CLOSE(RunStitcherFrame::OnClose)
    EVT_END_PROCESS(-1, RunStitcherFrame::OnProcessTerm)
END_EVENT_TABLE()

RunStitcherFrame::RunStitcherFrame(wxWindow *parent,
                                   Panorama * pano,
                                   const PanoramaOptions & options)
    : m_pid(-1),
      m_in(0),
      m_pano(pano),
      m_timer(this),
      m_percent(0)
{
    DEBUG_TRACE("");
    bool ok = wxXmlResource::Get()->LoadFrame(this, parent, wxT("run_stitcher_frame"));
    assert(ok);

    m_stitcherStatus = XRCCTRL(*this, "stitcher_status", wxStaticText);
    assert(m_stitcherStatus);
    m_stitcherProgress = XRCCTRL(*this, "stitcher_progress", wxGauge);
    assert(m_stitcherProgress);
    m_abort = XRCCTRL(*this, "stitcher_abort", wxButton);
    assert(m_abort);

    // start PTStitcher process

    wxConfigBase* config = wxConfigBase::Get();
#ifdef __WXMSW__
    wxString stitcherExe = config->Read("/PanoTools/PTStitcherExe","PTStitcher.exe");
    if (!wxFile::Exists(stitcherExe)){
        wxFileDialog dlg(this,_("Select PTStitcher.exe"),
        "", "PTStitcher.exe",
        "Executables (*.exe)|*.exe",
        wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            stitcherExe = dlg.GetPath();
            config->Write("/PanoTools/PTStitcherExe",stitcherExe);
        } else {
            wxLogError(_("No PTStitcher.exe selected"));
        }
    }
#else
    wxString stitcherExe = config->Read("/PanoTools/PTOptimizerExe","PTStitcher");
#endif
    wxString PTScriptFile = config->Read("/PanoTools/ScriptFile","PT_script.txt");

    std::ofstream script(PTScriptFile.c_str());
    if (!script.good()) {
        DEBUG_FATAL("could not open/create PTScript file");
    }
    m_pano->printStitcherScript(script, options);
    script.close();

    wxString cmd = stitcherExe + wxString(" -o \"") + wxString(options.outfile.c_str()) + "\" " + PTScriptFile;

    DEBUG_INFO("Executing cmd: " << cmd);

    // create our process
    m_process = new wxProcess(this);
    m_process->Redirect();
    m_pid = wxExecute(cmd.c_str(), wxEXEC_ASYNC, m_process);

#if __unix__
    if (m_pid <= 0 )
    {
        wxLogError(_T("Failed to launch the PTStitcher."));
        return;
    }
#endif

    wxInputStream * t_in = m_process->GetInputStream();
    assert(t_in);
    m_in = new wxTextInputStream(*t_in);
    if ( !m_in )
    {
        wxLogError(_T("Failed to connect to child stdout"));
        return;
    }
    // set new separators, for parsing PTStitcher output
    m_in->SetStringSeparators("%");

    m_process->SetNextHandler(this);

#ifdef __unix__
    Show();
    // start the timer to poll program output
    m_timer.Start(1000);
#endif
}


void RunStitcherFrame::OnTimer(wxTimerEvent & e)
{
    DEBUG_TRACE("");
    // wxWindows sucks... I wanna get notified of new data,
    // not poll for it.

    wxString line;

    if (!m_process->IsInputOpened()) {
        DEBUG_DEBUG("input not opened");
        return;
    }
    if (!m_process->IsInputAvailable()) {
        DEBUG_DEBUG("no input available");
    }

    // we don't have any way to be notified when any input appears on the
    // stream so we have to poll it :-(
    //
    while ( m_process->IsInputOpened() && m_process->IsInputAvailable() ){
        // read data from stream
        line = m_in->ReadWord();
        DEBUG_DEBUG("parsing line >" << line << "<");
        if (line.size() == 7) {
            // only percentage.
            size_t len = line.size();
            wxString number = line.substr(len -3);
            number.Trim(false);
            DEBUG_DEBUG("number read: >>>>>>>>>" << number << "<<<<<<<<<<<");
            bool ok = number.ToLong(&m_percent);
            assert(ok);
        } else if ( line.size() > 7) {
            // new description and number
            m_description = line.substr(0,line.find("  "));
            m_description.Trim();
            m_description.Trim(false);
            // number
            size_t len = line.size();
            wxString number = line.substr(len-3);
            number.Trim(false);
            DEBUG_DEBUG("number read: >>>>>>>" << number << "<<<<<<<");
            bool ok = number.ToLong(&m_percent);
            assert(ok);
        }
    }
    DEBUG_DEBUG("operation: " << m_description << " progress:" << m_percent << "%");
    m_stitcherStatus->SetLabel(m_description);
    m_stitcherProgress->SetValue(m_percent);
}

void RunStitcherFrame::OnAbort(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (m_process) {
        DEBUG_INFO("Killing PTStitcher process");
        // kill Process if it is still running.
        m_process->Kill(m_pid, wxSIGINT);
    } else {
        Close();
    }
}

void RunStitcherFrame::OnClose(wxCloseEvent& event)
{
    DEBUG_DEBUG("");
    if ( m_process )
    {
        // we're not interested in getting the process termination notification
        // if we are closing it ourselves
        wxProcess *process = m_process;
        m_process = NULL;
        process->SetNextHandler(NULL);

        process->CloseOutput();
    }

    delete m_in;
    m_in = 0;

    event.Skip();
}

void RunStitcherFrame::OnProcessTerm(wxProcessEvent& event)
{
    DEBUG_TRACE("");
#if __unix__
    m_timer.Stop();
#else
    Show();
#endif

    DEBUG_DEBUG("before del process");
    delete m_process;
    m_process = NULL;
    Close();
}

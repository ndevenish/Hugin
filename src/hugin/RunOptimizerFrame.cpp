// -*- c-basic-offset: 4 -*-

/** @file RunOptimizerFrame.cpp
 *
 *  @brief implementation of RunOptimizerFrame Class
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

#include <string>
#include <iostream>
#include <fstream>

#include "common/stl_utils.h"
#include "PT/PanoCommand.h"
#include "hugin/CommandHistory.h"

#include "hugin/RunOptimizerFrame.h"


using namespace std;
using namespace PT;

// ============================================================================
// RunOptimizerFrame implementation
// ============================================================================

BEGIN_EVENT_TABLE(RunOptimizerFrame, wxFrame)
    EVT_TIMER(-1,RunOptimizerFrame::OnTimer)
    EVT_BUTTON(XRCID("optimizer_apply_change"),RunOptimizerFrame::OnApply)
    EVT_BUTTON(XRCID("optimizer_cancel_change"),RunOptimizerFrame::OnCancel)
    EVT_CLOSE(RunOptimizerFrame::OnClose)
    EVT_END_PROCESS(-1, RunOptimizerFrame::OnProcessTerm)
END_EVENT_TABLE()

RunOptimizerFrame::RunOptimizerFrame(wxFrame *parent,
                                     Panorama * pano,
                                     const PanoramaOptions & options,
                                     const OptimizeVector & optvars)
    : m_pid(-1),
      m_in(0),
      m_pano(pano),
      m_timer(this)
{
    DEBUG_TRACE("");
    bool ok = wxXmlResource::Get()->LoadFrame(this, parent, wxT("run_optimizer_frame"));
    assert(ok);

    m_apply = XRCCTRL(*this, "optimizer_apply_change", wxButton);
    assert(m_apply);
    m_cancel = XRCCTRL(*this, "optimizer_cancel_change", wxButton);
    assert(m_cancel);
    m_optimizer_status = XRCCTRL(*this, "optimizer_status", wxStaticText);
    assert(m_optimizer_status);
    m_optimizer_result_text = XRCCTRL(*this, "optimizer_result_text", wxStaticText);
    assert(m_optimizer_result_text);

    
    // start PTOptimizer process
    
    // FIXME get from Preferences
    string optimizerExe("PTOptimizer");
    string PTScriptFile("PT_script.txt");

    std::ofstream script(PTScriptFile.c_str());
    m_pano->printOptimizerScript(script, optvars, options);
    script.close();

    string cmd = optimizerExe + " " + PTScriptFile;

    DEBUG_INFO("Executing cmd: " << cmd);

    // create our process
    m_process = new wxProcess(this);
    m_process->Redirect();
    m_pid = wxExecute(cmd.c_str(), wxEXEC_ASYNC, m_process);

    if (m_pid <= 0 )
    {
        wxLogError(_T("Failed to launch the PTOptimizer."));
        return;
    }

    wxInputStream * t_in = m_process->GetInputStream();
    assert(t_in);
    m_in = new wxTextInputStream(*t_in);
    
    if ( !m_in )
    {
        wxLogError(_T("Failed to connect to child stdout"));
        return;
    }

    m_process->SetNextHandler(this);

    m_apply->Disable();
    m_cancel->SetLabel(_("Stop"));
    Show();
    // start the timer to poll program output


    
#ifdef __unix__
    m_timer.Start(100);
#endif
}


void RunOptimizerFrame::OnTimer(wxTimerEvent & e)
{
    DEBUG_TRACE("");
    // wxWindows sucks... I wanna use QSocketNotifier to get notified!
    // of new data.

    wxString line;
    long iteration = 0;
    double diff = 0;
    
    // we don't have any way to be notified when any input appears on the
    // stream so we have to poll it :-(
    //
    while ( m_process->IsInputOpened() && m_process->IsInputAvailable() ){
        line = m_in->ReadLine();
        wxString rest;
        if (!line.StartsWith("after ", &rest)) {
            continue;
        }
        bool ok;
        ok = rest.BeforeFirst(' ').ToLong(&iteration);
        assert(ok);
        size_t t = line.First(": ");
        wxString t2 = line.substr(t+2).BeforeFirst(' ');
        diff = utils::lexical_cast<double>(t2.c_str());
        DEBUG_DEBUG("iteration: " << iteration << " difference:" << diff);
    }
    m_optimizer_status->SetLabel(
        wxString::Format(_("Iteration %d, average distance: %f"),
                         iteration,diff));
    
}

void RunOptimizerFrame::OnCancel(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    if (m_process) {
        DEBUG_INFO("Killing PTOptimizer process");
        // kill Process if it is still running.
        m_process->Kill(m_pid, wxSIGINT);
    } else {
        DEBUG_DEBUG("discarting optimizer results");
        Close();
    }
}

void RunOptimizerFrame::OnApply(wxCommandEvent & e)
{
    DEBUG_TRACE("");
    // ask the user if he wants to apply the settings
    // FIXME display a list with control points here
    DEBUG_DEBUG("Executing update variables command");
    GlobalCmdHist::getInstance().addCommand(
        new PT::UpdateVariablesCPCmd(*m_pano, m_vars, m_cps)
        );
    
    if (m_process) {
        // kill Process if it is still running.
        m_process->Kill(m_pid, wxSIGINT);
    } else {
        Close();
    }
}

void RunOptimizerFrame::OnClose(wxCloseEvent& event)
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

#if 0
void RunOptimizerFrame::OnProcessTerm(wxProcessEvent& event)
{
    DEBUG_TRACE("");
    m_timer.Stop();
    delete m_process;
    m_process = NULL;

    wxLogWarning(_T("The other process has terminated, closing"));

    // wxMessageBox hangs the program!
    if (wxMessageBox("commit changes", _("Optimize result"),wxYES_NO) == wxID_OK){
        DEBUG_DEBUG("commit");
    }

    DEBUG_DEBUG("before close");
    Close();
    DEBUG_DEBUG("after close");
}
#endif



void RunOptimizerFrame::OnProcessTerm(wxProcessEvent& event)
{
    DEBUG_TRACE("");
    m_timer.Stop();

    // update buttons
    m_apply->Enable();
    m_cancel->SetLabel(_("Cancel"));

    // get current vars & cp from pano
    m_vars = m_pano->getVariables();
    m_cps = m_pano->getCtrlPoints();
    // update them
    m_pano->readOptimizerOutput(m_vars,m_cps);

    // calculate average cp distance
    double dist = 0;
    CPVector::iterator it;
    for (it = m_cps.begin() ; it != m_cps.end(); it++) {
        dist += (*it).error;
    }
    dist = dist / m_cps.size();

    wxString msg;
    msg.Printf(_("Optimizer run finished.\n"
                 "average control point distance: %f\n\n"
                 "Apply the changes?"), dist);
    
    m_optimizer_result_text->SetLabel(msg);

    DEBUG_DEBUG("before del process");
    delete m_process;
    m_process = NULL;
}

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
#include "wx/config.h"
#include "wx/dialog.h"
#include "wx/file.h"

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

RunOptimizerFrame::RunOptimizerFrame(wxWindow *parent,
                                     Panorama * pano,
                                     const PanoramaOptions & options,
                                     const OptimizeVector & optvars,
                                     bool edit)
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

    stringstream script_stream;
    m_pano->printOptimizerScript(script_stream, optvars, options);
    std::string script(script_stream.str());
    if (edit) {
        // open a text dialog with an editor inside
        wxDialog * edit_dlg = wxXmlResource::Get()->LoadDialog(this, "edit_script_dialog");
        wxTextCtrl *txtCtrl=XRCCTRL(*edit_dlg,"script_edit_text",wxTextCtrl);
        txtCtrl->SetValue(script.c_str());

        if (edit_dlg->ShowModal() == wxID_OK) {
            script = txtCtrl->GetValue();
        } else {
            script = script_stream.str();
        }
    } else {
        script = script_stream.str();
    }

    wxConfigBase* config = wxConfigBase::Get();
#ifdef __WXMSW__
    Hide();
    wxString optimizerExe = config->Read("/PanoTools/PTOptimizerExe","PTOptimizer.exe");
    if (!wxFile::Exists(optimizerExe)){
        wxFileDialog dlg(this,_("Select PTOptimizer"),
        "", "PTOptimizer.exe",
        "Executables (*.exe)|*.exe",
        wxOPEN, wxDefaultPosition);
        if (dlg.ShowModal() == wxID_OK) {
            optimizerExe = dlg.GetPath();
            config->Write("/PanoTools/PTOptimizerExe",optimizerExe);
        } else {
            wxLogError(_("No PTOptimizer.exe selected"));
        }
    }
#else
    wxString optimizerExe = config->Read("/PanoTools/PTOptimizerExe","PTOptimizer");
#endif
    wxString PTScriptFile = config->Read("/PanoTools/ScriptFile","PT_script.txt");

    std::ofstream scriptfile(PTScriptFile.c_str());
    scriptfile << script;
    scriptfile.close();

    wxString cmd(optimizerExe + " " + PTScriptFile);

    DEBUG_INFO("Executing cmd: " << cmd);

    // create our process
    m_process = new wxProcess(this);
    m_process->Redirect();
    m_pid = wxExecute(cmd, wxEXEC_ASYNC | wxEXEC_NOHIDE, m_process);

#if __unix__
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
#else
    m_in = 0;
#endif

    m_process->SetNextHandler(this);

    m_apply->Disable();
    m_cancel->SetLabel(_("Stop"));
    // start the timer to poll program output

#ifdef __unix__
    Show();
    m_timer.Start(100);
#endif
}

RunOptimizerFrame::~RunOptimizerFrame()
{
    DEBUG_TRACE("dtor");
    DEBUG_TRACE("dtor end");
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
    if (iteration != 0) {
        m_optimizer_status->SetLabel(
            wxString::Format(_("Iteration %d, average distance: %f"),
                             iteration,diff));
    }

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
    DEBUG_TRACE("");
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


void RunOptimizerFrame::OnProcessTerm(wxProcessEvent& event)
{
    DEBUG_TRACE("");
#if __unix__
    m_timer.Stop();
#else
    Show();
#endif

    // update buttons
    m_apply->Enable();
    m_cancel->SetLabel(_("Cancel"));

    // get current vars & cp from pano
    m_vars = m_pano->getVariables();
    m_cps = m_pano->getCtrlPoints();
    // update them
    m_pano->readOptimizerOutput(m_vars,m_cps);

    // calculate average cp distance
    double mean_error = 0;
    double squared_error = 0;
    double max_error = 0;
    CPVector::iterator it;
    for (it = m_cps.begin() ; it != m_cps.end(); it++) {
        mean_error += (*it).error;
        squared_error += (*it).error * (*it).error;
        if ((*it).error > max_error) {
            max_error = (*it).error;
        }
    }
    mean_error = mean_error / m_cps.size();
    double std_dev = sqrt(squared_error/m_cps.size());


    wxString msg;
    msg.Printf(_("Optimizer run finished.\n"
                 "Results:\n"
                 "  average control point distance: %f\n"
                 "  standart deviation: %f\n"
                 "  maximum: %f\n\n"
                 "Apply the changes?"),
               mean_error, std_dev, max_error);

    m_optimizer_result_text->SetLabel(msg);

    Layout();
    Fit();

    DEBUG_DEBUG("before del process");
    delete m_process;
    m_process = NULL;
}

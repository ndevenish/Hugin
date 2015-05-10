// -*- c-basic-offset: 4 -*-

/** @file RunStitchFrame.cpp
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: RunStitchFrame.cpp 3322 2008-08-16 5:00:07Z mkuder $
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

#include "RunStitchFrame.h"

BEGIN_EVENT_TABLE(RunStitchFrame, wxFrame)
    EVT_BUTTON(wxID_CANCEL, RunStitchFrame::OnCancel)
    EVT_END_PROCESS(-1, RunStitchFrame::OnProcessTerminate)
END_EVENT_TABLE()

RunStitchFrame::RunStitchFrame(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size)  //ProjectArray projList, wxListBox *projListBox)
    : wxFrame(parent, -1, title, pos, size, wxRESIZE_BORDER | wxCAPTION | wxCLIP_CHILDREN), m_isStitching(false)
{

    wxBoxSizer* topsizer = new wxBoxSizer( wxVERTICAL );
    m_stitchPanel = new RunStitchPanel(this);

    topsizer->Add(m_stitchPanel, 1, wxEXPAND | wxALL, 2);

    topsizer->Add( new wxButton(this, wxID_CANCEL, _("Cancel")),
                   0, wxALL | wxALIGN_RIGHT, 10);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows..
    this->SetBackgroundColour(m_stitchPanel->GetBackgroundColour());
#endif

    SetSizer( topsizer );
}

int RunStitchFrame::GetProcessId()
{
    if(m_projectId<0)	//if a fake stitchframe is used (for a command), it doesn't have a stitch panel
    {
        return m_pid;
    }
    else
    {
        return m_stitchPanel->GetPid();
    }
}

int RunStitchFrame::GetProjectId()
{
    return m_projectId;
}

void RunStitchFrame::SetProcessId(int pid)
{
    if(m_projectId<0)	//if a fake stitchframe is used (for a command), it doesn't have a stitch panel
    {
        m_pid = pid;
    }
    else
    {
        m_pid = m_stitchPanel->GetPid();
    }
}

void RunStitchFrame::SetProjectId(int id)
{
    m_projectId=id;
}

void RunStitchFrame::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    DEBUG_TRACE("");
    if (m_isStitching)
    {
        m_stitchPanel->CancelStitch();
        m_isStitching = false;
    }
    else
    {
        Close();
    }
}

void RunStitchFrame::OnProcessTerminate(wxProcessEvent& event)
{
    if (! m_isStitching)
    {
        event.SetEventObject( this );
        event.m_exitcode = 1;
        event.SetId(m_projectId);
        DEBUG_TRACE("Sending wxProcess event");
        this->GetParent()->GetEventHandler()->ProcessEvent( event );
        // TODO: Cleanup files?
        Close();
    }
    else
    {
        m_isStitching = false;
        if (event.GetExitCode() != 0)
        {
            event.SetEventObject( this );
            event.SetId(m_projectId);
            this->GetParent()->GetEventHandler()->ProcessEvent( event );
            Close();
        }
        else
        {
            event.SetEventObject( this );
            event.SetId(m_projectId);
            DEBUG_TRACE("Sending wxProcess event");
            this->GetParent()->GetEventHandler()->ProcessEvent( event );
            Close();
        }
    }
}

bool RunStitchFrame::StitchProject(wxString scriptFile, wxString outname)
{
    if (! m_stitchPanel->StitchProject(scriptFile, outname))
    {
        return false;
    }
    m_isStitching = true;
    m_isDetecting = false;
    return true;
}

bool RunStitchFrame::DetectProject(wxString scriptFile)
{
    if (! m_stitchPanel->DetectProject(scriptFile))
    {
        return false;
    }
    m_isStitching = true;
    m_isDetecting = true;
    return true;
}

bool RunStitchFrame::SaveLog(const wxString& filename)
{
    return m_stitchPanel->SaveLog(filename);
};

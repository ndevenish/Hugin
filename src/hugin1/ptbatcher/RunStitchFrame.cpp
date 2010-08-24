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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "RunStitchFrame.h"

BEGIN_EVENT_TABLE(RunStitchFrame, wxFrame)
    EVT_BUTTON(wxID_CANCEL, RunStitchFrame::OnCancel)
    EVT_END_PROCESS(-1, RunStitchFrame::OnProcessTerminate)
END_EVENT_TABLE()

RunStitchFrame::RunStitchFrame(wxWindow * parent, const wxString& title, const wxPoint& pos, const wxSize& size) //ProjectArray projList, wxListBox *projListBox)
    : wxFrame(parent, -1, title, pos, size, wxRESIZE_BORDER | wxCAPTION | wxCLIP_CHILDREN), m_isStitching(false)
{

    wxBoxSizer * topsizer = new wxBoxSizer( wxVERTICAL );
    m_stitchPanel = new RunStitchPanel(this);

    topsizer->Add(m_stitchPanel, 1, wxEXPAND | wxALL, 2);

    topsizer->Add( new wxButton(this, wxID_CANCEL, _("Cancel")),
                   0, wxALL | wxALIGN_RIGHT, 10);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows..
    this->SetBackgroundColour(m_stitchPanel->GetBackgroundColour());
#endif

    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );   // set size hints to honour minimum size
}






int RunStitchFrame::GetProcessId()
{
	if(m_projectId<0)	//if a fake stitchframe is used (for a command), it doesn't have a stitch panel
		return m_pid;
	else
		return m_stitchPanel->GetPid();
}

int RunStitchFrame::GetProjectId()
{
	return m_projectId;
}

void RunStitchFrame::SetProcessId(int pid)
{
	if(m_projectId<0)	//if a fake stitchframe is used (for a command), it doesn't have a stitch panel
		m_pid = pid;
	else
		m_pid = m_stitchPanel->GetPid();
}

void RunStitchFrame::SetProjectId(int id)
{
	m_projectId=id;
}


void RunStitchFrame::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    DEBUG_TRACE("");
    if (m_isStitching) {
        m_stitchPanel->CancelStitch();
        m_isStitching = false;
    } else {
        Close();
    }
}

void RunStitchFrame::OnProcessTerminate(wxProcessEvent & event)
{
    if (! m_isStitching) {
		// canceled stitch  
		/*if (GetParent()) {		//send process notification to parent window
			event.SetEventObject( this );
			event.m_exitcode = 1;
			event.SetId(m_processId);
			DEBUG_TRACE("Sending wxProcess event");   
			this->GetParent()->ProcessEvent( event );
		}*/
		event.SetEventObject( this );
		event.m_exitcode = 1;
		event.SetId(m_projectId);
		DEBUG_TRACE("Sending wxProcess event");   
		this->GetParent()->ProcessEvent( event );
        // TODO: Cleanup files?
        Close();
    } else {
        m_isStitching = false;
        if (event.GetExitCode() != 0) {
            wxMessageBox(_("Error during stitching\nPlease report the complete text to the bug tracker on http://sf.net/projects/hugin."),
                     _("Error during stitching"), wxICON_ERROR | wxOK );
			event.SetEventObject( this );
			event.SetId(m_projectId);
			//this->GetParent()->ProcessEvent( event );
			this->GetParent()->ProcessEvent( event );
			Close();
        } else {
			//if (GetParent()) {		//send process notification to parent window
				event.SetEventObject( this );
				event.SetId(m_projectId);
				DEBUG_TRACE("Sending wxProcess event");   
				//m_evtParent->ProcessEvent( event );
				this->GetParent()->ProcessEvent( event );
				//this->GetParent()->ProcessEvent( event );
			//}
			/*if(!m_projList.IsEmpty())		//returns true if batch list is not empty yet after removing the first
			{
				wxGetApp().OnStitch(wxGetApp().projList.Item(0).path, wxGetApp().projList.Item(0).prefix);
				m_projList.RemoveAt(0);
				m_projListBox->Delete(0);
			}*/
            Close();
        }
    }
}

bool RunStitchFrame::StitchProject(wxString scriptFile, wxString outname,
                                   HuginBase::PanoramaMakefilelibExport::PTPrograms progs)
{
    if (! m_stitchPanel->StitchProject(scriptFile, outname, progs)) {
        return false;
    }
    m_isStitching = true;
    return true;
}

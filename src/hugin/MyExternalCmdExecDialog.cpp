// -*- c-basic-offset: 4 -*-
/** @file wxExternalCmdExecDial.cpp
*
*  @author Ippei UKAI <ippei_ukai@mac.com>
*
*  $Id$
*
*  This is free software; you can redistribute it and/or
*  modify it under the terms of the GNU General Public
*  License as published by the Free Software Foundation; either
*  version 2 of the License, or (at your option) any later version.
*
*  This software is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*  Lesser General Public License for more details.
*
*  You should have received a copy of the GNU General Public
*  License along with this software; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

// This class is written based on 'exec' sample of wxWidgets library.


#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"

#include "hugin/MyExternalCmdExecDialog.h"


int MyExecuteCommandOnDialog(wxString& command, wxWindow* parent)
{
    MyExternalCmdExecDialog dlg(parent, wxID_ANY);
    return dlg.ShowModal(command);
}

//----------

BEGIN_EVENT_TABLE(MyExternalCmdExecDialog, wxDialog)
EVT_IDLE(MyExternalCmdExecDialog::OnIdle)
EVT_TIMER(wxID_ANY, MyExternalCmdExecDialog::OnTimer)
END_EVENT_TABLE()


MyExternalCmdExecDialog::MyExternalCmdExecDialog(wxWindow* parent, 
                                                 wxWindowID id, 
                                                 const wxString& title, 
                                                 const wxPoint& pos, 
                                                 const wxSize& size, 
                                                 long style,
                                                 const wxString& name)
: wxDialog(parent, id, title, pos, size, style, name),
m_timerIdleWakeUp(this)
{
    //m_lbox = new wxListBox(this, wxID_ANY);
    wxBoxSizer *m_sizer = new wxBoxSizer(wxVERTICAL);
    this->SetSizer(m_sizer);
    m_tbox = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
    wxFont font(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    if ( font.Ok() )
        m_tbox->SetFont(font);
    m_sizer->Add(m_tbox, 1, wxEXPAND);
    m_sizer->AddSpacer(30);
}


int MyExternalCmdExecDialog::ShowModal(wxString &cmd)
{
    process = new MyPipedProcess(this, cmd);
    
    processID = wxExecute(cmd, wxEXEC_ASYNC, process);
    if (!processID)
    {
        delete process;
        EndModal(-1);
    }
    else
    {
        m_timerIdleWakeUp.Start(200);
    }
    
    return wxDialog::ShowModal();
}
    
void MyExternalCmdExecDialog::OnTimer(wxTimerEvent& WXUNUSED(event))
{
    wxWakeUpIdle();
}

void MyExternalCmdExecDialog::OnIdle(wxIdleEvent& event)
{
    if ( process->HasInput() )
    {
        event.RequestMore();
    }
}

//----------

bool MyPipedProcess::HasInput()
{
    bool hasInput = false;
    
    if ( IsInputAvailable() )
    {
        wxTextInputStream tis(*GetInputStream());
        
        // this assumes that the output is always line buffered
        wxString msg;
        msg << _T("> ") << tis.ReadLine() << _T("\n");
        
        //m_parent->GetLogListBox()->Append(msg);
        m_parent->GetLogTextBox()->AppendText(msg);
        
        hasInput = true;
    }
    
    if ( IsErrorAvailable() )
    {
        wxTextInputStream tis(*GetErrorStream());
        
        // this assumes that the output is always line buffered
        wxString msg;
        msg << _T("> ") << tis.ReadLine() << _T("\n");
        
        //m_parent->GetLogListBox()->Append(msg);
        m_parent->GetLogTextBox()->AppendText(msg);
        
        hasInput = true;
    }
    
    return hasInput;
}

void MyPipedProcess::OnTerminate(int pid, int status)
{
    // show the rest of the output
    while ( HasInput() ) ;
    
    m_parent->EndModal(-3);
    
    // we're not needed any more
    delete this;
}

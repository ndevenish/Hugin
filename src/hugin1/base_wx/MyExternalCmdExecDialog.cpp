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

#include <errno.h>

#include "common/wxPlatform.h"

#include "MyExternalCmdExecDialog.h"
#include "hugin/config_defaults.h"

#define LINE_IO 1

int MyExecuteCommandOnDialog(wxString command, wxString args, wxWindow* parent)
{

/*
    command = utils::wxQuoteFilename(command);
    wxString cmdline = command + wxT(" ") + args;
    MyExternalCmdExecDialog dlg(parent, wxID_ANY);
    return dlg.ShowModal(cmdline);

#if defined __WXMAC__
    command = utils::wxQuoteFilename(command);
    wxString cmdline = command + wxT(" ") + args;
    MyExternalCmdExecDialog dlg(parent, wxID_ANY);
    return dlg.ShowModal(cmdline);
*/
#if defined __WXMSW__
    int ret = -1;
    wxFileName tname(command);
    wxString ext = tname.GetExt();
    if (ext != wxT("exe")) {
        SHELLEXECUTEINFO seinfo;
        memset(&seinfo, 0, sizeof(SHELLEXECUTEINFO));
        seinfo.cbSize = sizeof(SHELLEXECUTEINFO);
        seinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        seinfo.lpFile = command.c_str();
        seinfo.lpParameters = args.c_str();
        if (ShellExecuteEx(&seinfo) != TRUE) {
            ret = -1;
            wxMessageBox(_("Could not execute command: ") + command + wxT(" ") + args, _("ShellExecuteEx failed"), wxCANCEL | wxICON_ERROR);
        } else {
            // wait for process
            WaitForSingleObject(seinfo.hProcess, INFINITE);
            ret = 0;
        }
    } else {
        wxString cmdline = utils::wxQuoteFilename(command) + wxT(" ") + args;
        // using CreateProcess on windows
        /* CreateProcess API initialization */
        STARTUPINFO siStartupInfo;
        PROCESS_INFORMATION piProcessInfo;
        memset(&siStartupInfo, 0, sizeof(siStartupInfo));
        memset(&piProcessInfo, 0, sizeof(piProcessInfo));
        siStartupInfo.cb = sizeof(siStartupInfo);
#if wxUSE_UNICODE
        WCHAR * cmdline_c = (WCHAR *) cmdline.wc_str();
        WCHAR * exe_c = (WCHAR *) command.wc_str();
#else //ANSI
        char * cmdline_c = (char*) cmdline.mb_str();
        char * exe_c = (char*) command.mb_str();
#endif
        DEBUG_DEBUG("using CreateProcess() to execute :" << command.mb_str());
        DEBUG_DEBUG("with cmdline:" << cmdline.mb_str());
        ret = CreateProcess(exe_c, cmdline_c, NULL, NULL, FALSE,
                IDLE_PRIORITY_CLASS | CREATE_NEW_CONSOLE, NULL,
                NULL, &siStartupInfo, &piProcessInfo);
        if (ret) {
            ret = 0;
            // Wait until child process exits.
            WaitForSingleObject( piProcessInfo.hProcess, INFINITE );

            // TODO: interrupt waiting and redraw window.

            // Close process and thread handles. 
            CloseHandle( piProcessInfo.hProcess );
            CloseHandle( piProcessInfo.hThread );

        } else {
        ret = -1;
        wxLogError(_("Could not execute command: ") + cmdline  , _("CreateProcess Error"));
        }
    }
    return ret;
#else
    command = utils::wxQuoteFilename(command);
    wxString cmdline = command + wxT(" ") + args;
    MyExternalCmdExecDialog dlg(parent, wxID_ANY);
    return dlg.ShowModal(cmdline);
#endif
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
#ifdef __WXMAC__
    wxFont font(12, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#else
    wxFont font(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#endif
    if ( font.Ok() )
        m_tbox->SetFont(font);
    m_sizer->Add(m_tbox, 1, wxEXPAND);
    m_sizer->AddSpacer(30);
}


int MyExternalCmdExecDialog::ShowModal(const wxString &cmd)
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

int MyExternalCmdExecDialog::Execute(const wxString & cmd)
{
    process = new MyPipedProcess(this, cmd);

    m_exitCode = 0;
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

int MyExternalCmdExecDialog::GetExitCode()
{
    return m_exitCode;
}

void MyExternalCmdExecDialog::SetExitCode(int ret)
{
    m_exitCode = ret;
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

// hack for wxmac 2.7.0-1
#ifdef __WXMAC__ 
    wxTextCtrl * tb = GetLogTextBox();
    tb->SetInsertionPoint(tb->GetLastPosition()); 
#endif
}

//----------

bool MyPipedProcess::HasInput()
{
    bool hasInput = false;

    wxTextInputStream tis(*GetInputStream());
//    if ( IsInputAvailable() )
    if (GetInputStream()->CanRead() )
    {
        DEBUG_DEBUG("input available");
        wxTextCtrl * tb = m_parent->GetLogTextBox();
        
        // does not assume line buffered stream.
        // tries to handle backspace chars properly
        wxString text = tb->GetValue();
        
        while(GetInputStream()->CanRead())
        {
            wxChar c = tis.GetChar();
            if (c) {
                if (c == '\b') {
                    // backspace
                    if (text.Last() != '\n') {
                        text.RemoveLast();
                    }
                } else if (c == 0x0d) {
                    // back to start of line
                    text = text.BeforeLast('\n') + wxT("\n");
                } else {
                    text.Append(c);
                }
            }
        }
        
        tb->SetValue(text);
        tb->ShowPosition(tb->GetLastPosition());
        hasInput = true;
    }

    wxTextInputStream tes(*GetErrorStream());
//    if ( IsErrorAvailable() )
    if (GetErrorStream()->CanRead())
    {
        DEBUG_DEBUG("error available");

        // does not assume line buffered stream.
        // tries to handle backspace chars properly
        wxTextCtrl * tb = m_parent->GetLogTextBox();
        wxString text = tb->GetValue();

        while(GetErrorStream()->CanRead())
        {
            wxChar c = tes.GetChar();
            if (c) {
                if (c == '\b') {
                    // backspace
                    if (text.Last() != '\n') {
                        text.RemoveLast();
                    }
                } else if (c == 0x0d) {
                    // back to start of line
                    text = text.BeforeLast('\n') + wxT("\n");
                } else {
                    text.Append(c);
                }
            }
        }

        tb->SetValue(text);
        tb->ShowPosition(tb->GetLastPosition());
        hasInput = true;
    }
    return hasInput;
}

void MyPipedProcess::OnTerminate(int pid, int status)
{
    // show the rest of the output
    while ( HasInput() ) ;

    m_parent->SetExitCode(status);
    m_parent->EndModal(-3);

    // we're not needed any more
    delete this;
}

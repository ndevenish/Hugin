// -*- c-basic-offset: 4 -*-
/** @file MyExternalCmdExecDialog.cpp
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

#include "wx/ffile.h"
#include "wx/process.h"
#include "wx/mimetype.h"

#ifdef __WINDOWS__
    #include "wx/dde.h"
#endif // __WINDOWS__

#include "MyExternalCmdExecDialog.h"
#include "hugin/config_defaults.h"

#define LINE_IO 1


// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
    // control ids
    Exec_Btn_Cancel = 1000,
};

// ----------------------------------------------------------------------------
// event tables and other macros for wxWidgets
// ----------------------------------------------------------------------------


BEGIN_EVENT_TABLE(MyExecDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL,  MyExecDialog::OnCancel)
//    EVT_IDLE(MyExecDialog::OnIdle)
    EVT_TIMER(wxID_ANY, MyExecDialog::OnTimer)
//    EVT_END_PROCESS(wxID_ANY, MyFrame::OnProcessTerm)
END_EVENT_TABLE()


// ============================================================================
// implementation
// ============================================================================


// frame constructor
MyExecDialog::MyExecDialog(wxWindow * parent, const wxString& title, const wxPoint& pos, const wxSize& size)
       : wxDialog(parent, wxID_ANY, title, pos, size, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU),
         m_timerIdleWakeUp(this)
{
    m_pidLast = 0;

    wxBoxSizer * topsizer = new wxBoxSizer( wxVERTICAL );
    // create the listbox in which we will show misc messages as they come
#ifdef HUGIN_EXEC_LISTBOX
    m_lbox = new wxListBox(this, wxID_ANY);
    m_lbox->Append(m_currLine);
#else
    m_textctrl = new wxTextCtrl(this, wxID_ANY, _T(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY);
    m_lastLineStart = 0;
#endif

    wxFont font(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL,
                wxFONTWEIGHT_NORMAL);
    if ( font.Ok() ) {
#ifdef HUGIN_EXEC_LISTBOX
        m_lbox->SetFont(font);
#else
        m_textctrl->SetFont(font);
#endif
    }

#ifdef HUGIN_EXEC_LISTBOX
    topsizer->Add(m_lbox, 1, wxEXPAND | wxALL, 10);
#else
    topsizer->Add(m_textctrl, 1, wxEXPAND | wxALL, 10);
#endif

    topsizer->Add( new wxButton(this, wxID_CANCEL, _("Cancel")),
                  0, wxALL | wxALIGN_RIGHT, 10);

    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );
}


void MyExecDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    wxKillError rc = wxProcess::Kill(m_pidLast, wxSIGTERM);
    if ( rc != wxKILL_OK ) {
        static const wxChar *errorText[] =
        {
            _T(""), // no error
            _T("signal not supported"),
            _T("permission denied"),
            _T("no such process"),
            _T("unspecified error"),
        };

        wxLogStatus(_("Failed to kill process %ld with sigterm: %s"),
                    m_pidLast, errorText[rc]);
    }
}


int MyExecDialog::ExecWithRedirect(wxString cmd)
{
    if ( !cmd )
        return -1;

    MyPipedProcess *process = new MyPipedProcess(this, cmd);
    if ( !wxExecute(cmd, wxEXEC_ASYNC, process) )
    {
        wxLogError(_T("Execution of '%s' failed."), cmd.c_str());

        delete process;
        return -1;
    }
    else
    {
        AddAsyncProcess(process);
    }
    m_cmdLast = cmd;

    return ShowModal();
}

void MyExecDialog::AddAsyncProcess(MyPipedProcess *process)
{
    if ( m_running.IsEmpty() )
    {
        // we want to start getting the timer events to ensure that a
        // steady stream of idle events comes in -- otherwise we
        // wouldn't be able to poll the child process input
        m_timerIdleWakeUp.Start(500);
    }
    //else: the timer is already running

    m_running.Add(process);
}


void MyExecDialog::RemoveAsyncProcess(MyPipedProcess *process)
{
    m_running.Remove(process);

    if ( m_running.IsEmpty() )
    {
        // we don't need to get idle events all the time any more
        m_timerIdleWakeUp.Stop();
    }
}


// ----------------------------------------------------------------------------
// various helpers
// ----------------------------------------------------------------------------



void MyExecDialog::AddToOutput(wxInputStream & s)
{
    DEBUG_TRACE("");
    wxTextInputStream ts(s);
#if HUGIN_EXEC_LISTBOX
    while(s.CanRead()) {
        wxChar c = ts.GetChar();
        if (c == '\b') {
            m_currLine.RemoveLast();
        } else if (c == 0x0d) {
            // back to start of line
            m_currLine.clear();
        } else if (c == '\n') {
            // add line to listbox and start new listbox
            m_lbox->SetString(m_lbox->GetCount()-1, m_currLine);
            m_currLine.clear();
            m_lbox->Append(m_currLine);
        } else {
            m_currLine.Append(c);
        }
    }
    m_lbox->SetString(m_lbox->GetCount()-1, m_currLine);

#else
    while(s.CanRead()) {
        wxChar c = ts.GetChar();
        if (c == '\b') {
            // backspace
            if (m_textctrl->GetLastPosition() != m_lastLineStart) {
                m_textctrl->Remove(m_textctrl->GetLastPosition(), m_textctrl->GetLastPosition()+1);
            }
            /*
            if (m_output.Last() != '\n') {
                m_output.RemoveLast();
            }*/
        } else if (c == 0x0d) {
            // back to start of line
            m_textctrl->Remove(m_lastLineStart, m_textctrl->GetLastPosition()+1);
            /*
            if (m_output.Last() != '\n') {
                m_output = m_output.BeforeLast('\n') + wxT("\n");
            }
            */
        } else if (c == '\n') {
            (*m_textctrl) << c;
            m_lastLineStart = m_textctrl->GetLastPosition();
        } else {
            (*m_textctrl) << c;
        }
    }
#endif
}


void MyExecDialog::AddToOutput(wxString str)
{
#if HUGIN_EXEC_LISTBOX
    m_lbox->Append(str);
#else
    for (size_t i = 0; i < str.length(); i++) {
        wxChar c = str[i];
        if (c == '\b') {
            // backspace
            if (m_output.Last() != '\n') {
                m_output.RemoveLast();
            }
        } else if (c == 0x0d) {
            // back to start of line
            if (m_output.Last() != '\n') {
                m_output = m_output.BeforeLast('\n') + wxT("\n");
            }
        } else {
            m_output.Append(c);
        }
    }
#endif
}

void MyExecDialog::OnTimer(wxTimerEvent& WXUNUSED(event))
{
//    wxWakeUpIdle();

#ifndef HUGIN_EXEC_LISTBOX
    m_textctrl->Freeze();
#endif
    bool changed=false;
    size_t count = m_running.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        while ( m_running[n]->IsInputAvailable() )
        {
            AddToOutput(*(m_running[n]->GetInputStream()));
            changed=true;
        }
        while ( m_running[n]->IsErrorAvailable() )
        {
            AddToOutput(*(m_running[n]->GetErrorStream()));
            changed=true;
        }
    }
#ifdef HUGIN_EXEC_LISTBOX
//    m_lbox->SetSelection(m_lbox->GetCount() -1);
#else
    if (changed) {
        DEBUG_DEBUG("refreshing textctrl");
        m_textctrl->ShowPosition(m_textctrl->GetLastPosition ()-1);
        m_textctrl->SetInsertionPoint(m_textctrl->GetLastPosition()-1);
    }
    m_textctrl->Thaw();
#endif
}

void MyExecDialog::OnProcessTerminated(MyPipedProcess *process, int pid, int status)
{
    // show the rest of the output
    wxString stderr, stdout;
    while ( process->HasInput(stdout, stderr) ) {
        if (stdout.length() > 0) AddToOutput(stdout);
        if (stderr.length() > 0) AddToOutput(stderr);
        stdout.clear();
        stderr.clear();
    }

    RemoveAsyncProcess(process);
    if (status != 0) {
        wxMessageBox(_("Error while executing process"),_("Error"));
    }
    EndModal(status);
}


// ----------------------------------------------------------------------------
// MyProcess
// ----------------------------------------------------------------------------

void MyProcess::OnTerminate(int pid, int status)
{
//    wxLogStatus(m_parent, _T("Process %u ('%s') terminated with exit code %d."),
//                pid, m_cmd.c_str(), status);

    // we're not needed any more
    delete this;
}

// ----------------------------------------------------------------------------
// MyPipedProcess
// ----------------------------------------------------------------------------

bool MyPipedProcess::HasInput(wxString & stdout, wxString & stderr)
{
    bool hasInput = false;

    if ( IsInputAvailable() )
    {
        wxTextInputStream tis(*GetInputStream());

        // this assumes that the output is always line buffered
        wxString msg;
        //msg << m_cmd << _T(" (stdout): ") << tis.ReadLine();
        stdout << tis.ReadLine();

//        m_parent->GetLogListBox()->Append(msg);

        hasInput = true;
    }

    if ( IsErrorAvailable() )
    {
        wxTextInputStream tis(*GetErrorStream());

        // this assumes that the output is always line buffered
        wxString msg;
        //msg << m_cmd << _T(" (stderr): ") << tis.ReadLine();
        stderr << tis.ReadLine();

//        m_parent->GetLogListBox()->Append(msg);

        hasInput = true;
    }

    return hasInput;
}

void MyPipedProcess::OnTerminate(int pid, int status)
{

    m_parent->OnProcessTerminated(this, pid, status);

    MyProcess::OnTerminate(pid, status);
}


int MyExecuteCommandOnDialog(wxString command, wxString args, wxWindow* parent,
                             wxString title)
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
#if defined DISABLED__WXMSW__
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
#if 0
    MyExternalCmdExecDialog dlg(parent, wxID_ANY);
    dlg.ShowModal(cmdline);
    wxMessageBox(_("program finished"), _("ExecuteProcess"));
    return 0;

#else
    MyExecDialog dlg(NULL, title,
                     wxDefaultPosition, wxSize(640, 400));
    return dlg.ExecWithRedirect(cmdline);
#endif

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
    process = new HuginPipedProcess(this, cmd);

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
    process = new HuginPipedProcess(this, cmd);

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

bool HuginPipedProcess::HasInput()
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

void HuginPipedProcess::OnTerminate(int pid, int status)
{
    // show the rest of the output
    while ( HasInput() ) ;

    m_parent->SetExitCode(status);
    m_parent->EndModal(-3);

    // we're not needed any more
    delete this;
}


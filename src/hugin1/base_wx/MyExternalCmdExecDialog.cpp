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

#include "base_wx/wxPlatform.h"

#include "wx/ffile.h"
#include "wx/process.h"
#include "wx/mimetype.h"

#ifdef __WINDOWS__
    #include "wx/dde.h"
	#include <windows.h>
	#include <tlhelp32.h>	//needed to pause process on windows
#else
	#include <sys/types.h>	
	#include <signal.h>	//needed to pause on unix - kill function
	#include <unistd.h> //needed to separate the process group of make
#endif // __WINDOWS__

// Slightly reworked fix for BUG_2075064 
#ifdef __WXMAC__
	#include <iostream>
	#include <stdio.h>
	#include "wx/utils.h"
#endif

#include "MyExternalCmdExecDialog.h"
#include "hugin/config_defaults.h"

// Slightly reworked fix for BUG_2075064 
using namespace std;


// This wx internal bug is in versions of wxmac>2.8.8 on Leopard on ppc.
/*#if defined __WXMAC__ && defined __ppc__
	#define __HUGIN_WORKAROUND_BUG_2075064
	DEBUG_DEBUG("Setting the __HUGIN_WORKAROUND_BUG_2075064");
#endif
*/
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


BEGIN_EVENT_TABLE(MyExecPanel, wxPanel)
//    EVT_IDLE(MyExecDialog::OnIdle)
    EVT_TIMER(wxID_ANY, MyExecPanel::OnTimer)
END_EVENT_TABLE()


// ============================================================================
// implementation
// ============================================================================


// frame constructor
MyExecPanel::MyExecPanel(wxWindow * parent)
       : wxPanel(parent),
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
    
#ifdef __WXMAC__
    wxFont font(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#else
    wxFont font(8, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
#endif
    
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

//    topsizer->Add( new wxButton(this, wxID_CANCEL, _("Cancel")),
//                  0, wxALL | wxALIGN_RIGHT, 10);

    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );
}

void MyExecPanel::KillProcess()
{
    if (m_pidLast) {
#ifdef __WXMSW__
        DEBUG_DEBUG("Killing process " << m_pidLast << " with sigkill");
        wxKillError rc = wxProcess::Kill(m_pidLast, wxSIGKILL, wxKILL_CHILDREN);
#else
        DEBUG_DEBUG("Killing process " << m_pidLast << " with sigterm");
        wxKillError rc = wxProcess::Kill(m_pidLast, wxSIGTERM, wxKILL_CHILDREN);
#endif
        if ( rc != wxKILL_OK ) {
            static const wxChar *errorText[] =
            {
                _T(""), // no error
                _T("signal not supported"),
                _T("permission denied"),
                _T("no such process"),
                _T("unspecified error"),
            };

            wxLogError(_("Failed to kill process %ld, error %d: %s"),
                        m_pidLast, rc, errorText[rc]);
        }
    }
}

/**function to pause running process, argument pause defaults to true - to resume, set it to false*/
void MyExecPanel::PauseProcess(bool pause)
{
#ifdef __WXMSW__
	HANDLE hProcessSnapshot = NULL;
	HANDLE hThreadSnapshot = NULL; 
	PROCESSENTRY32 pEntry = {0};
	THREADENTRY32 tEntry = {0};

	//we take a snapshot of all system processes
	hProcessSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (hProcessSnapshot == INVALID_HANDLE_VALUE)
		wxLogError(_("Error pausing process %ld, code 1"),m_pidLast);
	else
	{
		pEntry.dwSize = sizeof(PROCESSENTRY32);
		tEntry.dwSize = sizeof(THREADENTRY32);
		
		//we traverse all processes in the system
		if(Process32First(hProcessSnapshot, &pEntry))
		{
			do
			{
				//we pause threads of the main (make) process and its children (nona,enblend...)
				if((pEntry.th32ProcessID == m_pidLast) || (pEntry.th32ParentProcessID == m_pidLast))
				{
					//we take a snapshot of all system threads
					hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0); 
					if (hThreadSnapshot == INVALID_HANDLE_VALUE)
						wxLogError(_("Error pausing process %ld, code 2"),m_pidLast);

					//we traverse all threads
					if(Thread32First(hThreadSnapshot, &tEntry))
					{
						do
						{
							//we find all threads of the process
							if(tEntry.th32OwnerProcessID == pEntry.th32ProcessID)
							{
								HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, tEntry.th32ThreadID);
								if(pause)
									SuspendThread(hThread);
								else
									ResumeThread(hThread);
								CloseHandle(hThread);
							}
						}while(Thread32Next(hThreadSnapshot, &tEntry));
					}
					CloseHandle(hThreadSnapshot);
				}
			}while(Process32Next(hProcessSnapshot, &pEntry));
		}
	}
	CloseHandle(hProcessSnapshot);
#else
	//send the process group a pause/cont signal
	if(pause)
		killpg(m_pidLast,SIGSTOP);
	else
		killpg(m_pidLast,SIGCONT);
#endif //__WXMSW__
}

void MyExecPanel::ContinueProcess()
{
	PauseProcess(false);
}

long MyExecPanel::GetPid()
{
	return m_pidLast;
}

int MyExecPanel::ExecWithRedirect(wxString cmd)
{
    if ( !cmd )
        return -1;

// Slightly reworked fix for BUG_2075064 
#if defined __WXMAC__ && defined __ppc__
	int osVersionMajor;
	int osVersionMinor;
	
	int os = wxGetOsVersion(&osVersionMajor, &osVersionMinor);
	
	 cout << "osVersionCheck: os is " << os << "\n"  << endl;
	 cout << "osVersionCheck: osVersionMajor = " << osVersionMajor << endl;
	 cout << "osVersionCheck: osVersionMinor = " << osVersionMinor << endl;
	if ((osVersionMajor == 0x10) && (osVersionMinor >= 0x50))
	{
		//let the child process exit without becoming zombie
    		//may do some harm to internal handling by wxWidgets, but hey it's not working anyway
    		signal(SIGCHLD,SIG_IGN);
		cout <<  "osVersionCheck: Leopard loop 1" << endl;
	}
	else
	{
		cout <<  "osVersionCheck: Tiger loop 1" << endl;
	}	
#endif
    
    MyPipedProcess *process = new MyPipedProcess(this, cmd);
    m_pidLast = wxExecute(cmd, wxEXEC_ASYNC|wxEXEC_MAKE_GROUP_LEADER, process);
    if ( m_pidLast == 0 )
    {
        wxLogError(_T("Execution of '%s' failed."), cmd.c_str());
        delete process;
        return -1;
    }
    else
    {
        AddAsyncProcess(process);
#ifndef __WINDOWS__
		//on linux we put the new process into a separate group, 
		//so it can be paused with all it's children at the same time
		setpgid(m_pidLast,m_pidLast);	
#endif
    }
    m_cmdLast = cmd;
    return 0;   
}

void MyExecPanel::AddAsyncProcess(MyPipedProcess *process)
{
    if ( m_running.IsEmpty() )
    {
        // we want to start getting the timer events to ensure that a
        // steady stream of idle events comes in -- otherwise we
        // wouldn't be able to poll the child process input
        m_timerIdleWakeUp.Start(200);
    }
    //else: the timer is already running

    m_running.Add(process);
}


void MyExecPanel::RemoveAsyncProcess(MyPipedProcess *process)
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

void MyExecPanel::AddToOutput(wxInputStream & s)
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

#ifdef HUGIN_EXEC_AVOID_STREAM_GETCHAR
    while(s.CanRead()) {
        wxString buffer = ts.ReadLine() + wxT("\n");
        while(true) {
            int pos = buffer.Find(wxChar('\b'));
            if(pos == wxNOT_FOUND)
                break;
            else if(pos == 0)
                buffer = buffer.erase(0,1);
            else //if(pos > 0)
                buffer = buffer.erase(pos-1,2);
        }
        buffer = buffer.AfterLast(wxChar(0x0d));
        m_textctrl->AppendText(buffer);
    }
    return;
#endif
    bool lastCR= false;
    wxString currLine = m_textctrl->GetRange(m_lastLineStart, m_textctrl->GetLastPosition());
    while(s.CanRead()) {
        wxChar c = ts.GetChar();
        if (c == '\b') {
            lastCR=false;
            // backspace
            if (currLine.size() > 0) {
                if (currLine.Last() != wxChar('\n') )
                    currLine.Trim();
            }
            /*
            if (m_output.Last() != '\n') {
                m_output.RemoveLast();
            }*/
        } else if (c == 0x0d) {
            lastCR=true;
#ifndef __WXMSW__
            // back to start of line
             if (currLine.Last() != wxChar('\n') ) {
                currLine = currLine.BeforeLast('\n');
                if(currLine.size() > 0) {
                    currLine.Append('\n');
                }
             }
#endif
        } else if (c == '\n') {
            currLine.Append(c);
            lastCR=false;
        } else {
#ifdef __WXMSW__
            if (lastCR) {
            // back to start of line
                if (currLine.Last() != wxChar('\n') ) {
                    currLine = currLine.BeforeLast('\n');
                    if(currLine.size() > 0) {
                        currLine.Append('\n');
                    }
                }
            }
#endif
            currLine.Append(c);
            lastCR=false;
        }
    }

    m_textctrl->Replace(m_lastLineStart, m_textctrl->GetLastPosition(), currLine);
    size_t lret = currLine.find_last_of(wxChar('\n'));
    if (lret > 0 && lret+1 < currLine.size()) {
        m_lastLineStart += lret+1;
    }
#endif
}

void MyExecPanel::OnTimer(wxTimerEvent& WXUNUSED(event))
{
//    wxWakeUpIdle();

#ifndef HUGIN_EXEC_LISTBOX
//    m_textctrl->Freeze();
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
#ifndef LINE_IO
//        m_textctrl->ShowPosition(m_textctrl->GetLastPosition());
//        m_textctrl->SetInsertionPoint(m_textctrl->GetLastPosition()-1);
#endif
    }
//    m_textctrl->Thaw();
#endif

// Slightly reworked fix for BUG_2075064
#if defined __WXMAC__ && defined __ppc__
	int osVersionMajor;
	int osVersionMinor;
	
	int os = wxGetOsVersion(&osVersionMajor, &osVersionMinor);
	
	cerr << "osVersionCheck: os is " << os << "\n"  << endl;
	cerr << "osVersionCheck: osVersionMajor = " << osVersionMajor << endl;
	cerr << "osVersionCheck: osVersionMinor = " << osVersionMinor << endl;

    	if ((osVersionMajor == 0x10) && (osVersionMinor >= 0x50))
    {	
		cerr <<  "osVersionCheck: Leopard loop 2" << endl;	
		if(m_pidLast)
		{
			if(kill((pid_t)m_pidLast,0)!=0) //if not pid exists
			{
				DEBUG_DEBUG("Found terminated process: " << (pid_t)m_pidLast)
            
				// probably should clean up the wxProcess object which was newed when the process was launched.
				// for now, nevermind the tiny memory leak... it's a hack to workaround the bug anyway
            
				//notify dialog that it's finished.
				if (this->GetParent()) {
					wxProcessEvent event( wxID_ANY, m_pidLast, 0); // assume 0 exit code
					event.SetEventObject( this );
					DEBUG_TRACE("Sending wxProcess event");   
					this->GetParent()->ProcessEvent( event );
				}
			}
		}
	}
	else
	{
		cerr <<  "osVersionCheck: Tiger loop 2" << endl;
	}
#endif
}

void MyExecPanel::OnProcessTerminated(MyPipedProcess *process, int pid, int status)
{
    DEBUG_TRACE("process terminated: pid " << pid << " exit code:" << status);
    // show the rest of the output
    AddToOutput(*(process->GetInputStream()));
    AddToOutput(*(process->GetErrorStream()));

    RemoveAsyncProcess(process);
    // send termination to other parent
    if (this->GetParent()) {
        wxProcessEvent event( wxID_ANY, pid, status);
        event.SetEventObject( this );
        DEBUG_TRACE("Sending wxProcess event");   
        this->GetParent()->GetEventHandler()->ProcessEvent( event );
    }

}

MyExecPanel::~MyExecPanel() {
#ifdef HUGIN_EXEC_LISTBOX
    delete m_lbox;
#else
    delete m_textctrl;
#endif
}

bool MyExecPanel::SaveLog(const wxString &filename)
{
    return m_textctrl->SaveFile(filename);
};

void MyExecPanel::CopyLogToClipboard()
{
    m_textctrl->SelectAll();
    m_textctrl->Copy();
};

// ----------------------------------------------------------------------------
// MyProcess
// ----------------------------------------------------------------------------

void MyProcess::OnTerminate(int pid, int status)
{
    DEBUG_TRACE("");
   
//    wxLogStatus(m_parent, _T("Process %u ('%s') terminated with exit code %d."),
//                pid, m_cmd.c_str(), status);

    // we're not needed any more
    delete this;
}

// ----------------------------------------------------------------------------
// MyPipedProcess
// ----------------------------------------------------------------------------
/*
bool MyPipedProcess::HasInput(wxString & my_stdout, wxString & my_stderr)
{
    bool hasInput = false;

    if ( IsInputAvailable() )
    {
        wxTextInputStream tis(*GetInputStream());

        // this assumes that the output is always line buffered
        wxString msg;
        //msg << m_cmd << _T(" (my_stdout): ") << tis.ReadLine();
        my_stdout << tis.ReadLine();

//        m_parent->GetLogListBox()->Append(msg);

        hasInput = true;
    }

    if ( IsErrorAvailable() )
    {
        wxTextInputStream tis(*GetErrorStream());

        // this assumes that the output is always line buffered
        wxString msg;
        //msg << m_cmd << _T(" (stderr): ") << tis.ReadLine();
        my_stderr << tis.ReadLine();

//        m_parent->GetLogListBox()->Append(msg);

        hasInput = true;
    }

    return hasInput;
}
*/

void MyPipedProcess::OnTerminate(int pid, int status)
{
    DEBUG_DEBUG("Process " << pid << " terminated with return code: " << status);
    m_parent->OnProcessTerminated(this, pid, status);

    MyProcess::OnTerminate(pid, status);
}

// ==============================================================================
// MyExecDialog

BEGIN_EVENT_TABLE(MyExecDialog, wxDialog)
    EVT_BUTTON(wxID_CANCEL,  MyExecDialog::OnCancel)
    EVT_END_PROCESS(wxID_ANY, MyExecDialog::OnProcessTerminate)
END_EVENT_TABLE()




MyExecDialog::MyExecDialog(wxWindow * parent, const wxString& title, const wxPoint& pos, const wxSize& size)
    : wxDialog(parent, wxID_ANY, title, pos, size, wxRESIZE_BORDER | wxCAPTION | wxCLOSE_BOX | wxSYSTEM_MENU)
{

    wxBoxSizer * topsizer = new wxBoxSizer( wxVERTICAL );
    m_execPanel = new MyExecPanel(this);
    m_cancelled = false;
    
    topsizer->Add(m_execPanel, 1, wxEXPAND | wxALL, 2);

    topsizer->Add( new wxButton(this, wxID_CANCEL, _("Cancel")),
                   0, wxALL | wxALIGN_RIGHT, 10);

#ifdef __WXMSW__
    // wxFrame does have a strange background color on Windows..
    this->SetBackgroundColour(m_execPanel->GetBackgroundColour());
#endif
    SetSizer( topsizer );
//    topsizer->SetSizeHints( this );
}

void MyExecDialog::OnProcessTerminate(wxProcessEvent & event)
{
    DEBUG_DEBUG("Process terminated with return code: " << event.GetExitCode());
    if(wxConfigBase::Get()->Read(wxT("CopyLogToClipboard"), 0l)==1l)
    {
        m_execPanel->CopyLogToClipboard();
    };
    if (m_cancelled) {
        EndModal(HUGIN_EXIT_CODE_CANCELLED);
    } else {
        EndModal(event.GetExitCode());
    }
}

void MyExecDialog::OnCancel(wxCommandEvent& WXUNUSED(event))
{
    DEBUG_DEBUG("Cancel Pressed");
    m_cancelled = true;
    m_execPanel->KillProcess();
}


int MyExecDialog::ExecWithRedirect(wxString cmd)
{
    if (m_execPanel->ExecWithRedirect(cmd) == -1) {
        return -1;
    }

    return ShowModal();
}

MyExecDialog::~MyExecDialog() {
    delete m_execPanel;
}

int MyExecuteCommandOnDialog(wxString command, wxString args, wxWindow* parent,
                             wxString title, bool isQuoted)
{
    if(!isQuoted)
    {
        command = hugin_utils::wxQuoteFilename(command);
    };
    wxString cmdline = command + wxT(" ") + args;
    MyExecDialog dlg(parent, title,
                     wxDefaultPosition, wxSize(640, 400));
#ifdef __WXMAC__
    dlg.CentreOnParent();
#endif
    return dlg.ExecWithRedirect(cmdline);
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

MyExternalCmdExecDialog::~MyExternalCmdExecDialog() {
    delete m_tbox;
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


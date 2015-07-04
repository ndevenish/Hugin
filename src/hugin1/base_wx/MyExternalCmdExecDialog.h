// -*- c-basic-offset: 4 -*-
/** @file MyExternalCmdExecDialog.h
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
*  License along with this software. If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

// This class is written based on 'exec' sample of wxWidgets library.


#ifndef _MYEXTERNALCMDEXECDIALOG__H
#define _MYEXTERNALCMDEXECDIALOG__H

#include <hugin_shared.h>
#include <wx/utils.h>
#include "Executor.h"

const int HUGIN_EXIT_CODE_CANCELLED = -255;

/** execute a single command in own dialog, redirect output to frame and allow canceling */
WXIMPEX int MyExecuteCommandOnDialog(wxString command, wxString args, wxWindow* parent, wxString title, bool isQuoted=false);
/** execute all commands in queue with redirection of output to frame and allow canceling 
    the queue will be delete at the end, don't use it further */
WXIMPEX int MyExecuteCommandQueue(HuginQueue::CommandQueue* queue, wxWindow* parent, const wxString& title, const wxString& comment = wxEmptyString);

// Define an array of process pointers used by MyFrame
class MyPipedProcess;
WX_DEFINE_ARRAY_PTR(MyPipedProcess *, MyProcessesArray);

class WXIMPEX MyProcessListener
{
public:
    virtual void OnProcessTerminated(MyPipedProcess *process, int pid, int status) = 0;
    virtual ~MyProcessListener() {}
};

// Define a new exec dialog
class WXIMPEX MyExecPanel : public wxPanel, public MyProcessListener
{
public:
    // ctor(s)
    MyExecPanel(wxWindow * parent);

    void KillProcess();
	void PauseProcess(bool pause = true);
	void ContinueProcess();
	long GetPid();

    int ExecWithRedirect(wxString command);
    int ExecQueue(HuginQueue::CommandQueue* queue);

    // polling output of async processes
    void OnTimer(wxTimerEvent& event);

    // for MyPipedProcess
    void OnProcessTerminated(MyPipedProcess *process, int pid, int status);

    /** save the content of the window into a given log file 
        @return true if log was saved successful */
    bool SaveLog(const wxString &filename);
    /** copy the content of the log window into the clipboard */
    void CopyLogToClipboard();
    /** display the string in the panel */
    void AddString(const wxString& s);

    virtual ~MyExecPanel();

private:

    void AddToOutput(wxInputStream & s);

    void DoAsyncExec(const wxString& cmd);

    void AddAsyncProcess(MyPipedProcess *process);

    void RemoveAsyncProcess(MyPipedProcess *process);

    int ExecNextQueue();

    // the PID of the last process we launched asynchronously
    long m_pidLast;

    wxTextCtrl *m_textctrl;
    long m_lastLineStart;

    MyProcessesArray m_running;

    // the idle event wake up timer
    wxTimer m_timerIdleWakeUp;
    // store queue
    HuginQueue::CommandQueue* m_queue;
    size_t m_queueLength;
    // if the return code of the process should be checked
    bool m_checkReturnCode;
#if wxCHECK_VERSION(3,0,0)
    wxExecuteEnv m_executeEnv;
#endif
    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxProcess-derived classes
// ----------------------------------------------------------------------------

// A specialization of MyProcess for redirecting the output
class WXIMPEX MyPipedProcess : public wxProcess
{
public:
    MyPipedProcess(MyProcessListener *parent, const wxString& cmd)
        : wxProcess(0), m_cmd(cmd)
        {
            m_parent = parent;
            Redirect();
        }

    // This is the handler for process termination events
    virtual void OnTerminate(int pid, int status);
protected:
    MyProcessListener *m_parent;
    wxString m_cmd;
};

// Define a new exec dialog, which uses MyExecPanel defined above
class WXIMPEX MyExecDialog : public wxDialog
{
public:
// ctor(s)
    MyExecDialog(wxWindow * parent, const wxString& title, const wxPoint& pos, const wxSize& size);

    void OnCancel(wxCommandEvent& event);

    int ExecWithRedirect(wxString command);
    int ExecQueue(HuginQueue::CommandQueue* queue);
    /** display the string in the panel */
    void AddString(const wxString& s);

    void OnProcessTerminate(wxProcessEvent & event);
    
    virtual ~MyExecDialog();

private:
    
    MyExecPanel * m_execPanel;
    bool m_cancelled;

    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

DECLARE_EXPORTED_EVENT_TYPE(WXIMPEX, EVT_QUEUE_PROGRESS, -1)

#endif

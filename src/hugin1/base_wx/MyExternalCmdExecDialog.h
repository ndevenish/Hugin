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
*  License along with this software; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*/

// This class is written based on 'exec' sample of wxWidgets library.


#ifndef _MYEXTERNALCMDEXECDIALOG__H
#define _MYEXTERNALCMDEXECDIALOG__H

#include <hugin_shared.h>

const int HUGIN_EXIT_CODE_CANCELLED = -255;

WXIMPEX int MyExecuteCommandOnDialog(wxString command, wxString args, wxWindow* parent, wxString title, bool isQuoted=false);

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

    // polling output of async processes
    void OnTimer(wxTimerEvent& event);

    // for MyPipedProcess
    void OnProcessTerminated(MyPipedProcess *process, int pid, int status);

    /** save the content of the window into a given log file 
        @return true if log was saved successful */
    bool SaveLog(const wxString &filename);
    /** copy the content of the log window into the clipboard */
    void CopyLogToClipboard();
    
    virtual ~MyExecPanel();

private:

    void AddToOutput(wxInputStream & s);

    void DoAsyncExec(const wxString& cmd);

    void AddAsyncProcess(MyPipedProcess *process);

    void RemoveAsyncProcess(MyPipedProcess *process);

    // the PID of the last process we launched asynchronously
    long m_pidLast;

    // last command we executed
    wxString m_cmdLast;

    wxTextCtrl *m_textctrl;
    long m_lastLineStart;

    MyProcessesArray m_running;

    // the idle event wake up timer
    wxTimer m_timerIdleWakeUp;

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
    
    void OnProcessTerminate(wxProcessEvent & event);
    
    virtual ~MyExecDialog();

private:
    
    MyExecPanel * m_execPanel;
    bool m_cancelled;

    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

#endif

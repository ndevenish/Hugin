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

class MyExternalCmdExecDialog;
class HuginPipedProcess;
WXIMPEX int MyExecuteCommandOnDialog(wxString command, wxString args, wxWindow* parent, wxString title, bool isQuoted=false);

//#define HUGIN_EXEC_LISTBOX 1

// Define an array of process pointers used by MyFrame
class MyPipedProcess;
WX_DEFINE_ARRAY_PTR(MyPipedProcess *, MyProcessesArray);


class MyPipedProcess;

class MyProcessListener
{
public:
    virtual void OnProcessTerminated(MyPipedProcess *process, int pid, int status) = 0;
    virtual ~MyProcessListener() {}
};


// Define a new exec dialog
class MyExecPanel : public wxPanel, public MyProcessListener
{
public:
    // ctor(s)
    MyExecPanel(wxWindow * parent);

    void KillProcess();
	void PauseProcess(bool pause = true);
	void ContinueProcess();
	long GetPid();

    void OnExecWithRedirect(wxCommandEvent& event);

    int ExecWithRedirect(wxString command);

    // polling output of async processes
    void OnTimer(wxTimerEvent& event);

    // for MyPipedProcess
    void OnProcessTerminated(MyPipedProcess *process, int pid, int status);
    //wxListBox *GetLogListBox() const { return m_lbox; }
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

//    wxString m_output;

#ifdef HUGIN_EXEC_LISTBOX
    wxListBox *m_lbox;
    wxString   m_currLine;
#else
    wxTextCtrl *m_textctrl;
    long m_lastLineStart;
#endif

    MyProcessesArray m_running;

    // the idle event wake up timer
    wxTimer m_timerIdleWakeUp;

    // any class wishing to process wxWidgets events must use this macro
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxProcess-derived classes
// ----------------------------------------------------------------------------

// This is the handler for process termination events
class MyProcess : public wxProcess
{
public:
    MyProcess(MyProcessListener *parent, const wxString& cmd)
        : wxProcess(0), m_cmd(cmd)
    {
        m_parent = parent;
    }
    // instead of overriding this virtual function we might as well process the
    // event from it in the frame class - this might be more convenient in some
    // cases
    virtual void OnTerminate(int pid, int status);
protected:
    MyProcessListener *m_parent;
    wxString m_cmd;
};

// A specialization of MyProcess for redirecting the output
class MyPipedProcess : public MyProcess
{
public:
    MyPipedProcess(MyProcessListener *parent, const wxString& cmd)
        : MyProcess(parent, cmd)
        {
            Redirect();
        }

    virtual void OnTerminate(int pid, int status);
};


// Define a new exec dialog, which uses MyExecPanel defined above
class MyExecDialog : public wxDialog
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

//----------



class MyExternalCmdExecDialog : public wxDialog
{
public:
    MyExternalCmdExecDialog(wxWindow* parent, 
                            wxWindowID id, 
                            const wxString& title = _("Command Line Progress"), 
                            const wxPoint& pos = wxDefaultPosition, 
                            const wxSize& size = wxDefaultSize, 
#ifdef __WXMAC__
                            long style = wxRESIZE_BORDER|wxFRAME_FLOAT_ON_PARENT|wxMINIMIZE_BOX,
#else
                            long style = wxDEFAULT_DIALOG_STYLE,
#endif
                            const wxString& name = wxT("externalCmDialogBox"));
    
    int ShowModal(const wxString &cmd);
	int Execute(const wxString & cmd);
	int GetExitCode();
    void SetExitCode(int ret);
    void OnTimer(wxTimerEvent& WXUNUSED(event));
    void OnIdle(wxIdleEvent& event);
    //wxListBox *GetLogListBox() const { return m_lbox; }
    wxTextCtrl *GetLogTextBox() const { return m_tbox; }
    virtual ~MyExternalCmdExecDialog();
    
private:
    //wxListBox *m_lbox;
    wxTextCtrl *m_tbox;
    wxTimer m_timerIdleWakeUp;
    HuginPipedProcess *process;
    long processID;
    int m_exitCode;

    DECLARE_EVENT_TABLE()
};

//----------

class HuginPipedProcess : public wxProcess
{
public:
    HuginPipedProcess(MyExternalCmdExecDialog *parent, const wxString& cmd)
    : wxProcess(parent), m_cmd(cmd)
    {
        m_parent = parent;
        Redirect();
    }
    
    virtual void OnTerminate(int pid, int status);
    virtual bool HasInput();
    
protected:
    MyExternalCmdExecDialog *m_parent;
    wxString m_cmd;
};

#endif

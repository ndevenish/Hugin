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
#endif

class MyExternalCmdExecDialog;
class MyPipedProcess;
int MyExecuteCommandOnDialog(wxString command, wxString args, wxWindow* parent);

//----------

class MyExternalCmdExecDialog : public wxDialog
{
public:
    MyExternalCmdExecDialog(wxWindow* parent, 
                            wxWindowID id, 
                            const wxString& title = _("Command Line Progress"), 
                            const wxPoint& pos = wxDefaultPosition, 
#ifdef __WXMAC__
                            const wxSize& size = wxDefaultSize, 
#else
                            const wxSize& size = wxSize(650,480),
#endif
                            long style = wxRESIZE_BORDER|wxFRAME_FLOAT_ON_PARENT|wxMINIMIZE_BOX,
                            const wxString& name = wxT("externalCmDialogBox"));
    
    int ShowModal(const wxString &cmd);
	int Execute(const wxString & cmd);
	int GetExitCode();
    void SetExitCode(int ret);
    void OnTimer(wxTimerEvent& WXUNUSED(event));
    void OnIdle(wxIdleEvent& event);
    //wxListBox *GetLogListBox() const { return m_lbox; }
    wxTextCtrl *GetLogTextBox() const { return m_tbox; }
    
private:
    //wxListBox *m_lbox;
    wxTextCtrl *m_tbox;
    wxTimer m_timerIdleWakeUp;
    MyPipedProcess *process;
    long processID;
    int m_exitCode;

    DECLARE_EVENT_TABLE()
};

//----------

class MyPipedProcess : public wxProcess
{
public:
    MyPipedProcess(MyExternalCmdExecDialog *parent, const wxString& cmd)
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

// -*- c-basic-offset: 4 -*-

/** @file RunStitchFrame.h
 *
 *  @brief Batch processor for Hugin
 *
 *  @author Marko Kuder <marko.kuder@gmail.com>
 *
 *  $Id: RunStitchFrame.h 3322 2008-08-16 5:00:07Z mkuder $
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

#ifndef RUNSTITCHFRAME_H
#define RUNSTITCHFRAME_H
#include "panoinc_WX.h"

#include <wx/wfstream.h>

#include "base_wx/RunStitchPanel.h"
#include "base_wx/huginConfig.h"
#include "base_wx/platform.h"

// somewhere SetDesc gets defined.. this breaks wx/cmdline.h on OSX
#ifdef SetDesc
#undef SetDesc
#endif

using namespace vigra;
using namespace std;

class RunStitchFrame: public wxFrame
{
public:
    //Constructor
    RunStitchFrame(wxWindow* parent, const wxString& title, const wxPoint& pos, const wxSize& size);  //ProjectArray projList, wxListBox *projListBox);

    /** Starts stitching of project file */
    bool StitchProject(wxString scriptFile, wxString outname);
    /** starts assistant of project file */
    bool DetectProject(wxString scriptFile);
    /** Returns process ID of running process */
    int GetProcessId();
    /** Gets project id from batch */
    int GetProjectId();
    /** Sets process ID of running process (if running process from outside) */
    void SetProcessId(int pid);
    /** Sets project id from batch */
    void SetProjectId(int id);
    /** save the content of the window into a given log file
        @return true if log was saved successful */
    bool SaveLog(const wxString& filename);


    /** Cancels project execution - kills process */
    void OnCancel(wxCommandEvent& event);
    RunStitchPanel* m_stitchPanel;

private:

    wxEvtHandler* m_evtParent;
    bool m_isStitching;
    bool m_isDetecting;
    int m_projectId;
    int m_pid;
    //Called in GUI application when process terminates
    void OnProcessTerminate(wxProcessEvent& event);

    DECLARE_EVENT_TABLE()
};

// event ID's for RunStitchPanel
enum
{
    ID_Quit = 1,
    ID_About
};

#endif //RUNSTITCHFRAME_H

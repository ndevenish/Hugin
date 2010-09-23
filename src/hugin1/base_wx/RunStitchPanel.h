// -*- c-basic-offset: 4 -*-

/** @file RunStitchFrame.h
 *
 *  @brief Stitch a pto project file, with GUI output etc.
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id: hugin_stitch_project.cpp 2705 2008-01-27 19:56:06Z ippei $
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

#ifndef RUN_STITCH_FRAME_H
#define RUN_STITCH_FRAME_H

#include <hugin_shared.h>
#include <vector>
#include <set>
#include <functional>
#include <utility>
#include <string>

#include <PT/Panorama.h>

#include <algorithms/panorama_makefile/PanoramaMakefilelibExport.h>
#include <algorithms/assistant_makefile/AssistantMakefilelibExport.h>

#include "MyExternalCmdExecDialog.h"

struct WXIMPEX StitchProjectEntry
{
    StitchProjectEntry(wxString script, wxString output)
    : scriptFile(script), outputPrefix(output), finished(false),
      error(false)   
    {
    }

    wxString scriptFile;
    wxString outputPrefix;
    bool finished;
    bool error;
};

WXIMPEX wxString getGNUMakeCmd(const wxString& args);

class WXIMPEX RunStitchPanel: public wxPanel
{
public:
    RunStitchPanel(wxWindow * parent);

    bool StitchProject(wxString scriptFile, wxString outname,
                       HuginBase::PanoramaMakefilelibExport::PTPrograms progs);
    bool DetectProject(wxString scriptFile, 
                       HuginBase::AssistantMakefilelibExport::AssistantPrograms progs);
    void CancelStitch();
	bool IsPaused();
	void SetOverwrite(bool over = true);
	void PauseStitch();
	void ContinueStitch();
	long GetPid();
    /** save the content of the window into a given log file 
        @return true if log was saved successful */
    bool SaveLog(const wxString &filename);

private:
	bool m_paused;
	bool m_overwrite;
    wxString m_currentPTOfn;
    wxString m_currentMakefn;
    void OnProcessTerminate(wxProcessEvent & event);

    MyExecPanel * m_execPanel;

    DECLARE_EVENT_TABLE()
};

#endif

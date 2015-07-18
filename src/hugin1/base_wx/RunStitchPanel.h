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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
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

#include <panodata/Panorama.h>

#include "MyExternalCmdExecDialog.h"

class WXIMPEX RunStitchPanel: public wxPanel
{
public:
    explicit RunStitchPanel(wxWindow * parent);

    bool StitchProject(const wxString& scriptFile, const wxString& outname, const wxString& userDefinedOutput = wxEmptyString);
    bool DetectProject(const wxString& scriptFile);
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
    wxArrayString m_tempFiles;
    wxString m_oldCwd;
    void OnProcessTerminate(wxProcessEvent & event);

    MyExecPanel * m_execPanel;

    DECLARE_EVENT_TABLE()
};

#endif

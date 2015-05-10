// -*- c-basic-offset: 4 -*-

/** @file ProgressStatusBar.h
 *
 *  @brief definition of statusbar with progress indicator
 *
 *  @author T. Modes
 *
 */

/*
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

#ifndef PROGRESSSTATUSBAR_H
#define PROGRESSSTATUSBAR_H

/** class for showing a status bar with progress, the progress bar is always in the last field of the statusbar */
#include <wx/statusbr.h>
#include <wx/gauge.h>

class ProgressStatusBar : public wxStatusBar
{
public:
    /** constructor, create the gauge internal */
    ProgressStatusBar(wxWindow *parent, wxWindowID id, long style = wxST_SIZEGRIP, const wxString &name = wxT("statusBar"));
    /** destructor, clean up all stuff */
    ~ProgressStatusBar();
    /** size change handler, correctly position gauge when size has changed */
    void OnSize(wxSizeEvent &event);
    /** update progress bar
        @param progress sets the progress, range 0 - 100, a value of -1 will hide the progress control */
    void SetProgress(int progress);
    /** return current progress value, should be in range 0 - 100, or -1 if the progress gauge is hidden */
    int GetProgress();

private:
    wxGauge *m_progress;
    int m_progressValue;

    DECLARE_EVENT_TABLE();
};

#endif // PROGRESSSTATUSBAR_H

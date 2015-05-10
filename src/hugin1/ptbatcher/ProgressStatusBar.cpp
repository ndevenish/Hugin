// -*- c-basic-offset: 4 -*-

/** @file ProgressStatusBar.cpp
 *
 *  @brief declaration of statusbar with progress indicator
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

#include "ProgressStatusBar.h"

// Event table
BEGIN_EVENT_TABLE(ProgressStatusBar, wxStatusBar)
EVT_SIZE(ProgressStatusBar::OnSize)
END_EVENT_TABLE()

ProgressStatusBar::ProgressStatusBar(wxWindow *parent, wxWindowID id, long style, const wxString &name) : wxStatusBar(parent, id, style, name)
{
    m_progress = new wxGauge(this, -1, 100, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL);
    SetProgress(-1);
}

ProgressStatusBar::~ProgressStatusBar()
{
    if (m_progress)
    {
        delete m_progress;
    };
}

void ProgressStatusBar::OnSize(wxSizeEvent& event)
{
    if (GetFieldsCount() > 0)
    {
        wxRect r;
        GetFieldRect(GetFieldsCount() - 1, r);
        r.Deflate(2, 2);
        m_progress->SetSize(r);
    };
}

void ProgressStatusBar::SetProgress(int progress)
{
    m_progressValue = progress;
    m_progress->Show(m_progressValue >= 0);
    // SetValue expects values >=0 and < max value, but we are using -1 to indicate that progress bar is hidden
    m_progress->SetValue(m_progressValue < 0 ? 0 : m_progressValue);
};

int ProgressStatusBar::GetProgress()
{
    return m_progressValue;
};

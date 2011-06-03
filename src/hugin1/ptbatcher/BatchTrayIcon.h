// -*- c-basic-offset: 4 -*-

/** @file BatchTrayIcon.h
 *
 *  @brief definition of tray/task bar icon for PTBatcherGUI
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
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#ifndef BATCHTRAYICON_H
#define BATCHTRAYICON_H

/** class for showing a taskbar/tray icon */
#include <wx/taskbar.h>
class BatchTaskBarIcon : public wxTaskBarIcon
{
protected:
    /** handler if double clicked on taskbar icon, opens window */
    void OnLeftButtonDClick(wxTaskBarIconEvent&);
    /** handler to open window */
    void OnShowGUI(wxCommandEvent& e);
    /** handler to start batch */
    void OnStartBatch(wxCommandEvent& e);
    /** handler to pause batch */
    void OnPauseBatch(wxCommandEvent& e);
    /** handler to stop batch */
    void OnStopBatch(wxCommandEvent& e);
    /** handler to exit PTBatcherGUI */
    void OnExit(wxCommandEvent& e);
    /** handler to adding a project to stitching queue */
    void OnAddProject(wxCommandEvent& e);
    /** creates the popup menu */
    virtual wxMenu *CreatePopupMenu();

    DECLARE_EVENT_TABLE()
};

#if defined __WXMSW__ && wxUSE_TASKBARICON_BALLOONS && wxCHECK_VERSION(2,9,0)
// wxMSW, version 2.9 offers a native balloon for the traybar notification
// we need to implement a own version for the other systems only
#else
#include <wx/frame.h>
#include <wx/timer.h>

/** class to show a taskbar balloon */
// idea from http://wiki.wxwidgets.org/WxTaskBarIcon
class TaskBarBalloon : public wxFrame
{
    public:
        TaskBarBalloon(wxString sTitle, wxString sMessage);
        virtual ~TaskBarBalloon();
        /** timer to close window */
        void OnTimerTick(wxTimerEvent & e);
        /** click on the balloon */
        void OnClick(wxMouseEvent & e);
        /** key down event in the balloon */
        void OnKeyDown(wxKeyEvent & e);
 
        /** display the baloon and run the timer */
        void showBalloon(unsigned int iTimeout);
    private:
        wxTimer * m_timer;
        DECLARE_EVENT_TABLE();
};
#endif

#endif // BATCHTRAYICON_H

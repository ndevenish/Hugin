// -*- c-basic-offset: 4 -*-

/** @file BatchTrayIcon.cpp
 *
 *  @brief declaration of tray/task bar icon for PTBatcherGUI
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

#include "BatchTrayIcon.h"
#include <wx/app.h>
#include <wx/menu.h>
#include "PTBatcherGUI.h"

enum
{
    ID_SHOWGUI=wxID_HIGHEST+101,
    ID_START=wxID_HIGHEST+102,
    ID_PAUSE=wxID_HIGHEST+103,
    ID_STOP=wxID_HIGHEST+104,
    ID_ADDPROJECT=wxID_HIGHEST+105,
    ID_ADDPROJECTASSISTANT=wxID_HIGHEST+106,
    ID_EXIT=wxID_HIGHEST+120
};

BEGIN_EVENT_TABLE(BatchTaskBarIcon, wxTaskBarIcon)
    EVT_TASKBAR_LEFT_DCLICK  (BatchTaskBarIcon::OnLeftButtonDClick)
    EVT_MENU(ID_SHOWGUI, BatchTaskBarIcon::OnShowGUI)
    EVT_MENU(ID_START, BatchTaskBarIcon::OnStartBatch)
    EVT_MENU(ID_PAUSE, BatchTaskBarIcon::OnPauseBatch)
    EVT_MENU(ID_STOP, BatchTaskBarIcon::OnStopBatch)
    EVT_MENU(ID_ADDPROJECT, BatchTaskBarIcon::OnAddProject)
    EVT_MENU(ID_ADDPROJECTASSISTANT, BatchTaskBarIcon::OnAddProjectToAssistant)
    EVT_MENU(ID_EXIT, BatchTaskBarIcon::OnExit)
END_EVENT_TABLE()

// Overridables
wxMenu* BatchTaskBarIcon::CreatePopupMenu()
{
    wxMenu* menu = new wxMenu;
    menu->Append(ID_SHOWGUI,_("&Show window"));
    menu->AppendSeparator();
    bool isRunning=wxGetApp().GetFrame()->IsRunning();
    menu->Append(ID_START,_("Start batch"));
    menu->Enable(ID_START,!isRunning);
    if(wxGetApp().GetFrame()->IsPaused())
    {
        menu->Append(ID_PAUSE,_("Continue batch"));
    }
    else
    {
        menu->Append(ID_PAUSE,_("Pause batch"));
    };
    menu->Enable(ID_PAUSE,isRunning);
    menu->Append(ID_STOP,_("Stop batch"));
    menu->Enable(ID_STOP,isRunning);
    menu->AppendSeparator();
    menu->Append(ID_ADDPROJECT,_("Add project to stitching queue..."));
    menu->Append(ID_ADDPROJECTASSISTANT,_("Add project to assistant queue..."));
#ifndef __WXMAC_OSX__
    /*Mac has built-in quit menu*/
    menu->AppendSeparator();
    menu->Append(ID_EXIT, _("E&xit"));
#endif
    return menu;
}

void BatchTaskBarIcon::OnShowGUI(wxCommandEvent& e)
{
    wxGetApp().GetFrame()->Show(true);
    wxGetApp().GetFrame()->Iconize(false);
    wxGetApp().GetFrame()->UpdateBatchVerboseStatus();
};

void BatchTaskBarIcon::OnStartBatch(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_TOOL_CLICKED ,XRCID("tool_start"));
    wxGetApp().GetFrame()->GetEventHandler()->AddPendingEvent(ev);
};

void BatchTaskBarIcon::OnPauseBatch(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_TOOL_CLICKED ,XRCID("tool_pause"));
    wxGetApp().GetFrame()->GetEventHandler()->AddPendingEvent(ev);
};

void BatchTaskBarIcon::OnStopBatch(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_TOOL_CLICKED ,XRCID("tool_cancel"));
    wxGetApp().GetFrame()->GetEventHandler()->AddPendingEvent(ev);
};

void BatchTaskBarIcon::OnAddProject(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED ,XRCID("menu_add"));
    wxGetApp().GetFrame()->GetEventHandler()->AddPendingEvent(ev);
};

void BatchTaskBarIcon::OnAddProjectToAssistant(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED ,XRCID("menu_add_assistant"));
    wxGetApp().GetFrame()->GetEventHandler()->AddPendingEvent(ev);
};

void BatchTaskBarIcon::OnExit(wxCommandEvent& e)
{
    wxCommandEvent ev(wxEVT_COMMAND_MENU_SELECTED  ,XRCID("menu_exit"));
    wxGetApp().GetFrame()->GetEventHandler()->AddPendingEvent(ev);
};

void BatchTaskBarIcon::OnLeftButtonDClick(wxTaskBarIconEvent& e)
{
    wxCommandEvent dummy;
    OnShowGUI(dummy);
};

#if defined __WXMSW__ && wxUSE_TASKBARICON_BALLOONS && wxCHECK_VERSION(2,9,0)
// wxMSW, version 2.9 offers a native balloon for the traybar notification
// we need to implement a own version for the other systems
#else
enum
{
    TIMER_BALLOON=wxID_HIGHEST+207,
};
//declaration of the balloon tool tip
BEGIN_EVENT_TABLE(TaskBarBalloon, wxFrame)
    EVT_LEFT_DOWN(TaskBarBalloon::OnClick)
    EVT_KEY_DOWN(TaskBarBalloon::OnKeyDown)
    EVT_TIMER(TIMER_BALLOON,TaskBarBalloon::OnTimerTick)
END_EVENT_TABLE()

TaskBarBalloon::TaskBarBalloon(wxString sTitle, wxString sMessage)
    : wxFrame(NULL,-1,wxT("no title"),wxDefaultPosition,wxDefaultSize,wxNO_BORDER | wxSTAY_ON_TOP | wxFRAME_SHAPED | wxFRAME_NO_TASKBAR)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticText* title = new wxStaticText(this, -1, sTitle);
    wxFont titleFont = GetFont();
    titleFont.SetWeight(wxFONTWEIGHT_BOLD);
    title->SetFont(titleFont);
    title->SetBackgroundColour(GetBackgroundColour());
    mainSizer->Add(title,0,wxEXPAND | wxTOP | wxLEFT | wxRIGHT, 5);
    wxStaticText* text = new wxStaticText(this, -1, sMessage);
    text->SetBackgroundColour(GetBackgroundColour());
    mainSizer->Add(text,1,wxEXPAND | wxBOTTOM | wxLEFT | wxRIGHT, 5);
    SetSizer(mainSizer);
    mainSizer->SetSizeHints( this );

    m_timer = new wxTimer(this,TIMER_BALLOON);
    // here, we try to align the frame to the right bottom corner
    Center();
    int iX = 0, iY = 0;
    GetPosition( &iX, &iY );
    iX = (iX * 2) - 2;
    iY = (iY * 2) - 2;
    Move( iX, iY );
}

TaskBarBalloon::~TaskBarBalloon()
{
    delete m_timer;
};

/** closing frame at end of timeout */
void TaskBarBalloon::OnTimerTick(wxTimerEvent& e)
{
    Destroy();
}

void TaskBarBalloon::OnClick(wxMouseEvent& e)
{
    Destroy();
};

void TaskBarBalloon::OnKeyDown(wxKeyEvent& e)
{
    Destroy();
};

/** showing frame and running timer */
void TaskBarBalloon::showBalloon(unsigned int iTimeout)
{
    Show(false);
    Show(true);
    m_timer->Start(iTimeout,wxTIMER_ONE_SHOT);
}
#endif

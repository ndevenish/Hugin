// -*- c-basic-offset: 4 -*-

/** @file TextKillFocusHandler.cpp
 *
 *  @brief implementation of TextKillFocusHandler Class
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
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

#include <wx/wxprec.h>
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

// For all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "hugin/TextKillFocusHandler.h"

#include "common/utils.h"


BEGIN_EVENT_TABLE(TextKillFocusHandler, wxEvtHandler)
    EVT_KILL_FOCUS(TextKillFocusHandler::OnKillFocus)
END_EVENT_TABLE()


TextKillFocusHandler::~TextKillFocusHandler()
{

}

void TextKillFocusHandler::OnKillFocus(wxFocusEvent & e)
{
    DEBUG_TRACE("Control ID:" << e.m_id);
    // create a text changed event
    // need to get id of the eve
    wxCommandEvent cmdEvt(wxEVT_COMMAND_TEXT_ENTER, e.m_id);
    cmdEvt.m_eventObject = e.m_eventObject;
    m_parent->ProcessEvent(cmdEvt);
    e.Skip();
}

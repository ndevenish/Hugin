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

#include <config.h>
#include "panoinc_WX.h"
#include "panoinc.h"
#include "hugin/TextKillFocusHandler.h"

BEGIN_EVENT_TABLE(TextKillFocusHandler, wxEvtHandler)
    EVT_KILL_FOCUS(TextKillFocusHandler::OnKillFocus)
    EVT_TEXT_ENTER(-1, TextKillFocusHandler::OnTextEnter)
    EVT_TEXT(-1, TextKillFocusHandler::OnTextChange)
END_EVENT_TABLE()


TextKillFocusHandler::~TextKillFocusHandler()
{

}

void TextKillFocusHandler::OnKillFocus(wxFocusEvent & e)
{
    DEBUG_TRACE("Control ID:" << e.GetId());
    // create a text changed event
    // need to get id of the eve
    if (dirty) {
        DEBUG_DEBUG("forwarding focus change");
        wxCommandEvent cmdEvt(wxEVT_COMMAND_TEXT_ENTER, e.GetId());
        cmdEvt.SetEventObject(e.GetEventObject());
        m_parent->GetEventHandler()->ProcessEvent(cmdEvt);
        dirty = false;
    }
    e.Skip();
}

void TextKillFocusHandler::OnTextEnter(wxCommandEvent & e)
{
    DEBUG_TRACE("Control ID:" << e.GetId());
    // create a text changed event
    // need to get id of the event
    if (dirty) {
        // let the event through
        dirty = false;
        e.Skip();
    } else {
        // do not skip the event -> block
    }
}


void TextKillFocusHandler::OnTextChange(wxCommandEvent & e)
{
    DEBUG_TRACE("Control ID:" << e.GetId());
    // check if it was an enter event.
    DEBUG_DEBUG("event: int: " << e.GetInt() << "  sel: " << e.GetSelection()
                << "  string: " << e.GetString().mb_str(wxConvLocal));
    dirty = true;
    e.Skip();
}



// -*- c-basic-offset: 4 -*-
/** @file TextKillFocusHandler.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _TEXTKILLFOCUSHANDLER_H
#define _TEXTKILLFOCUSHANDLER_H


/** Handle EVT_KILL_FOCUS and convert it to a EVT_TEXT_ENTER event
 *
 *  This is needed to be notified of changes to a wxTextCtrl when
 *  the focus changes (click somewhere else, tab movement).
 *
 */
class TextKillFocusHandler : public wxEvtHandler
{
public:

    /** ctor.
     *  @param parent that will receive the EVT_TEXT_ENTER event.
     *         to indicate the change
     */
    TextKillFocusHandler(wxWindow * parent)
        : m_parent(parent), dirty(false)
        { }

    /** dtor.
     */
    virtual ~TextKillFocusHandler();

    void OnKillFocus(wxFocusEvent & e);
    void OnTextChange(wxCommandEvent & e);
    void OnTextEnter(wxCommandEvent & e);

private:
    wxWindow * m_parent;
    
    bool dirty;

    DECLARE_EVENT_TABLE()

};



#endif // _TEXTKILLFOCUSHANDLER_H

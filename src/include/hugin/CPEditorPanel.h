// -*- c-basic-offset: 4 -*-
/** @file CPEditorPanel.h
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

#ifndef _CPEDITORPANEL_H
#define _CPEDITORPANEL_H



//-----------------------------------------------------------------------------
// Headers
//-----------------------------------------------------------------------------

#include "wx/panel.h"

// forward declarations
class CPImageCtrl;
class CPEvent;

/** control point editor panel.
 *
 *  This panel is used to create/change/edit control points
 *
 *  @todo support control lines
 */
class CPEditorPanel : public wxPanel
{
public:

    /** ctor.
     */
    CPEditorPanel(wxWindow * parent);

    /** dtor.
     */
    virtual ~CPEditorPanel();

    void setLeftImage(wxImage & img);
    void setRightImage(wxImage & img);

private:

    void OnMyButtonClicked(wxCommandEvent &e);
    void OnCPEvent(CPEvent &ev);

    // my data
    CPImageCtrl * m_leftImg, *m_rightImg;

    // needed for receiving events.
    DECLARE_EVENT_TABLE();
};



#endif // _CPEDITORPANEL_H

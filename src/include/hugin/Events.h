// -*- c-basic-offset: 4 -*-

/** @file Events.h
 *
 *  @brief small event handling Class
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
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

#ifndef _EVENTS_H
#define _EVENTS_H


#include <wx/xrc/xmlres.h>          // XRC XML resouces
#include "hugin/config.h"
#include "hugin/List.h"
#include "hugin/LensPanel.h"
#include "hugin/ImagesPanel.h"
#include "hugin/MainFrame.h"
#include "hugin/huginApp.h"

// ImagesPanel
extern    ImagesPanel* images_panel;
// LensPanel
extern    LensEdit* lens_edit;
// LensPanel images list
extern    List* images_list2;
// Image Preview
extern ImgPreview *canvas;

// Define a custom event handler / taken from wxWindows samples
class MyEvtHandler : public wxEvtHandler
{
public:
    MyEvtHandler(size_t level) { m_level = level; }

    void ToImagesPanel(wxListEvent& event)
    {
        DEBUG_TRACE("");
        images_panel->SetImages (event);

        // if we don't skip the event, the other event handlers won't get it:
        // try commenting out this line and see what changes
        event.Skip(true);
    }

    void ToLensPanel(wxListEvent& event)
    {
        DEBUG_TRACE("");
        lens_panel->LensChanged (event);

        // if we don't skip the event, the other event handlers won't get it:
        // try commenting out this line and see what changes
        event.Skip(true);
    }

private:
    size_t m_level;

    DECLARE_EVENT_TABLE()
};

/*BEGIN_EVENT_TABLE(MyEvtHandler, wxEvtHandler)
    EVT_LIST_ITEM_SELECTED (XRCID("images_list2_unknown"), MyEvtHandler::ToLensPanel)
END_EVENT_TABLE()

*/

//------------------------------------------------------------------------------
#endif // _EVENTS_H

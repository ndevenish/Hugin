// -*- c-basic-offset: 4 -*-
/** @file List.h
 *
 *  @author Kai-Uwe Behrmann <web@tiscali.de>
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

#ifndef _LIST_H
#define _LIST_H


#include "wx/frame.h"
#include "wx/dnd.h"
#include "wx/listctrl.h"

#include "PT/Panorama.h"
using namespace PT;


class List: public wxListCtrl, public PT::PanoramaObserver
{
 public:
    List( wxWindow * win, Panorama * pano );
    ~List(void) ;

    /** multi use list.
     *
     *
     */
    virtual void panoramaChanged(PT::Panorama &pano);

 private:
    // event handlers
    void OnAddItem(wxCommandEvent & e);
    void OnRemoveItem(wxCommandEvent & e);
    // Here we select the preview image
//    void Change ( wxListEvent & e );
    void Change ( wxMouseEvent & e );

    // the model
    Panorama &pano;

    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------

#endif // _LIST_H

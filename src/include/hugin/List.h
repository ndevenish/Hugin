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
#include "hugin/MainFrame.h"

using namespace PT;

/** kind of list.
 *
 *  Add here the new layout an call List with it.
 */
enum {
    images_layout,
    lens_layout
};

/** multi use list.
 *
 *  This list shall contain the overall handling of datas in lists,
 *  moving selecting, drag'n drop and contain several special layouts.
 *  Layouts are then selected by the contructor variable list_layout.
 * 
 *  @todo   make the listcontents editable -> wxListCtrl->wxGrid?
 */
class List: public wxListCtrl, public PT::PanoramaObserver//, public MainPanel
{
 public:
    List( wxWindow * win, Panorama * pano , int list_layout);
    ~List(void) ;

    /** receive the update signal and start an check.
     */
//    virtual void panoramaChanged(PT::Panorama &pano);
    /** receive the update signal and start an check.
     */
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** says wich item in single selection List(lens) is selected
     */
    int GetSelectedImage(void)
        {
            return selectedItem;
        };

 private:
    // event handlers
    // @todo   remove
    void OnAddItem(wxCommandEvent & e);
    void OnRemoveItem(wxCommandEvent & e);
    void itemSelected ( wxListEvent & e );
    // Here we select the preview image
    void Change ( wxMouseEvent & e );

    // the model
    Panorama &pano;

    /** reminder for the object
     *
     *  What shall the object beheave like?
     */
    int list_layout;

    // wich item is selected?
    int selectedItem;

    DECLARE_EVENT_TABLE()
};


#endif // _LIST_H

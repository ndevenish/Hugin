// -*- c-basic-offset: 4 -*-
/** @file LensPanel.h
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

#ifndef _LENSPANEL_H
#define _LENSPANEL_H


#include "wx/frame.h"
#include "wx/dnd.h"
#include "wx/listctrl.h"

#include "PT/Panorama.h"
#include "hugin/MainFrame.h"
#include "hugin/List.h"

using namespace PT;

/** Image Preview
 *
 *  Reach the ImgPreview through this pointer globally to update only. 
 *  really needed here?
 */
extern ImgPreview *canvas;

/** Define the second the Lens panel
 *
 *  - the second for lens selection to images
 *    
 */
class LensPanel: public wxPanel, public PT::PanoramaObserver
{
 public:
    LensPanel( wxWindow *parent, const wxPoint& pos, const wxSize& size,
                 Panorama * pano );
    ~LensPanel(void) ;

    /** this is called whenever the panorama has changed.
     *
     *  This function must now update all the gui representations
     *  of the panorama to display the new state.
     *
     *  Functions that change the panororama must not update
     *  the GUI directly. The GUI should always be updated
     *  to reflect the current panorama state in this function.
     *
     *  This avoids unnessecary close coupling between the
     *  controller and the view (even if they sometimes
     *  are in the same object). See model view controller
     *  pattern.
     *  
     *  @todo   react on different update signals more special
     */
    virtual void panoramaChanged(PT::Panorama &pano);

 private:
    // event handlers
    void OnAddLens(wxCommandEvent & e);
    void OnRemoveLens(wxCommandEvent & e);
    // Here we select the preview image
    void ChangePreview ( wxListEvent & e );

    // the model
    Panorama &pano;

    // pointer to the list control
    List* images_list2;

    DECLARE_EVENT_TABLE()
};


#endif // _LENSPANEL_H

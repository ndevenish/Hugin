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

class ImagesListLens;

/** Define the second the Lens panel
 *
 *  - the second for lens selection to images
 *
 *  @note currently only one lens per panorama is supported.
 *
 *  @todo add support multiple lenses
 *
 */
class LensPanel: public wxPanel, public PT::PanoramaObserver
{
 public:
    LensPanel( wxWindow *parent, const wxPoint& pos, const wxSize& size,
                 Panorama * pano );
    virtual ~LensPanel(void) ;

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
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** update the edit Lens values with values from the
     *  selected image.
     */
    void UpdateLensDisplay (unsigned int imgNr);

    /** adjust the center of the image */
    void SetCenter (wxCommandEvent & e);

 private:
    // a window event
    void FitParent(wxSizeEvent & e);

    // event handlers
    /**  changes lens type */
    void LensTypeChanged (wxCommandEvent & e);
    /**  computes & updates HFOV */
    void focalLengthChanged(wxCommandEvent & e);

    /**  handlers for all other variables */
    void OnVarChanged(wxCommandEvent & e);
    /**  for all other variables */
    void OnVarInheritChanged(wxCommandEvent & e);

    /** catches changes to the list selection */
    void ListSelectionChanged(wxListEvent& e);


    // the model
    Panorama &pano;

    ImagesListLens * images_list;

    // Lens to change settings
    unsigned int m_editLensNr;
    // image that has been changed.
    unsigned int m_editImageNr;

    void updateHFOV(void);

    DECLARE_EVENT_TABLE()
};


#endif // _LENSPANEL_H

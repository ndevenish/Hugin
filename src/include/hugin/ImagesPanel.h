// -*- c-basic-offset: 4 -*-
/** @file ImagesPanel.h
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

#ifndef _IMAGESPANEL_H
#define _IMAGESPANEL_H


#include "wx/frame.h"
#include "wx/dnd.h"
#include "wx/listctrl.h"

#include "PT/Panorama.h"
#include "hugin/MainFrame.h"

using namespace PT;

// forward declarations, to save the #include statements
class CPEditorPanel;

// Define a new area
class ImgPreview;

/// Define the first panel - the one for image selection into Panorama
class ImagesPanel: public wxPanel, public PT::PanoramaObserver
{
 public:
    ImagesPanel( wxWindow *parent, const wxPoint& pos, const wxSize& size,
                 Panorama * pano );
    ~ImagesPanel(void) ;

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
     */
    virtual void panoramaChanged(PT::Panorama &pano);

 private:
    // event handlers
    void OnAddImages(wxCommandEvent & e);
    void OnRemoveImages(wxCommandEvent & e);
    // Here we select the preview image
    void ChangePreview ( wxListEvent & e );

    // the model
    Panorama &pano;

    // Image Preview
    ImgPreview *canvas;

    // pointer to the list control
    wxListCtrl* images_list;

    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------

// Define a new canvas which can receive some events
class ImgPreview: public wxScrolledWindow
{
 public:
    ImgPreview(wxWindow *parent, const wxPoint& pos, const wxSize& size);
    ~ImgPreview(void) ;

    void OnDraw(wxDC& dc);
    //void OnPaint(wxPaintEvent& event);

 private:
    // Here we select the preview image
    void ChangePreview ( wxMouseEvent & e );

    DECLARE_EVENT_TABLE()
};

#endif // _IMAGESPANEL_H

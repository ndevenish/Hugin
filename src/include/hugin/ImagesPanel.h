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
#include "hugin/List.h"

using namespace PT;

// forward declarations, to save the #include statements
class CPEditorPanel;

// Define a new area
class ImgPreview;

/** Image Preview
 *
 *  Reach the ImgPreview through this pointer globally to update only. 
 */
extern ImgPreview *canvas;

/** hugins first panel
 *
 *  This Panel is for loading of images into Panorama. 
 *  Here one can set first values vor the camera orientation and
 *  link these parameters for the optimization.
 */
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
     *  @todo   react on different update signals more special
     */
//    virtual void panoramaChanged(PT::Panorama &pano);
     void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    /** sets the actual image to work on for roll, pitch and yaw
     */
    void SetImages ( wxListEvent & e );

 private:
    /** the model */
    Panorama &pano;

    // event handlers
    void OnAddImages(wxCommandEvent & e);
    void OnRemoveImages(wxCommandEvent & e);
    // Here we select the preview image

    /**  holds the images just in work
      *  in conjunction with SetImages()
      */
    unsigned int imgNr[512];
    /**  take sliders */
    void SetYaw ( wxCommandEvent & e );
    void SetPitch ( wxCommandEvent & e );
    void SetRoll ( wxCommandEvent & e );
    /**  take text */
    void SetYawText ( wxCommandEvent & e );
    void SetPitchText ( wxCommandEvent & e );
    void SetRollText ( wxCommandEvent & e );
    /** event -> pano
     *
     *  usually for events to set the new pano state
     *
     *  @param  type  "roll", "pitch" or "yaw"
     *  @param  var   the new value
     */
    void ChangePano ( std::string type, double var );
    /** Are we changing the pano?
     */
    bool changePano;

    /** Here we update the orientation values in the gui
     */
    void OrientationChanged ( void );

    // the image actually selected
    int orientationEdit_RefImg;


    /** pointer to the list control */
    List* images_list;

    DECLARE_EVENT_TABLE()
};

//------------------------------------------------------------------------------

/** simple Image Preview
 *
 *  Define a new canvas which can receive some events.
 *  Use pointerTo_ImgPreview->Refresh() to update if needed.
 *
 *  @todo  give the referenced image as an pointer in the argument list.
 */
class ImgPreview: public wxScrolledWindow
{
 public:
    ImgPreview(wxWindow *parent, const wxPoint& pos, const wxSize& size);
    ~ImgPreview(void) ;

 private:
    // Here we select the preview image
    void ChangePreview ( long item );

    void OnDraw(wxDC& dc);
    //void OnPaint(wxPaintEvent& event);

    DECLARE_EVENT_TABLE()
};

#endif // _IMAGESPANEL_H

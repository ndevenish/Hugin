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
#include "hugin/ImagesList.h"
#include "hugin/TextKillFocusHandler.h"

using namespace PT;

// forward declarations, to save the #include statements
class CPEditorPanel;

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
    virtual ~ImagesPanel(void) ;

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

private:
    // a window event
    void FitParent(wxSizeEvent & e);

    /** the model */
    Panorama &pano;

    // event handlers
    void OnAddImages(wxCommandEvent & e);
    void OnRemoveImages(wxCommandEvent & e);
    // Here we select the preview image

    /**  gui -> pano */
    void OnYawTextChanged ( wxCommandEvent & e );
    void OnPitchTextChanged ( wxCommandEvent & e );
    void OnRollTextChanged ( wxCommandEvent & e );

    void OnOptAnchorChanged(wxCommandEvent & e);
    void OnColorAnchorChanged(wxCommandEvent &e );
    
    /** gui -> pano
     *
     *  usually for events to set the new pano state
     *
     *  @param  type  "r", "p" or "y"
     *  @param  var   the new value
     */
    void ChangePano ( std::string type, double var );

    /** change displayed variables if the selection
     *  has changed.
     */
    void ListSelectionChanged(wxListEvent & e);

    /** pano -> gui
     */
    void ShowImgParameters(unsigned int imgNr);

    /** clear display */
    void ClearImgParameters();

    void DisableImageCtrls();
    void EnableImageCtrls();

    /** show a bigger thumbnail */
    void ShowImage(unsigned int imgNr);


    /** pointer to the list control */
    ImagesListImage* images_list;
    wxButton * m_optAnchorButton;
    wxButton * m_colorAnchorButton;

    TextKillFocusHandler * m_tkf;

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
    ImgPreview(wxWindow *parent, const wxPoint& pos, const wxSize& size, Panorama *pano);
    ~ImgPreview(void) ;

    // Here we select the preview image
    void ChangePreview ( wxImage & s_img );

 private:
    void OnMouse ( wxMouseEvent & event );

    void OnDraw(wxDC& dc);
    //void OnPaint(wxPaintEvent& event);

    /** the model */
    Panorama &pano;


    DECLARE_EVENT_TABLE()
};

#endif // _IMAGESPANEL_H

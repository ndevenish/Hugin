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

#ifndef _IMAGECENTER_H
#define _IMAGECENTER_H


#include "wx/frame.h"
//#include "wx/dnd.h"
//#include "wx/listctrl.h"

#include "PT/Panorama.h"
//#include "hugin/MainFrame.h"
//#include "hugin/List.h"

class CenterCanvas;

/** adjustment dialog
 *
 */
class ImgCenter: public wxDialog
{
 public:
    ImgCenter(wxWindow *parent, const wxPoint& pos, const wxSize& size, PT::Panorama& pano, const PT::UIntSet & i );
    ~ImgCenter(void) ;

    /** select the preview image */
    void ChangeView ( wxImage & s_img );

 private:
    PT::Panorama &pano;

    void FitParent ( wxSizeEvent & e );

    void OnApply ( wxCommandEvent & e );
    void OnClose ( wxCommandEvent & e );

    CenterCanvas *c_canvas;

    DECLARE_EVENT_TABLE()
};

/** adjustment image view
 *
 *  Define a new canvas which can receive some events.
 */
class CenterCanvas: public wxPanel
{
 public:
    CenterCanvas(wxWindow *parent, const wxPoint& pos, const wxSize& size, PT::Panorama& pano, const PT::UIntSet & i );
    ~CenterCanvas(void) ;

    /** Here we select the preview image */
    void ChangeView ( wxImage & s_img );

    /** change the pano variables */
    void ChangePano ( void );

 private:
    void Resize ( wxSizeEvent & e );
    void OnPaint(wxDC& dc);
    void OnMouse ( wxMouseEvent & event );

    /** the model */
    PT::Panorama &pano;

    // the image to adjust ( full scale )
    wxImage img;
    // the scaled image (clear/dirty)
    wxBitmap c_img;
    wxBitmap dirty_img;
    // first mouse click
    int first_x;
    int first_y;
    bool first_is_set;
    // second mouse click
    int second_x;
    int second_y;
    bool second_is_set;
    // zoom of view
    float zoom;
    // the values of the panorama
    float pt_d;
    float pt_e;
    // mouse coordinates
    int old_m_x;
    int old_m_y;

//    unsigned int *imgNr;
    PT::UIntSet imgNr;

    DECLARE_EVENT_TABLE()
};



#endif // _IMAGECENTER_H

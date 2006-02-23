// -*- c-basic-offset: 4 -*-
/** @file ImagesPanel.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de
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

class CenterCanvas;

/** adjustment dialog
 *
 */
class ImgCenter: public wxDialog
{
 public:
    ImgCenter(wxWindow *parent);
    virtual ~ImgCenter(void);

    /** select the preview image */
    void SetImage(wxImage & img);

    void SetParameters (const vigra::Rect2D & crop, bool circle, const vigra::Point2D & p, bool useCenter);

    void CropChanged(vigra::Rect2D & crop);

    vigra::Rect2D & getCrop();
    bool getCenterOnDE();

protected:

    /** update text */
    void UpdateDisplay();

    void OnClose ( wxCloseEvent & e );

    // reset crop area. 
    void OnResetButton(wxCommandEvent & e);

    void OnSetLeft(wxCommandEvent & e);
    void OnSetRight(wxCommandEvent & e);
    void OnSetTop(wxCommandEvent & e);
    void OnSetBottom(wxCommandEvent & e);
    void OnAutoCenter(wxCommandEvent & e);

    // ensure that the crop roi is centered
    void CenterCrop();

    wxTextCtrl * m_left_textctrl;
    wxTextCtrl * m_right_textctrl;
    wxTextCtrl * m_top_textctrl;
    wxTextCtrl * m_bottom_textctrl;
    wxCheckBox * m_autocenter_cb;


 private:
    CenterCanvas *m_Canvas;

    vigra::Rect2D m_roi;
    bool m_circle;
    vigra::Point2D m_center;

    DECLARE_EVENT_TABLE()
};

/** adjustment image view
 *
 *  Define a new canvas which can receive some events.
 */
class CenterCanvas: public wxPanel
{
public:
    CenterCanvas(wxWindow *parent, ImgCenter * listener, const wxSize & sz=wxDefaultSize);
    virtual ~CenterCanvas(void) ;

    /** set image and crop parameters */
    void SetImage(wxImage & img);
    void SetParameters (const vigra::Rect2D & crop, bool circle, const vigra::Point2D & center, bool centered);

    /** get the user selected crop area */
    vigra::Rect2D & getCrop()
    {
        return m_roi;
    }

protected:
    /** draw the view into the offscreen buffer */
    void DrawView();
    void UpdateCropCircle();

    wxCursor * m_cursor_no_sel;
    wxCursor * m_cursor_circ_drag;
    wxCursor * m_cursor_move_crop;
    wxCursor * m_cursor_drag_vert;
    wxCursor * m_cursor_drag_horiz;

private:
    void Resize ( wxSizeEvent & e );
    void OnPaint(wxPaintEvent & dc);
    void OnMouse ( wxMouseEvent & event );

    // the image to adjust ( full scale )
    wxImage img;
    // the scaled image (clear/dirty)
    wxBitmap m_scaled_img;
    // image with center cross and circle
    wxBitmap m_display_img;

    ImgCenter * m_listener;
    
    enum PointState { NO_SEL, RECT_DRAGGING, CIRC_DRAGGING, CROP_SELECTED, CROP_MOVE,
                      RECT_LEFT_DRAG, RECT_RIGHT_DRAG, RECT_TOP_DRAG, RECT_BOTTOM_DRAG};

    void ChangeMode(PointState m);
    
    wxPoint m_moveAnchor;

    PointState m_state;
    // the selected rectangle
    vigra::Rect2D m_roi;

    // circular crop
    bool m_circle;
    vigra::Point2D m_circ_first_point;
    vigra::Point2D m_circ_second_point;

    // center
    vigra::Point2D m_center;
    bool m_centered;

    // scale factor
    float m_scale;

    DECLARE_EVENT_TABLE()
};



#endif // _IMAGECENTER_H

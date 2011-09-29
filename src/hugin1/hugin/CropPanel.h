// -*- c-basic-offset: 4 -*-
/** @file CropPanel.h
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

#ifndef _CROP_PANEL_H
#define _CROP_PANEL_H

class CenterCanvas;
class ImagesListCrop;

#include <base_wx/wxImageCache.h>

/** adjustment dialog
 *
 */
class CropPanel: public wxPanel, public PT::PanoramaObserver
{
public:
    CropPanel();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));
    void Init(PT::Panorama * pano);

    virtual ~CropPanel(void);

#if 0
    /** restore layout after hugin start */
    void RestoreLayout();
    /// hack to restore the layout on next resize
    void RestoreLayoutOnNextResize()
    {
        m_restoreLayoutOnResize = true;
    }
#endif

    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet & imgNr);

    void CropChanged(vigra::Rect2D & crop, bool dragging);


protected:
    void Pano2Display(int imgNr);
    void Display2Pano();

    /** update GUI display */
    void UpdateDisplay();

    /** catches changes to the list selection */
    void ListSelectionChanged(wxListEvent& e);
    
    // Handle when background image loading completes
    void OnAsyncLoad (ImageCache::EntryPtr entry, std::string filename, bool load_small);
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

    Panorama       * m_pano;

    CenterCanvas   * m_Canvas;
    ImagesListCrop * m_imagesList;

    UIntSet m_selectedImages;
    std::string m_currentImageFile;
    ImageCache::RequestPtr m_imageRequest;
    /** Place holder for the image.
     *  Used when no image is selected, or we are waiting for an image to load.
     */
    ImageCache::EntryPtr m_transparentImage;
    /// File name for m_transparentImage.
    std::string m_transparentFilename;

    HuginBase::SrcPanoImage::CropMode m_cropMode;
    vigra::Rect2D m_cropRect;
    bool m_autoCenterCrop;
    vigra::Point2D m_center;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(CropPanel)
};

/** xrc handler */
class CropPanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(CropPanelXmlHandler)

    public:
        CropPanelXmlHandler();
        virtual wxObject *DoCreateResource();
        virtual bool CanHandle(wxXmlNode *node);
};

/** adjustment image view
 *
 *  Define a new canvas which can receive some events.
 */
class CenterCanvas: public wxPanel
{
public:
    CenterCanvas();
    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));
    void Init(CropPanel * listener);

    virtual ~CenterCanvas(void) ;

    /** set image and crop parameters */
    void SetImage(ImageCache::EntryPtr img);
    void UpdateDisplay(const vigra::Rect2D & crop, bool circle,
                       const vigra::Point2D & center, bool useCenter);

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
    ImageCache::EntryPtr m_imgCacheImg;
    wxImage img;
    // the scaled image (clear/dirty)
    wxBitmap m_scaled_img;
    // image with center cross and circle
    wxBitmap m_display_img;

    CropPanel * m_listener;

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
    DECLARE_DYNAMIC_CLASS(CenterCanvas)
};

/** xrc handler */
class CenterCanvasXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(CenterCanvasXmlHandler)

    public:
        CenterCanvasXmlHandler();
        virtual wxObject *DoCreateResource();
        virtual bool CanHandle(wxXmlNode *node);
};


#endif // _IMAGECENTER_H

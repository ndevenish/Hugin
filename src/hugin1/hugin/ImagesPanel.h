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

#include "hugin/MainFrame.h"
#include "hugin/ImagesList.h"

// Celeste files
#include "Celeste.h"
#include "CelesteGlobals.h"
#include "Utilities.h"

#include "base_wx/wxImageCache.h"

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
    ImagesPanel();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(PT::Panorama * pano);

    ~ImagesPanel();

    /** restore layout after hugin start */
    void RestoreLayout();

    /// hack to restore the layout on next resize
    void RestoreLayoutOnNextResize()
    {
        m_restoreLayoutOnResize = true;
    }

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
    /** Reloads the cp detector settings from config, necessary after edit preferences */
    void ReloadCPDetectorSettings();
    /** returns the default cp detector settings */
    CPDetectorSetting& GetDefaultSetting() { return cpdetector_config.settings.Item(cpdetector_config.GetDefaultGenerator());};
private:
    // a window event
    void OnSize(wxSizeEvent & e);

    /** the model */
    Panorama * pano;

    // event handlers
    void OnAddImages(wxCommandEvent & e);
    void OnRemoveImages(wxCommandEvent & e);
    void OnPositionChanged(wxSplitterEvent& event);

    // Here we select the preview image

    /**  gui -> pano */
    void OnVarTextChanged ( wxCommandEvent & e );
    void OnImageLinkChanged ( wxCommandEvent & e );

    void OnOptAnchorChanged(wxCommandEvent & e);
    void OnColorAnchorChanged(wxCommandEvent &e );

    void OnRemoveCtrlPoints(wxCommandEvent & e);
    void OnResetImagePositions(wxCommandEvent & e);

    void OnMoveImageUp(wxCommandEvent & e);
    void OnMoveImageDown(wxCommandEvent & e);
    
    void OnNewStack(wxCommandEvent & e);
    void OnChangeStack(wxCommandEvent & e); 

    /** gui -> pano
     *
     *  usually for events to set the new pano state
     *
     *  @param  type  "r", "p" or "y"
     *  @param  var   the new value
     */
    void ChangePano ( std::string type, double var );

    /** sift feature matching */
    void SIFTMatching(wxCommandEvent & e);
    /** clean the control points ala ptoclean */
    void OnCleanCP(wxCommandEvent & e);

    /** change displayed variables if the selection
     *  has changed.
     */
    void ListSelectionChanged(wxListEvent & e);

    void OnCelesteButton(wxCommandEvent & e);

    /** pano -> gui
     */
    void ShowImgParameters(unsigned int imgNr);

    /** clear display */
    void ClearImgParameters();

    void DisableImageCtrls();
    void EnableImageCtrls();

    /** show a bigger thumbnail */
    void ShowImage(unsigned int imgNr);
    void ShowExifInfo(unsigned int imgNr);
    void ClearImgExifInfo();
    void UpdatePreviewImage();

    /** bitmap with default image */
    wxBitmap m_empty;
    
    /** Request for thumbnail image */
    HuginBase::ImageCache::RequestPtr thumbnail_request;

    /** pointer to the list control */
    ImagesListImage* images_list;
    wxStaticBitmap * m_smallImgCtrl;
    unsigned m_showImgNr;

    wxButton * m_optAnchorButton;
    wxButton * m_colorAnchorButton;
    wxButton * m_moveUpButton;
    wxButton * m_moveDownButton;
    wxButton * m_stackNewButton;
    wxButton * m_stackChangeButton;

    wxButton * m_matchingButton;
    wxButton * m_cleaningButton;
    wxButton * m_removeCPButton;
    
    wxCheckBox * m_linkCheckBox;

    wxPanel *m_img_ctrls;
    wxChoice *m_CPDetectorChoice;
    //storing for different cp detector settings
    CPDetectorConfig cpdetector_config;

    int m_degDigits;

    bool m_restoreLayoutOnResize;

    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(ImagesPanel)
};

/** xrc handler */
class ImagesPanelXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(ImagesPanelXmlHandler)

public:
    ImagesPanelXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
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
    Panorama * pano;


    DECLARE_EVENT_TABLE()
};

#endif // _IMAGESPANEL_H

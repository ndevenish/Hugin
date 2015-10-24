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
 *  License along with this software. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _IMAGESPANEL_H
#define _IMAGESPANEL_H

#include "hugin/MainFrame.h"

#include "base_wx/wxImageCache.h"

// forward declarations, to save the #include statements
class ImagesTreeCtrl;

/** Hugin's first panel
 *
 *  This Panel is for loading of images into Panorama.
 *  Here one can set first values vor the camera orientation and
 *  link these parameters for the optimization.
 */
class ImagesPanel: public wxPanel, public HuginBase::PanoramaObserver
{
public:
    ImagesPanel();

    bool Create(wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxT("panel"));

    void Init(HuginBase::Panorama * pano);

    ~ImagesPanel();

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
//    virtual void panoramaChanged(HuginBase::Panorama &pano);
    /** receives notification about panorama changes */
    virtual void panoramaChanged(HuginBase::Panorama & pano);
    virtual void panoramaImagesChanged(HuginBase::Panorama &pano, const HuginBase::UIntSet & imgNr);
    /** Reloads the cp detector settings from config, necessary after edit preferences */
    void ReloadCPDetectorSettings();
    /** returns the default cp detector settings */
    CPDetectorSetting& GetDefaultSetting() { return cpdetector_config.settings.Item(cpdetector_config.GetDefaultGenerator());};

    /** sets the GuiLevel for all controls on this panel */
    void SetGuiLevel(GuiLevel newGuiLevel);
    /** return the currently selected optimizer setting as string from the drop down list box */
    wxString GetCurrentOptimizerString();
    /** run the cp generator with the given setting on selected images */
    void RunCPGenerator(CPDetectorSetting &setting, const HuginBase::UIntSet& img);
    /** runs the currently selected cp generator on given images */
    void RunCPGenerator(const HuginBase::UIntSet& img);
    /** return the currently selected cp generator description */
    const wxString GetSelectedCPGenerator();
protected:
    /** event handler for geometric optimizer */
    void OnOptimizeButton(wxCommandEvent &e);
    /** event handler for photometric optimizer */
    void OnPhotometricOptimizeButton(wxCommandEvent &e);

private:
    // a window event
    void OnSize(wxSizeEvent & e);

    /** the model */
    HuginBase::Panorama * m_pano;

    /** control point detection event handler*/
    void CPGenerate(wxCommandEvent & e);
    /** change displayed variables if the selection
     *  has changed.
     */
    void OnSelectionChanged(wxTreeEvent & e);

    /** updates the lens type for the selected images */
    void OnLensTypeChanged(wxCommandEvent & e);
    /** updates the focal length for the selected images */
    void OnFocalLengthChanged(wxCommandEvent & e);
    /** updates the crop factor for the selected images */
    void OnCropFactorChanged(wxCommandEvent & e);
    /** updates the minimum overlap */
    void OnMinimumOverlapChanged(wxCommandEvent & e);
    /** updates the max ev difference */
    void OnMaxEvDiffChanged(wxCommandEvent& e);

    /** event handler when grouping selection was changed */
    void OnGroupModeChanged(wxCommandEvent & e);
    /** event handler when display mode (which information should be shown) was changed */
    void OnDisplayModeChanged(wxCommandEvent & e);
    /** event handler, when optimizer master switch was changed */
    void OnOptimizerSwitchChanged(wxCommandEvent &e);
    /** event handler, when photometric optimizer master switch was changed */
    void OnPhotometricOptimizerSwitchChanged(wxCommandEvent &e);
    /** fills the grouping wxChoice with values depending on GuiLevel */
    void FillGroupChoice();
    /** fills the optmizer wxChoices with values depending on GuiLevel */
    void FillOptimizerChoice();

    void DisableImageCtrls();
    void EnableImageCtrls();

    /** show a bigger thumbnail */
    void ShowImage(unsigned int imgNr);
    void UpdatePreviewImage();

    /** bitmap with default image */
    wxBitmap m_empty;
    
    /** Request for thumbnail image */
    HuginBase::ImageCache::RequestPtr thumbnail_request;

    /** pointer to the main control */
    ImagesTreeCtrl* m_images_tree;
    /** pointer to the preview image control */
    wxStaticBitmap * m_smallImgCtrl;
    /** pointer to lens type selector */
    wxChoice *m_lenstype;
    /** pointer to optimizer switch selector */
    wxChoice *m_optChoice;
    /** pointer to photometric optimizer switch selector */
    wxChoice *m_optPhotoChoice;
    /** the text input control for focal length */
    wxTextCtrl *m_focallength;
    /** the text input control for crop factor */
    wxTextCtrl *m_cropfactor;
    /** the text input control for minimum overlap */
    wxTextCtrl *m_overlap;
    /** the text input control for max ev difference */
    wxTextCtrl *m_maxEv;
    size_t m_showImgNr;

    wxButton * m_matchingButton;
    wxChoice *m_CPDetectorChoice;
    //storing for different cp detector settings
    CPDetectorConfig cpdetector_config;

    GuiLevel m_guiLevel;
    int m_degDigits;

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

#endif // _IMAGESPANEL_H

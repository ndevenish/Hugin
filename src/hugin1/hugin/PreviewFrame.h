// -*- c-basic-offset: 4 -*-
/** @file PreviewFrame.h
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
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

#ifndef _PREVIEWFRAME_H
#define _PREVIEWFRAME_H

class PreviewPanel;
class wxToolBar;
class wxToggleButton;
class wxCheckBox;
#include <appbase/ProgressDisplayOld.h>

/** The image preview frame
 *
 *  Contains the ImagePreviewPanel and various controls for it.
 *
 *  it is not created with XRC, because it is highly dynamic, buttons
 *  have to be added etc.
 */
class PreviewFrame : public wxFrame, public PT::PanoramaObserver, public AppBase::MultiProgressDisplay
{
public:

    /** ctor.
     */
    PreviewFrame(wxFrame * frame, PT::Panorama &pano);

    /** dtor.
     */
    virtual ~PreviewFrame();

    void panoramaChanged(PT::Panorama &pano);
    void panoramaImagesChanged(PT::Panorama &pano, const PT::UIntSet &changed);
    void OnUpdate(wxCommandEvent& event);
    
    void updateProgressDisplay();
protected:
    void OnClose(wxCloseEvent& e);

    void OnChangeDisplayedImgs(wxCommandEvent & e);
    void OnAutoPreviewToggle(wxCommandEvent & e);
    void OnCenterHorizontally(wxCommandEvent & e);
    void OnFitPano(wxCommandEvent& e);
    void OnStraighten(wxCommandEvent & e);
    void OnShowAll(wxCommandEvent & e);
    void OnShowNone(wxCommandEvent & e);
    void OnNumTransform(wxCommandEvent & e);
    void OnChangeFOV(wxScrollEvent & e);
    void OnTextCtrlChanged(wxCommandEvent & e);

    void OnDefaultExposure( wxCommandEvent & e );
    void OnDecreaseExposure( wxSpinEvent & e );
    void OnIncreaseExposure( wxSpinEvent & e );
    /** event handler for reset projection parameters */
    void OnProjParameterReset(wxCommandEvent & e);

    void OnBlendChoice(wxCommandEvent & e);
    void OnProjectionChoice(wxCommandEvent & e);
    void OnOutputChoice(wxCommandEvent & e);
    /** event handler for full screen */
    void OnFullScreen(wxCommandEvent &e);
    /** event handler for undo */
    void OnUndo(wxCommandEvent &e);
    /** event handler for redo */
    void OnRedo(wxCommandEvent &e);

    // update the panorama display
    void updatePano();
private:

    PT::Panorama & m_pano;

    PreviewPanel * m_PreviewPanel;
    wxToolBar * m_ToolBar;
    wxSlider * m_HFOVSlider;
    wxSlider * m_VFOVSlider;
    wxChoice * m_BlendModeChoice;
    wxChoice * m_ProjectionChoice;
    wxChoice * m_outputModeChoice;
    wxTextCtrl * m_exposureTextCtrl;
    wxBitmapButton * m_defaultExposureBut;
    wxSpinButton * m_exposureSpinBut;

    wxString m_choices[3];
    int m_oldProjFormat;

//    wxButton * m_updatePreview;
//    wxCheckBox * m_autoCB;

	wxScrolledWindow * m_ButtonPanel;
	wxBoxSizer * m_ButtonSizer;
	wxStaticBoxSizer * m_ToggleButtonSizer;

    wxBoxSizer * m_topsizer;
    wxStaticBoxSizer * m_projParamSizer;
    std::vector<wxStaticText *> m_projParamNamesLabel;
    std::vector<wxTextCtrl *>   m_projParamTextCtrl;
    std::vector<wxSlider *>     m_projParamSlider;

#ifdef USE_TOGGLE_BUTTON
    std::vector<wxToggleButton *> m_ToggleButtons;
#else
    std::vector<wxCheckBox *> m_ToggleButtons;
#endif

    DECLARE_EVENT_TABLE()
};



#endif // _PREVIEWFRAME_H
